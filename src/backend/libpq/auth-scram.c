/*-------------------------------------------------------------------------
 *
 * auth-scram.c
 *	  Server-side implementation of the SASL SCRAM mechanism.
 *
 * See the following RFCs 5802 and RFC 7666 for more details:
 * - RFC 5802: https://tools.ietf.org/html/rfc5802
 * - RFC 7677: https://tools.ietf.org/html/rfc7677
 *
 * Here are some differences:
 *
 * - Username from the authentication exchange is not used. The client
 *   should send an empty string as the username.
 * - Password is not processed with the SASLprep algorithm.
 * - Channel binding is not supported yet.
 *
 * The password stored in pg_authid consists of the salt, iteration count,
 * StoredKey and ServerKey.
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/libpq/auth-scram.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>

#include "catalog/pg_authid.h"
#include "common/base64.h"
#include "common/scram-common.h"
#include "common/sha2.h"
#include "libpq/auth.h"
#include "libpq/crypt.h"
#include "libpq/scram.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"

/*
 * Status data for SCRAM.  This should be kept internal to this file.
 */
typedef struct
{
	enum
	{
		INIT,
		SALT_SENT,
		FINISHED
	} state;

	const char *username;	/* username from startup packet */
	char	   *salt;		/* base64-encoded */
	int			iterations;
	uint8		StoredKey[SCRAM_KEY_LEN];
	uint8		ServerKey[SCRAM_KEY_LEN];

	/* Fields of the first message from client */
	char	   *client_first_message_bare;
	char	   *client_username;
	char	   *client_authzid;
	char	   *client_nonce;

	/* Fields from the last message from client */
	char	   *client_final_message_without_proof;
	char	   *client_final_nonce;
	char		ClientProof[SCRAM_KEY_LEN];

	/* Server-side status fields */
	char	   *server_first_message;
	char	   *server_nonce;		/* base64-encoded */
	char	   *server_signature;
} scram_state;

/*
 * Internal error codes for exchange functions.
 * Callers of the exchange routines do not need to be aware of any of
 * that and should just send messages generated here.
 */
typedef enum
{
	SASL_NO_ERROR = 0,
	/* error codes */
	SASL_INVALID_ENCODING,
	SASL_EXTENSIONS_NOT_SUPPORTED,
	SASL_CHANNEL_BINDING_UNMATCH,
	SASL_CHANNEL_BINDING_NO_SUPPORT,
	SASL_CHANNEL_BINDING_TYPE_NOT_SUPPORTED,
	SASL_UNKNOWN_USER,
	SASL_INVALID_PROOF,
	SASL_INVALID_USERNAME_ENCODING,
	SASL_NO_RESOURCES,
	/*
	 * Unrecognized errors should be treated as "other-error". In order to
	 * prevent information disclosure, the server may substitute the real
	 * reason with "other-error".
	 */
	SASL_OTHER_ERROR
} SASLStatus;


static SASLStatus read_client_first_message(scram_state *state,
											char *input, char **logdetail);
static SASLStatus read_client_final_message(scram_state *state,
											char *input, char **logdetail);
static char *build_server_first_message(scram_state *state);
static char *build_server_final_message(scram_state *state);
static char *build_error_message(SASLStatus status);
static SASLStatus check_client_data(void *opaque, char **logdetail);
static bool verify_client_proof(scram_state *state);
static bool verify_final_nonce(scram_state *state);
static bool parse_scram_verifier(const char *verifier, char **salt,
				 int *iterations, char **stored_key, char **server_key);
static void generate_nonce(char *out, int len);

/*
 * build_error_message
 *
 * Build an error message for a problem that happened during the SASL
 * message exchange.  Those messages are formatted with e= as prefix
 * and need to be sent back to the client.
 */
