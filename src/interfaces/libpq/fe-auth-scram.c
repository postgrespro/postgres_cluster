/*-------------------------------------------------------------------------
 *
 * fe-auth-scram.c
 *	   The front-end (client) implementation of SCRAM authentication.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/interfaces/libpq/fe-auth-scram.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "common/base64.h"
#include "common/scram-common.h"
#include "fe-auth.h"

/*
 * Status of exchange messages used for SCRAM authentication via the
 * SASL protocol.
 */
typedef struct
{
	enum
	{
		INIT,
		NONCE_SENT,
		PROOF_SENT,
		FINISHED
	} state;

	const char *username;
	const char *password;

	char	   *client_first_message_bare;
	char	   *client_final_message_without_proof;

	/* These come from the server-first message */
	char	   *server_first_message;
	char	   *salt;
	int			saltlen;
	int			iterations;
	char	   *server_nonce;

	/* These come from the server-final message */
	char	   *server_final_message;
	char		ServerProof[SCRAM_KEY_LEN];
} fe_scram_state;

static bool read_server_first_message(fe_scram_state *state,
									  char *input,
									  PQExpBuffer errormessage);
static bool read_server_final_message(fe_scram_state *state,
									  char *input,
									  PQExpBuffer errormessage);
static char *build_client_first_message(fe_scram_state *state,
										PQExpBuffer errormessage);
static char *build_client_final_message(fe_scram_state *state,
										PQExpBuffer errormessage);
static bool verify_server_proof(fe_scram_state *state);
static bool generate_nonce(char *buf, int len);
static void calculate_client_proof(fe_scram_state *state,
					const char *client_final_message_without_proof,
					uint8 *result);

/*
 * Initialize SCRAM exchange status.
 */
void *
pg_fe_scram_init(const char *username, const char *password)
{
	fe_scram_state *state;

	state = (fe_scram_state *) malloc(sizeof(fe_scram_state));
	if (!state)
		return NULL;
	memset(state, 0, sizeof(fe_scram_state));
	state->state = INIT;
	state->username = username;
	state->password = password;

	return state;
}

/*
 * Free SCRAM exchange status
 */
void
pg_fe_scram_free(void *opaq)
{
	fe_scram_state *state = (fe_scram_state *) opaq;

	/* client messages */
	if (state->client_first_message_bare)
		free(state->client_first_message_bare);
	if (state->client_final_message_without_proof)
		free(state->client_final_message_without_proof);

	/* first message from server */
	if (state->server_first_message)
		free(state->server_first_message);
	if (state->salt)
		free(state->salt);
	if (state->server_nonce)
		free(state->server_nonce);

	/* final message from server */
	if (state->server_final_message)
		free(state->server_final_message);

	free(state);
}

/*
 * Exchange a SCRAM message with backend.
 */
void
pg_fe_scram_exchange(void *opaq, char *input, int inputlen,
					 char **output, int *outputlen,
					 bool *done, bool *success, PQExpBuffer errorMessage)
{
	fe_scram_state *state = (fe_scram_state *) opaq;

	*done = false;
	*success = false;
	*output = NULL;
	*outputlen = 0;

	switch (state->state)
	{
		case INIT:
			/* send client nonce */
			*output = build_client_first_message(state, errorMessage);
			if (*output == NULL)
			{
				*done = true;
				*success = false;
				break;
			}
			*outputlen = strlen(*output);
			*done = false;
			state->state = NONCE_SENT;
			break;

		case NONCE_SENT:
			/* receive salt and server nonce, send response */
			if (!read_server_first_message(state, input, errorMessage))
			{
				*done = true;
				*success = false;
				break;
			}
			*output = build_client_final_message(state, errorMessage);
			if (*output == NULL)
			{
				*done = true;
				*success = false;
				break;
			}
			*outputlen = strlen(*output);
			*done = false;
			state->state = PROOF_SENT;
			break;

		case PROOF_SENT:
			/* receive server proof, and verify it */
			if (!read_server_final_message(state, input, errorMessage))
			{
				*done = true;
				*success = false;
				break;
			}
			*success = verify_server_proof(state);
			*done = true;
			state->state = FINISHED;
			break;

		default:
			/* shouldn't happen */
			*done = true;
			*success = false;
			printfPQExpBuffer(errorMessage,
							  libpq_gettext("invalid SCRAM exchange state\n"));
	}
}

/*
 * Read value for an attribute part of a SASL message.
 *
 * This routine is able to handle error messages e= sent by the server
 * during the exchange of SASL messages.  Returns NULL in case of error,
 * setting errorMessage as well.
 */
