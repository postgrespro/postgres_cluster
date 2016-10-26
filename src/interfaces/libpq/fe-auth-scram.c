/*-------------------------------------------------------------------------
 *
 * fe-auth-scram.c
 *	   The front-end (client) implementation of SCRAM authentication.
 *
 * See the following RFCs 5802 and RFC 7666 for more details:
 * - RFC 5802: https://tools.ietf.org/html/rfc5802
 * - RFC 7677: https://tools.ietf.org/html/rfc7677
 *
 * Here are some differences:
 *
 * - Username from the authentication exchange is not used. The client
 *	 should send an empty string as the username.
 * - Password is not processed with the SASLprep algorithm.
 * - Channel binding is not supported yet.
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

#include "common/encode_utils.h"
#include "common/scram-common.h"
#include "fe-auth.h"

/*
 * Status of exchange messages used for SCRAM authentication via the
 * SASL protocol.
 */
typedef enum
{
	INIT,						/* Send client-first-message */
	NONCE_SENT,					/* Receive server-first-message, respond with
								 * client-final-message */
	PROOF_SENT,					/* Receive server-final-message, verify proof */
	FINISHED
} fe_scram_exchange_state;

typedef struct
{
	fe_scram_exchange_state state;

	/* Supplied by caller */
	const char *username;
	const char *password;

	char		client_nonce[SCRAM_NONCE_LEN + 1];
	char	   *client_first_message_bare;
	char	   *client_final_message_without_proof;

	/* Fields from the server-first message */
	char	   *server_first_message;
	char	   *salt;
	int			saltlen;
	int			iterations;
	char	   *nonce;			/* client+server nonces */

	/* Fields from the server-final message */
	char	   *server_final_message;
	char		ServerProof[SCRAM_KEY_LEN];
} fe_scram_state;

static bool read_server_first_message(fe_scram_state *state,
						  char *input,
						  PQExpBuffer errormessage);
static bool read_server_final_message(fe_scram_state *state,
						  char *input,
						  PQExpBuffer errormessage);
static char *build_client_first_message(fe_scram_state *state);
static char *build_client_final_message(fe_scram_state *state);
static bool verify_server_proof(fe_scram_state *state);
static void generate_nonce(char *buf, int len);
static void calculate_client_proof(fe_scram_state *state,
					   const char *client_final_message_without_proof,
					   uint8 *result);

/*
 * Initialize a SCRAM authentication exchange.
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
 * Free SCRAM exchange state.
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
	if (state->nonce)
		free(state->nonce);

	/* final message from server */
	if (state->server_final_message)
		free(state->server_final_message);

	free(state);
}

/*
 * Exchange a SCRAM message with backend.
 */
void
pg_fe_scram_exchange(void *opaq,
					 char *input, int inputlen,
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
			/* send client_first_message */
			*output = build_client_first_message(state);
			if (*output == NULL)
			{
				printfPQExpBuffer(errorMessage,
								  libpq_gettext("out of memory"));
				*done = true;
				state->state = FINISHED;
				return;
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
				state->state = FINISHED;
				return;
			}
			*output = build_client_final_message(state);
			if (*output == NULL)
			{
				printfPQExpBuffer(errorMessage,
								  libpq_gettext("out of memory"));
				*done = true;
				state->state = FINISHED;
				return;
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
				state->state = FINISHED;
				return;
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
							  libpq_gettext("invalid SCRAM exchange state"));
	}
}

/*
 * Read value for an attribute part of a SASL message.
 */