static char *
build_error_message(SASLStatus status)
{
	char *res = NULL;

	/*
	 * The following error format is respected here:
	 *
	 * server-error = "e=" server-error-value
	 *
	 * server-error-value = "invalid-encoding" /
	 *           "extensions-not-supported" /  ; unrecognized 'm' value
	 *            "invalid-proof" /
	 *            "channel-bindings-dont-match" /
	 *            "server-does-support-channel-binding" /
	 *              ; server does not support channel binding
	 *            "channel-binding-not-supported" /
	 *            "unsupported-channel-binding-type" /
	 *            "unknown-user" /
	 *            "invalid-username-encoding" /
	 *              ; invalid username encoding (invalid UTF-8 or
	 *              ; SASLprep failed)
	 *            "no-resources" /
	 *            "other-error" /
	 *            server-error-value-ext
	 *     ; Unrecognized errors should be treated as "other-error".
	 *     ; In order to prevent information disclosure, the server
	 *     ; may substitute the real reason with "other-error".
	 *
	 * server-error-value-ext = value
	 *     ; Additional error reasons added by extensions
	 *     ; to this document.
	 */

	switch (status)
	{
		case SASL_INVALID_ENCODING:
			res = psprintf("e=invalid-encoding");
			break;
		case SASL_EXTENSIONS_NOT_SUPPORTED:
			res = psprintf("e=extensions-not-supported");
			break;
		case SASL_INVALID_PROOF:
			res = psprintf("e=invalid-proof");
			break;
		case SASL_CHANNEL_BINDING_UNMATCH:
			res = psprintf("e=channel-bindings-dont-match");
			break;
		case SASL_CHANNEL_BINDING_NO_SUPPORT:
			res = psprintf("e=server-does-support-channel-binding");
			break;
		case SASL_CHANNEL_BINDING_TYPE_NOT_SUPPORTED:
			res = psprintf("e=unsupported-channel-binding-type");
			break;
		case SASL_UNKNOWN_USER:
			res = psprintf("e=unknown-user");
			break;
		case SASL_INVALID_USERNAME_ENCODING:
			res = psprintf("e=invalid-username-encoding");
			break;
		case SASL_NO_RESOURCES:
			res = psprintf("e=no-resources");
			break;
		case SASL_OTHER_ERROR:
			res = psprintf("e=other-error");
			break;
		case SASL_NO_ERROR:
		default:
			Assert(0);	/* should not happen */
	}

	Assert(res != NULL);
	return res;
}

/*
 * pg_be_scram_init
 *
 * Initialize a new SCRAM authentication exchange status tracker. This
 * needs to be called before doing any exchange. It will be filled later
 * after the beginning of the exchange with verifier data.
 */
void *
pg_be_scram_init(char *username)
{
	scram_state *state;

	state = (scram_state *) palloc0(sizeof(scram_state));
	state->state = INIT;
	state->username = username;
	state->salt = NULL;

	return state;
}

/*
 * check_client_data
 *
 * Fill into the SASL exchange status all the information related to user and
 * perform sanity checks.
 */