static char *
read_attr_value(char **input, char attr, PQExpBuffer errorMessage)
{
	char		*begin = *input;
	char		*end;

	if (*begin == 'e')
	{
		/* Received an error */
		begin++;
		if (*begin != '=')
		{
			printfPQExpBuffer(errorMessage,
				libpq_gettext("malformed SCRAM message (expected = in attr e)\n"));
			return NULL;
		}

		begin++;
		printfPQExpBuffer(errorMessage,
			libpq_gettext("error received from server in SASL exchange: %s\n"),
						  begin);
		return NULL;
	}

	if (*begin != attr)
	{
		printfPQExpBuffer(errorMessage,
			libpq_gettext("malformed SCRAM message (%c expected)\n"),
						  attr);
		return NULL;
	}
	begin++;

	if (*begin != '=')
	{
		printfPQExpBuffer(errorMessage,
			libpq_gettext("malformed SCRAM message (expected = in attr %c)\n"),
						  attr);
		return NULL;
	}
	begin++;

	end = begin;
	while (*end && *end != ',')
		end++;

	if (*end)
	{
		*end = '\0';
		*input = end + 1;
	}
	else
		*input = end;

	return begin;
}

/*
 * Build the first exchange message sent by the client.
 */
static char *
build_client_first_message(fe_scram_state *state, PQExpBuffer errormessage)
{
	char		nonce[SCRAM_NONCE_LEN + 1];
	char	   *buf;
	char		msglen;

	if (!generate_nonce(nonce, SCRAM_NONCE_LEN))
	{
		printfPQExpBuffer(errormessage, libpq_gettext("failed to generate nonce\n"));
		return NULL;
	}

	/* Generate message */
	msglen = 5 + strlen(state->username) + 3 + strlen(nonce);
	buf = malloc(msglen + 1);
	if (buf == NULL)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return NULL;
	}
	snprintf(buf, msglen + 1, "n,,n=%s,r=%s", state->username, nonce);

	state->client_first_message_bare = strdup(buf + 3);
	if (!state->client_first_message_bare)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return NULL;
	}

	return buf;
}

/*
 * Build the final exchange message sent from the client.
 */
static char *
build_client_final_message(fe_scram_state *state, PQExpBuffer errormessage)
{
	char		client_final_message_without_proof[200];
	uint8		client_proof[SCRAM_KEY_LEN];
	char		client_proof_base64[SCRAM_KEY_LEN * 2 + 1];
	int			client_proof_len;
	char		buf[300];

	snprintf(client_final_message_without_proof,
			 sizeof(client_final_message_without_proof),
			 "c=biws,r=%s", state->server_nonce);

	calculate_client_proof(state,
						   client_final_message_without_proof,
						   client_proof);

	if (pg_b64_enc_len(SCRAM_KEY_LEN) > sizeof(client_proof_base64))
	{
		printfPQExpBuffer(errormessage,
			libpq_gettext("malformed client proof (%d found)\n"),
						  pg_b64_enc_len(SCRAM_KEY_LEN));
		return NULL;
	}

	client_proof_len = pg_b64_encode((char *) client_proof,
									 SCRAM_KEY_LEN,
									 client_proof_base64);
	if (client_proof_len < 0)
	{
		printfPQExpBuffer(errormessage,
			  libpq_gettext("failure when encoding client proof\n"));
		return NULL;
	}
	client_proof_base64[client_proof_len] = '\0';

	state->client_final_message_without_proof =
		strdup(client_final_message_without_proof);
	if (state->client_final_message_without_proof == NULL)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return NULL;
	}

	snprintf(buf, sizeof(buf), "%s,p=%s",
			 client_final_message_without_proof,
			 client_proof_base64);

	return strdup(buf);
}

/*
 * Read the first exchange message coming from the server.
 */
static bool
read_server_first_message(fe_scram_state *state,
						  char *input,
						  PQExpBuffer errormessage)
{
	char	   *iterations_str;
	char	   *endptr;
	char	   *encoded_salt;
	char	   *server_nonce;

	state->server_first_message = strdup(input);
	if (!state->server_first_message)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return false;
	}

	/* parse the message */
	server_nonce = read_attr_value(&input, 'r', errormessage);
	if (server_nonce == NULL)
	{
		/* read_attr_value() has generated an error string */
		return false;
	}

	state->server_nonce = strdup(server_nonce);
	if (state->server_nonce == NULL)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return false;
	}

	encoded_salt = read_attr_value(&input, 's', errormessage);
	if (encoded_salt == NULL)
	{
		/* read_attr_value() has generated an error string */
		return false;
	}
	state->salt = malloc(pg_b64_dec_len(strlen(encoded_salt)));
	if (state->salt == NULL)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return false;
	}
	state->saltlen = pg_b64_decode(encoded_salt,
								   strlen(encoded_salt),
								   state->salt);
	if (state->saltlen != SCRAM_SALT_LEN)
	{
		printfPQExpBuffer(errormessage,
			  libpq_gettext("invalid salt length: found %d, expected %d\n"),
			  state->saltlen, SCRAM_SALT_LEN);
		return false;
	}

	iterations_str = read_attr_value(&input, 'i', errormessage);
	if (iterations_str == NULL || *input != '\0')
	{
		/* read_attr_value() has generated an error string */
		return false;
	}
	state->iterations = strtol(iterations_str, &endptr, SCRAM_ITERATION_LEN);
	if (*endptr != '\0')
	{
		printfPQExpBuffer(errormessage,
			  libpq_gettext("malformed SCRAM message for number of iterations\n"));
		return false;
	}

	return true;
}