static char *
read_attr_value(char **input, char attr, PQExpBuffer errorMessage)
{
	char	   *begin = *input;
	char	   *end;

	if (*begin != attr)
	{
		printfPQExpBuffer(errorMessage,
			   libpq_gettext("malformed SCRAM message (%c expected)"), attr);
		return NULL;
	}
	begin++;

	if (*begin != '=')
	{
		printfPQExpBuffer(errorMessage,
		  libpq_gettext("malformed SCRAM message (expected '=' in attr %c)"),
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
 *
 * Returns NULL on out-of-memory.
 */
static char *
build_client_first_message(fe_scram_state *state)
{
	char	   *buf;
	char		msglen;

	generate_nonce(state->client_nonce, SCRAM_NONCE_LEN);

	/* Construct message */
	msglen = 5 + strlen(state->username) + 3 + strlen(state->client_nonce);
	buf = malloc(msglen + 1);
	if (!buf)
		return NULL;
	snprintf(buf, msglen + 1, "n,,n=%s,r=%s",
			 state->username, state->client_nonce);

	/* Save the client_first_message_bare part for later */
	state->client_first_message_bare = strdup(buf + 3);
	if (!state->client_first_message_bare)
	{
		free(buf);
		return NULL;
	}

	return buf;
}

/*
 * Build the final exchange message sent by the client.
 *
 * Returns NULL on out-of-memory.
 */
static char *
build_client_final_message(fe_scram_state *state)
{
	int			len;
	char	   *buf;
	uint8		client_proof[SCRAM_KEY_LEN];
	char		client_proof_base64[SCRAM_KEY_LEN * 2 + 1];
	int			client_proof_len;

	/* Build client_final_message_without_proof */
	len = 9 + strlen(state->nonce);
	buf = malloc(len + 1);
	if (!buf)
		return NULL;
	snprintf(buf, len + 1,
			 "c=biws,r=%s", state->nonce);
	state->client_final_message_without_proof = buf;

	/* Calculate client-proof */
	calculate_client_proof(state,
						   state->client_final_message_without_proof,
						   client_proof);
	if (b64_enc_len((char *) client_proof, SCRAM_KEY_LEN) > sizeof(client_proof_base64))
		return NULL;

	client_proof_len =
		b64_encode((char *) client_proof, SCRAM_KEY_LEN, client_proof_base64);
	client_proof_base64[client_proof_len] = '\0';

	/* Build client_final_message (with the proof) */
	len = strlen(state->client_final_message_without_proof) + 3 + client_proof_len;
	buf = malloc(len + 1);
	if (!buf)
		return NULL;
	snprintf(buf, len + 1, "%s,p=%s",
			 state->client_final_message_without_proof,
			 client_proof_base64);

	return buf;
}

/*
 * Read the first exchange message coming from the server.
 *
 * Returns true on success. On failure, returns false,
 * and adds details to 'errormessage'.
 */
static bool
read_server_first_message(fe_scram_state *state,
						  char *input,
						  PQExpBuffer errorMessage)
{
	char	   *iterations_str;
	char	   *endptr;
	char	   *encoded_salt;
	char	   *s;

	state->server_first_message = strdup(input);
	if (!state->server_first_message)
		return false;

	/*----------
	 * Parse the message.
	 *
	 * According to RFC 5820, the syntax of the server-first-message is:
	 *
	 * server-first-message =
	 *					 [reserved-mext ","] nonce "," salt ","
	 *					 iteration-count ["," extensions]
	 *
	 * reserved-mext   = "m=" 1*(value-char)
	 *					 ;; Reserved for signaling mandatory extensions.
	 *					 ;; The exact syntax will be defined in
	 *					 ;; the future.
	 *
	 * nonce		   = "r=" c-nonce [s-nonce]
	 *					 ;; Second part provided by server.
	 *
	 * c-nonce		   = printable
	 *
	 * s-nonce		   = printable
	 *
	 * salt			   = "s=" base64
	 *
	 * iteration-count = "i=" posit-number
	 *					 ;; A positive number.
	 *
	 * Example:
	 * r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=4096
	 *----------
	 */

	/*
	 * Check for possible mandatory extensions first. We don't support any, so
	 * throw an error, if there are any.
	 */
	if (*input == 'm')
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("server requires a SCRAM extension that is not supported"));
		return false;
	}

	/*
	 * Read nonce. It consists of the client-nonce that we sent earlier, and
	 * the server-generated part. Check that the client-nonce matches what we
	 * sent.
	 */
	s = read_attr_value(&input, 'r', errorMessage);
	if (!s)
		return false;
	state->nonce = strdup(s);
	if (state->nonce == NULL)
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("out of memory"));
		return false;
	}

	if (strncmp(state->nonce, state->client_nonce, strlen(state->client_nonce)) != 0)
	{
		printfPQExpBuffer(errorMessage,
					 libpq_gettext("invalid client-nonce in SCRAM message"));
		return false;
	}

	/* Read the salt */
	encoded_salt = read_attr_value(&input, 's', errorMessage);
	if (encoded_salt == NULL)
		return false;
	state->salt = malloc(b64_dec_len(encoded_salt, strlen(encoded_salt)));
	if (state->salt == NULL)
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("out of memory"));
		return false;
	}
	state->saltlen = b64_decode(encoded_salt, strlen(encoded_salt), state->salt);

	/* FIXME: shouldn't we allow any salt length? */
	if (state->saltlen != SCRAM_SALT_LEN)
	{
		printfPQExpBuffer(errorMessage,
			 libpq_gettext("malformed SCRAM message (invalid salt length)"));
		return false;
	}

	/* Read iteration count */
	iterations_str = read_attr_value(&input, 'i', errorMessage);
	if (iterations_str == NULL)
		return false;
	state->iterations = strtol(iterations_str, &endptr, 10);
	if (*endptr != '\0' || state->iterations < 1)
	{
		printfPQExpBuffer(errorMessage,
		 libpq_gettext("malformed SCRAM message (invalid # of iterations)"));
		return false;
	}

	if (*input != '\0')
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("malformed SCRAM message (garbage after end of message)"));
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
						  PQExpBuffer errorMessage)
{
	char	   *encoded_server_proof;
	int			server_proof_len;
	char		buf[SCRAM_KEY_LEN * 2];

	state->server_final_message = strdup(input);
	if (!state->server_final_message)
		return false;

	/*
	 * Parse the message. It consists of a single "v=..." attribute,
	 * containing the base64-encoded server-proof.
	 */

	/*
	 * FIXME: also parse a possible server-error attribute. The server
	 * currently never sends that, but still.
	 */
	encoded_server_proof = read_attr_value(&input, 'v', errorMessage);
	if (encoded_server_proof == NULL)
		return false;

	if (b64_dec_len(encoded_server_proof,
					strlen(encoded_server_proof)) > sizeof(buf))
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("malformed SCRAM message (invalid ServerProof length)"));
		return false;
	}

	server_proof_len = b64_decode(encoded_server_proof,
								  strlen(encoded_server_proof),
								  buf);
	if (server_proof_len != SCRAM_KEY_LEN)
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("malformed SCRAM message (invalid ServerProof length)"));
		return false;
	}
	memcpy(state->ServerProof, buf, SCRAM_KEY_LEN);

	if (*input != '\0')
	{
		printfPQExpBuffer(errorMessage,
						  libpq_gettext("malformed SCRAM message (garbage after end of message)"));
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
 *
 * It's important to verify the server-proof, even if the server subsequently
 * accepts the login, to detect that the server is genuine! (XXX: An impersonating
 * server could choose to present us an MD5 challenge instead, or no challenge
 * at all, but that's a different problem.)
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
 */
static void
generate_nonce(char *buf, int len)
{
	int			i;

	/* FIXME: use RAND_bytes() if available? */
	for (i = 0; i < len; i++)
		buf[i] = random() % 255 + 1;

	buf[len] = '\0';
}