static SASLStatus
check_client_data(void *opaque, char **logdetail)
{
	scram_state *state = (scram_state *) opaque;
	char   *server_key;
	char   *stored_key;
	char   *salt;
	int		iterations;
	int		res;
	char   *passwd;
	TimestampTz vuntil = 0;
	bool	vuntil_null;

	/* compute the salt to use for computing responses */
	if (!pg_strong_random(MyProcPort->SASLSalt, sizeof(MyProcPort->SASLSalt)))
	{
		*logdetail = psprintf(_("Could not generate random salt"));
		return SASL_OTHER_ERROR;
	}

	/*
	 * Fetch details about role needed for password checks.
	 */
	res = get_role_details(state->username, &passwd, &vuntil,
						   &vuntil_null,
						   logdetail);

	/*
	 * Check if an error has happened.  "logdetail" is already filled,
	 * here we just need to find what is the error mapping with the SASL
	 * exchange and let the client know what happened.
	 */
	if (res == PG_ROLE_NOT_DEFINED)
		return SASL_UNKNOWN_USER;
	else if (res == PG_ROLE_NO_PASSWORD ||
			 res == PG_ROLE_EMPTY_PASSWORD)
		return SASL_OTHER_ERROR;

	/*
	 * Check password validity, there is nothing to do if the password
	 * validity field is null.
	 */
	if (!vuntil_null)
	{
		if (vuntil < GetCurrentTimestamp())
		{
			*logdetail = psprintf(_("User \"%s\" has an expired password."),
								  state->username);
			pfree(passwd);
			return SASL_OTHER_ERROR;
		}
	}

	/* The SCRAM verifier needs to be in correct shape as well. */
	if (!parse_scram_verifier(passwd, &salt, &iterations,
							  &stored_key, &server_key))
	{
		*logdetail = psprintf(_("User \"%s\" does not have a valid SCRAM verifier."),
							  state->username);
		pfree(passwd);
		return SASL_OTHER_ERROR;
	}

	/* OK to fill in everything */
	state->salt = salt;
	state->iterations = iterations;
	memcpy(state->ServerKey, server_key, SCRAM_KEY_LEN);
	memcpy(state->StoredKey, stored_key, SCRAM_KEY_LEN);
	pfree(stored_key);
	pfree(server_key);
	pfree(passwd);
	return SASL_NO_ERROR;
}

/*
 * Continue a SCRAM authentication exchange.
 *
 * The next message to send to client is saved in "output", for a length
 * of "outputlen".  In the case of an error, optionally store a palloc'd
 * string at *logdetail that will be sent to the postmaster log (but not
 * the client).
 */
int
pg_be_scram_exchange(void *opaq, char *input, int inputlen,
					 char **output, int *outputlen, char **logdetail)
{
	scram_state	   *state = (scram_state *) opaq;
	SASLStatus		status;
	int				result;

	*output = NULL;
	*outputlen = 0;

	if (inputlen > 0)
		elog(DEBUG4, "got SCRAM message: %s", input);

	switch (state->state)
	{
		case INIT:
			/*
			 * Initialization phase.  Things happen in the following order:
			 * 1) Receive the first message from client and be sure that it
			 *    is parsed correctly.
			 * 2) Validate the user information. A couple of things are done
			 *    here, mainly validity checks on the password and the user.
			 * 3) Send the challenge to the client.
			 */
			status = read_client_first_message(state, input, logdetail);
			if (status != SASL_NO_ERROR)
			{
				*output = build_error_message(status);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}

			/* check validity of user data */
			status = check_client_data(state, logdetail);
			if (status != SASL_NO_ERROR)
			{
				*output = build_error_message(status);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}

			/* prepare message to send challenge */
			*output = build_server_first_message(state);
			if (*output == NULL)
			{
				*output = build_error_message(SASL_OTHER_ERROR);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}
			*outputlen = strlen(*output);
			state->state = SALT_SENT;
			result = SASL_EXCHANGE_CONTINUE;
			break;

		case SALT_SENT:
			/*
			 * Final phase for the server. First receive the response to
			 * the challenge previously sent and then let the client know
			 * that everything went well.
			 */
			status = read_client_final_message(state, input, logdetail);
			if (status != SASL_NO_ERROR)
			{
				*output = build_error_message(status);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}

			/* Now check the final nonce and the client proof */
			if (!verify_final_nonce(state) ||
				!verify_client_proof(state))
			{
				*output = build_error_message(SASL_INVALID_PROOF);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}

			/* Build final message for client */
			*output = build_server_final_message(state);
			if (*output == NULL)
			{
				*output = build_error_message(SASL_OTHER_ERROR);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_FAILURE;
				break;
			}

			/* Success! */
			*outputlen = strlen(*output);
			result = SASL_EXCHANGE_SUCCESS;
			state->state = FINISHED;
			break;

		default:
			elog(ERROR, "invalid SCRAM exchange state");
			result = 0;
	}

	return result;
}