/*
 * Read the final exchange message coming from the server.
 */
static bool
read_server_final_message(fe_scram_state *state,
						  char *input,
						  PQExpBuffer errormessage)
{
	char	   *encoded_server_proof;
	int			server_proof_len;

	state->server_final_message = strdup(input);
	if (!state->server_final_message)
	{
		printfPQExpBuffer(errormessage, libpq_gettext("out of memory\n"));
		return false;
	}

	/* parse the message */
	encoded_server_proof = read_attr_value(&input, 'v', errormessage);
	if (encoded_server_proof == NULL || *input != '\0')
	{
		/* read_attr_value() has generated an error message */
		return false;
	}

	server_proof_len = pg_b64_decode(encoded_server_proof,
									 strlen(encoded_server_proof),
									 state->ServerProof);
	if (server_proof_len != SCRAM_KEY_LEN)
	{
		printfPQExpBuffer(errormessage,
						  libpq_gettext("invalid server proof\n"));
		return false;
	}

	return true;
}

/*
 * Calculate the client proof, part of the final exchange message sent
 * by the client.
 */
static void
calculate_client_proof(fe_scram_state *state,
					   const char *client_final_message_without_proof,
					   uint8 *result)
{
	uint8		StoredKey[SCRAM_KEY_LEN];
	uint8		ClientKey[SCRAM_KEY_LEN];
	uint8		ClientSignature[SCRAM_KEY_LEN];
	int			i;
	scram_HMAC_ctx ctx;

	scram_ClientOrServerKey(state->password, state->salt, state->saltlen,
							state->iterations, SCRAM_CLIENT_KEY_NAME, ClientKey);
	scram_H(ClientKey, SCRAM_KEY_LEN, StoredKey);

	scram_HMAC_init(&ctx, StoredKey, SCRAM_KEY_LEN);
	scram_HMAC_update(&ctx,
					  state->client_first_message_bare,
					  strlen(state->client_first_message_bare));
	scram_HMAC_update(&ctx, ",", 1);
	scram_HMAC_update(&ctx,
					  state->server_first_message,
					  strlen(state->server_first_message));
	scram_HMAC_update(&ctx, ",", 1);
	scram_HMAC_update(&ctx,
					  client_final_message_without_proof,
					  strlen(client_final_message_without_proof));
	scram_HMAC_final(ClientSignature, &ctx);

	for (i = 0; i < SCRAM_KEY_LEN; i++)
		result[i] = ClientKey[i] ^ ClientSignature[i];
}

/*
 * Validate the server proof, received as part of the final exchange message
 * received from the server.
 */
static bool
verify_server_proof(fe_scram_state *state)
{
	uint8		ServerSignature[SCRAM_KEY_LEN];
	uint8		ServerKey[SCRAM_KEY_LEN];
	scram_HMAC_ctx ctx;

	scram_ClientOrServerKey(state->password, state->salt, state->saltlen,
							state->iterations, SCRAM_SERVER_KEY_NAME,
							ServerKey);

	/* calculate ServerSignature */
	scram_HMAC_init(&ctx, ServerKey, SCRAM_KEY_LEN);
	scram_HMAC_update(&ctx,
					  state->client_first_message_bare,
					  strlen(state->client_first_message_bare));
	scram_HMAC_update(&ctx, ",", 1);
	scram_HMAC_update(&ctx,
					  state->server_first_message,
					  strlen(state->server_first_message));
	scram_HMAC_update(&ctx, ",", 1);
	scram_HMAC_update(&ctx,
					  state->client_final_message_without_proof,
					  strlen(state->client_final_message_without_proof));
	scram_HMAC_final(ServerSignature, &ctx);

	if (memcmp(ServerSignature, state->ServerProof, SCRAM_KEY_LEN) != 0)
		return false;

	return true;
}

/*
 * Generate nonce with some randomness.
 * Returns true of nonce has been succesfully generated, and false
 * otherwise.
 */
static bool
generate_nonce(char *buf, int len)
{
	int		count = 0;

	/* compute the salt to use for computing responses */
	while (count < len)
	{
		char	byte;

		if (!pg_strong_random(&byte, 1))
			return false;

		/*
		 * Only ASCII printable characters, except commas are accepted in
		 * the nonce.
		 */
		if (byte < '!' || byte > '~' || byte == ',')
			continue;

		buf[count] = byte;
		count++;
	}

	buf[len] = '\0';
	return true;
}