/*
 * Construct a verifier string for SCRAM, stored in pg_authid.rolpassword.
 *
 * If iterations is 0, default number of iterations is used. The result is
 * palloc'd, so caller is responsible for freeing it.
 */
char *
scram_build_verifier(const char *username, const char *password,
					 int iterations)
{
	uint8		keybuf[SCRAM_KEY_LEN + 1];
	char		storedkey_hex[SCRAM_KEY_LEN * 2 + 1];
	char		serverkey_hex[SCRAM_KEY_LEN * 2 + 1];
	char		salt[SCRAM_SALT_LEN];
	char	   *encoded_salt;
	int			encoded_len;

	if (iterations <= 0)
		iterations = SCRAM_ITERATIONS_DEFAULT;

	generate_nonce(salt, SCRAM_SALT_LEN);

	encoded_salt = palloc(pg_b64_enc_len(SCRAM_SALT_LEN) + 1);
	encoded_len = pg_b64_encode(salt, SCRAM_SALT_LEN, encoded_salt);
	encoded_salt[encoded_len] = '\0';

	/* Calculate StoredKey, and encode it in hex */
	scram_ClientOrServerKey(password, salt, SCRAM_SALT_LEN,
							iterations, SCRAM_CLIENT_KEY_NAME, keybuf);
	scram_H(keybuf, SCRAM_KEY_LEN, keybuf); /* StoredKey */
	(void) hex_encode((const char *) keybuf, SCRAM_KEY_LEN, storedkey_hex);
	storedkey_hex[SCRAM_KEY_LEN * 2] = '\0';

	/* And same for ServerKey */
	scram_ClientOrServerKey(password, salt, SCRAM_SALT_LEN, iterations,
							SCRAM_SERVER_KEY_NAME, keybuf);
	(void) hex_encode((const char *) keybuf, SCRAM_KEY_LEN, serverkey_hex);
	serverkey_hex[SCRAM_KEY_LEN * 2] = '\0';

	return psprintf("%s:%d:%s:%s", encoded_salt, iterations, storedkey_hex, serverkey_hex);
}


/*
 * Check if given verifier can be used for SCRAM authentication.
 *
 * Returns true if it is a SCRAM verifier, and false otherwise.
 */
bool
is_scram_verifier(const char *verifier)
{
	return parse_scram_verifier(verifier, NULL, NULL, NULL, NULL);
}


/*
 * Parse and validate format of given SCRAM verifier.
 *
 * Returns true if the SCRAM verifier has been parsed, and false otherwise.
 */
static bool
parse_scram_verifier(const char *verifier, char **salt, int *iterations,
					 char **stored_key, char **server_key)
{
	char	   *salt_res = NULL;
	char	   *stored_key_res = NULL;
	char	   *server_key_res = NULL;
	char	   *v;
	char	   *p;
	int			iterations_res;

	/*
	 * The verifier is of form:
	 *
	 * salt:iterations:storedkey:serverkey
	 */
	v = pstrdup(verifier);

	/* salt */
	if ((p = strtok(v, ":")) == NULL)
		goto invalid_verifier;
	salt_res = pstrdup(p);

	/* iterations */
	if ((p = strtok(NULL, ":")) == NULL)
		goto invalid_verifier;
	errno = 0;
	iterations_res = strtol(p, &p, SCRAM_ITERATION_LEN);
	if (*p || errno != 0)
		goto invalid_verifier;

	/* storedkey */
	if ((p = strtok(NULL, ":")) == NULL)
		goto invalid_verifier;
	if (strlen(p) != SCRAM_KEY_LEN * 2)
		goto invalid_verifier;

	stored_key_res = (char *) palloc(SCRAM_KEY_LEN);
	hex_decode(p, SCRAM_KEY_LEN * 2, stored_key_res);

	/* serverkey */
	if ((p = strtok(NULL, ":")) == NULL)
		goto invalid_verifier;
	if (strlen(p) != SCRAM_KEY_LEN * 2)
		goto invalid_verifier;
	server_key_res = (char *) palloc(SCRAM_KEY_LEN);
	hex_decode(p, SCRAM_KEY_LEN * 2, server_key_res);

	if (iterations)
		*iterations = iterations_res;
	if (salt)
		*salt = salt_res;
	else
		pfree(salt_res);
	if (stored_key)
		*stored_key = stored_key_res;
	else
		pfree(stored_key_res);
	if (server_key)
		*server_key = server_key_res;
	else
		pfree(server_key_res);
	pfree(v);
	return true;

invalid_verifier:
	if (salt_res)
		pfree(salt_res);
	if (stored_key_res)
		pfree(stored_key_res);
	if (server_key_res)
		pfree(server_key_res);
	pfree(v);
	return false;
}

/*
 * Read the value in a given SASL exchange message for given attribute.
 */
static char *
read_attr_value(char **input, char attr)
{
	char		*begin = *input;
	char		*end;

	if (*begin != attr)
		elog(ERROR, "malformed SCRAM message (%c expected)", attr);
	begin++;

	if (*begin != '=')
		elog(ERROR, "malformed SCRAM message (expected = in attr %c)", attr);
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
 * Read the next attribute and value in a SASL exchange message.
 */
static char *
read_any_attr(char **input, char *attr_p)
{
	char *begin = *input;
	char *end;
	char attr = *begin;

	if (!((attr >= 'A' && attr <= 'Z') ||
		  (attr >= 'a' && attr <= 'z')))
		return NULL;
	if (attr_p)
		*attr_p = attr;
	begin++;

	if (*begin != '=')
		return NULL;
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
 * Read and parse the first message from client in the context of a SASL
 * authentication exchange message.  In the event of an error, returns
 * to caller a e= message to be used for the rest of the exchange, or
 * NULL in case of success.
 */
static SASLStatus
read_client_first_message(scram_state *state, char *input, char **logdetail)
{
	input = pstrdup(input);

	/*
	 * saslname        = 1*(value-safe-char / "=2C" / "=3D")
	 *              ;; Conforms to <value>.
	 *
	 * authzid         = "a=" saslname
	 *              ;; Protocol specific.
	 *
	 * username        = "n=" saslname
	 *               ;; Usernames are prepared using SASLprep.
	 *
	 * gs2-cbind-flag  = ("p=" cb-name) / "n" / "y"
	 *               ;; "n" -> client doesn't support channel binding.
	 *               ;; "y" -> client does support channel binding
	 *               ;;        but thinks the server does not.
	 *               ;; "p" -> client requires channel binding.
	 *               ;; The selected channel binding follows "p=".
	 *
	 * gs2-header      = gs2-cbind-flag "," [ authzid ] ","
	 *               ;; GS2 header for SCRAM
	 *               ;; (the actual GS2 header includes an optional
	 *               ;; flag to indicate that the GSS mechanism is not
	 *               ;; "standard", but since SCRAM is "standard", we
	 *               ;; don't include that flag).
	 *
	 *   client-first-message-bare =
	 *               [reserved-mext ","]
	 *               username "," nonce ["," extensions]
	 *
	 *   client-first-message =
	 *                gs2-header client-first-message-bare
	 *
	 *
	 * For example:
	 * n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
	 */

	/* read gs2-cbind-flag */
	switch (*input)
	{
		case 'n':
			/* client does not support channel binding */
			input++;
			break;
		case 'y':
			/* client supports channel binding, but we're not doing it today */
			input++;
			break;
		case 'p':
			/* client requires channel binding. We don't support it */
			*logdetail = psprintf(_("channel binding not supported."));
			return SASL_CHANNEL_BINDING_NO_SUPPORT;
	}

	/* any mandatory extensions would go here. */
	if (*input != ',')
	{
		*logdetail = psprintf(_("mandatory extension %c not supported."), *input);
		return SASL_OTHER_ERROR;
	}
	input++;

	/* read optional authzid (authorization identity) */
	if (*input != ',')
		state->client_authzid = read_attr_value(&input, 'a');
	else
		input++;

	state->client_first_message_bare = pstrdup(input);

	/* read username */
	state->client_username = read_attr_value(&input, 'n');

	/* read nonce */
	state->client_nonce = read_attr_value(&input, 'r');

	/*
	 * There can be any number of optional extensions after this. We don't
	 * support any extensions, so ignore them.
	 */
	while (*input != '\0')
		read_any_attr(&input, NULL);

	/* success! */
	return SASL_EXCHANGE_CONTINUE;
}

/*
 * Verify the final nonce contained in the last message received from
 * client in an exchange.
 */
static bool
verify_final_nonce(scram_state *state)
{
	int			client_nonce_len = strlen(state->client_nonce);
	int			server_nonce_len = strlen(state->server_nonce);
	int			final_nonce_len = strlen(state->client_final_nonce);

	if (final_nonce_len != client_nonce_len + server_nonce_len)
		return false;
	if (memcmp(state->client_final_nonce, state->client_nonce, client_nonce_len) != 0)
		return false;
	if (memcmp(state->client_final_nonce + client_nonce_len, state->server_nonce, server_nonce_len) != 0)
		return false;

	return true;
}

/*
 * Verify the client proof contained in the last message received from
 * client in an exchange.
 */
static bool
verify_client_proof(scram_state *state)
{
	uint8		ClientSignature[SCRAM_KEY_LEN];
	uint8		ClientKey[SCRAM_KEY_LEN];
	uint8		client_StoredKey[SCRAM_KEY_LEN];
	scram_HMAC_ctx ctx;
	int			i;

	/* calculate ClientSignature */
	scram_HMAC_init(&ctx, state->StoredKey, SCRAM_KEY_LEN);
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
	scram_HMAC_final(ClientSignature, &ctx);
	elog(DEBUG4, "ClientSignature: %02X%02X", ClientSignature[0], ClientSignature[1]);
	elog(DEBUG4, "AuthMessage: %s,%s,%s", state->client_first_message_bare,
		 state->server_first_message, state->client_final_message_without_proof);

	/* Extract the ClientKey that the client calculated from the proof */
	for (i = 0; i < SCRAM_KEY_LEN; i++)
		ClientKey[i] = state->ClientProof[i] ^ ClientSignature[i];

	/* Hash it one more time, and compare with StoredKey */
	scram_H(ClientKey, SCRAM_KEY_LEN, client_StoredKey);
	elog(DEBUG4, "client's ClientKey: %02X%02X", ClientKey[0], ClientKey[1]);
	elog(DEBUG4, "client's StoredKey: %02X%02X", client_StoredKey[0], client_StoredKey[1]);
	elog(DEBUG4, "StoredKey: %02X%02X", state->StoredKey[0], state->StoredKey[1]);

	if (memcmp(client_StoredKey, state->StoredKey, SCRAM_KEY_LEN) != 0)
		return false;

	return true;
}

/*
 * Build the first server-side message sent to the client in a SASL
 * communication exchange.
 */
static char *
build_server_first_message(scram_state *state)
{
	char		nonce[SCRAM_NONCE_LEN];
	int			encoded_len;

	/*
	 * server-first-message =
	 *                   [reserved-mext ","] nonce "," salt ","
	 *                   iteration-count ["," extensions]
	 *
	 *   nonce           = "r=" c-nonce [s-nonce]
	 *               ;; Second part provided by server.
	 *
	 * c-nonce         = printable
	 *
	 * s-nonce         = printable
	 *
	 * salt            = "s=" base64
	 *
	 * iteration-count = "i=" posit-number
	 *              ;; A positive number.
	 *
	 * Example:
	 *
	 * r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=4096
	 */
	generate_nonce(nonce, SCRAM_NONCE_LEN);

	state->server_nonce = palloc(pg_b64_enc_len(SCRAM_NONCE_LEN) + 1);
	encoded_len = pg_b64_encode(nonce, SCRAM_NONCE_LEN, state->server_nonce);

	if (encoded_len < 0)
		return NULL;

	state->server_nonce[encoded_len] = '\0';
	state->server_first_message =
		psprintf("r=%s%s,s=%s,i=%u",
				 state->client_nonce, state->server_nonce,
				 state->salt, state->iterations);

	return state->server_first_message;
}


/*
 * Read and parse the final message received from client.
 */
static SASLStatus
read_client_final_message(scram_state *state, char *input, char **logdetail)
{
	char		attr;
	char	   *channel_binding;
	char	   *value;
	char	   *begin, *proof;
	char	   *p;
	char	   *client_proof;

	begin = p = pstrdup(input);

	/*
	 *
	 * cbind-input   = gs2-header [ cbind-data ]
	 *               ;; cbind-data MUST be present for
	 *               ;; gs2-cbind-flag of "p" and MUST be absent
	 *               ;; for "y" or "n".
	 *
	 * channel-binding = "c=" base64
	 *               ;; base64 encoding of cbind-input.
	 *
	 * proof           = "p=" base64
	 *
	 * client-final-message-without-proof =
	 *               channel-binding "," nonce ["," extensions]
	 *
	 * client-final-message =
	 *              client-final-message-without-proof "," proof
	 */
	channel_binding = read_attr_value(&p, 'c');
	if (strcmp(channel_binding, "biws") != 0)
	{
		*logdetail = psprintf(_("invalid channel binding input."));
		return SASL_CHANNEL_BINDING_TYPE_NOT_SUPPORTED;
	}
	state->client_final_nonce = read_attr_value(&p, 'r');

	/* ignore optional extensions */
	do
	{
		proof = p - 1;
		value = read_any_attr(&p, &attr);
	} while (attr != 'p');

	client_proof = palloc(pg_b64_dec_len(strlen(value)));
	if (pg_b64_decode(value, strlen(value), client_proof) != SCRAM_KEY_LEN)
	{
		*logdetail = psprintf(_("invalid client proof."));
		return SASL_INVALID_PROOF;
	}
	memcpy(state->ClientProof, client_proof, SCRAM_KEY_LEN);
	pfree(client_proof);

	if (*p != '\0')
	{
		*logdetail = psprintf(_("malformed SCRAM message (garbage at end of message %c)."),
							  attr);
		return SASL_OTHER_ERROR;
	}

	state->client_final_message_without_proof = palloc(proof - begin + 1);
	memcpy(state->client_final_message_without_proof, input, proof - begin);
	state->client_final_message_without_proof[proof - begin] = '\0';

	/* XXX: check channel_binding field if support is added */
	return SASL_NO_ERROR;
}

/*
 * Build the final server-side message of an exchange.
 */
static char *
build_server_final_message(scram_state *state)
{
	uint8		ServerSignature[SCRAM_KEY_LEN];
	char	   *server_signature_base64;
	int			siglen;
	scram_HMAC_ctx ctx;

	/* calculate ServerSignature */
	scram_HMAC_init(&ctx, state->ServerKey, SCRAM_KEY_LEN);
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

	server_signature_base64 = palloc(pg_b64_enc_len(SCRAM_KEY_LEN) + 1);
	siglen = pg_b64_encode((const char *) ServerSignature,
						   SCRAM_KEY_LEN, server_signature_base64);
	if (siglen < 0)
		return NULL;
	server_signature_base64[siglen] = '\0';

	/*
	 * The following error is generated:
	 *
	 * verifier        = "v=" base64
	 *               ;; base-64 encoded ServerSignature.
	 *
	 * server-final-message = (server-error / verifier)
	 *                ["," extensions]
	 */
	return psprintf("v=%s", server_signature_base64);
}

static void
generate_nonce(char *result, int len)
{
	/* Use the salt generated for SASL authentication */
	memset(result, 0, len);
	memcpy(result, MyProcPort->SASLSalt, Min(sizeof(MyProcPort->SASLSalt), len));
}
