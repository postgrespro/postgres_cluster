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
 *	 should send an empty string as the username.
 * - Password is not processed with the SASLprep algorithm.
 * - Channel binding is not supported yet.
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
#include "common/encode_utils.h"
#include "common/scram-common.h"
#include "common/sha.h"
#include "libpq/auth.h"
#include "libpq/crypt.h"
#include "libpq/scram.h"
#include "miscadmin.h"
#include "utils/builtins.h"

typedef enum
{
	INIT,			/* receive client-first-message */
	SALT_SENT,		/* receive client-final-message */
	FINISHED		/* all done */
} scram_exchange_state;

typedef struct
{
	scram_exchange_state state;

	/* Username from startup packet */
	const char *username;

	/* Fields from the password verifier, stored in pg_authid. */
	char	   *salt;			/* base64-encoded */
	int			iterations;
	uint8		StoredKey[SCRAM_KEY_LEN];
	uint8		ServerKey[SCRAM_KEY_LEN];

	/* Fields from the first message from client */
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
	char	   *server_nonce;	/* base64-encoded */
	char	   *server_signature;

} scram_state;

static void read_client_first_message(scram_state *state, char *input);
static void read_client_final_message(scram_state *state, char *input);
static char *build_server_first_message(scram_state *state);
static char *build_server_final_message(scram_state *state);
static bool verify_client_proof(scram_state *state);
static bool verify_final_nonce(scram_state *state);

static bool parse_scram_verifier(const char *verifier, char **salt,
					 int *iterations, char **stored_key, char **server_key);

/*
 * Initialize a new SCRAM authentication exchange, with given username and
 * its stored verifier.
 */
void *
scram_init(const char *username, const char *verifier)
{
	scram_state *state;
	char	   *server_key;
	char	   *stored_key;
	char	   *salt;
	int			iterations;


	state = (scram_state *) palloc0(sizeof(scram_state));
	state->state = INIT;
	state->username = username;

	if (!parse_scram_verifier(verifier, &salt, &iterations,
							  &stored_key, &server_key))
		elog(ERROR, "invalid SCRAM verifier");

	state->salt = salt;
	state->iterations = iterations;
	memcpy(state->ServerKey, server_key, SCRAM_KEY_LEN);
	memcpy(state->StoredKey, stored_key, SCRAM_KEY_LEN);
	pfree(stored_key);
	pfree(server_key);
	return state;
}

/*
 * Continue a SCRAM authentication exchange.
 */
int
scram_exchange(void *opaq,
			   char *input, int inputlen,
			   char **output, int *outputlen)
{
	scram_state *state = (scram_state *) opaq;
	int			result;

	*output = NULL;
	*outputlen = 0;

	switch (state->state)
	{
		case INIT:
			/* receive username and client nonce, send challenge */
			read_client_first_message(state, input);
			*output = build_server_first_message(state);
			*outputlen = strlen(*output);
			result = SASL_EXCHANGE_CONTINUE;
			state->state = SALT_SENT;
			break;

		case SALT_SENT:
			/* receive response to challenge and verify it */
			read_client_final_message(state, input);
			if (verify_final_nonce(state) && verify_client_proof(state))
			{
				*output = build_server_final_message(state);
				*outputlen = strlen(*output);
				result = SASL_EXCHANGE_SUCCESS;
			}
			else
			{
				result = SASL_EXCHANGE_FAILURE;
			}
			state->state = FINISHED;
			break;

		default:
			elog(ERROR, "invalid SCRAM exchange state");
			result = 0;
	}

	return result;
}

/*
 * Read the value in a given SASL exchange message for given attribute.
 */
static char *
read_attr_value(char **input, char attr)
{
	char	   *begin = *input;
	char	   *end;

	if (*begin != attr)
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (attr %c expected)", attr)));
	begin++;

	if (*begin != '=')
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (expected '=' in attr %c)", attr)));
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
	char	   *begin = *input;
	char	   *end;
	char		attr = *begin;

	if (!((attr >= 'A' && attr <= 'Z') ||
		  (attr >= 'a' && attr <= 'z')))
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (invalid attribute char)")));
	if (attr_p)
		*attr_p = attr;
	begin++;

	if (*begin != '=')
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (expected '=' in attr %c)", attr)));
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
 * authentication exchange message.
 */
static void
read_client_first_message(scram_state *state, char *input)
{
	input = pstrdup(input);

	/*----------
	 * According to RFC 5820, the syntax for the client's first
	 * message is:
	 *
	 * saslname        = 1*(value-safe-char / "=2C" / "=3D")
	 *                   ;; Conforms to <value>.
	 *
	 * authzid         = "a=" saslname
	 *                   ;; Protocol specific.
	 *
	 * username        = "n=" saslname
	 *                   ;; Usernames are prepared using SASLprep.
	 *
	 * reserved-mext   = "m=" 1*(value-char)
	 *                   ;; Reserved for signaling mandatory extensions.
	 *                   ;; The exact syntax will be defined in
	 *                   ;; the future.
	 *
	 * gs2-cbind-flag  = ("p=" cb-name) / "n" / "y"
	 *                   ;; "n" -> client doesn't support channel binding.
	 *                   ;; "y" -> client does support channel binding
	 *                   ;;        but thinks the server does not.
	 *                   ;; "p" -> client requires channel binding.
	 *                   ;; The selected channel binding follows "p=".
	 *
	 * gs2-header      = gs2-cbind-flag "," [ authzid ] ","
	 *                   ;; GS2 header for SCRAM
	 *                   ;; (the actual GS2 header includes an optional
	 *                   ;; flag to indicate that the GSS mechanism is not
	 *                   ;; "standard", but since SCRAM is "standard", we
	 *                   ;; don't include that flag).
	 *
	 * client-first-message-bare =
	 *               [reserved-mext ","]
	 *               username "," nonce ["," extensions]
	 *
	 * client-first-message =
	 *                gs2-header client-first-message-bare
	 *
	 * Note: Contrary to the spec, the username in the SASL exchange
	 * is always sent as an empty string, because we get the username
	 * from the startup packet.
	 *
	 * For example:
	 * n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL
	 *----------
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
			ereport(FATAL,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("channel binding not supported")));
		default:
			ereport(COMMERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("malformed SCRAM request")));
	}
	if (*input != ',')
		ereport(FATAL,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("malformed SCRAM request (',' expected after gs2-cbind-flag)")));
	input++;

	/* read optional authzid (authorization identity) */
	if (*input != ',')
	{
		state->client_authzid = read_attr_value(&input, 'a');
		ereport(FATAL,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("SCRAM authorization identity not supported")));
	}
	else
		input++;

	/*
	 * We're now at the beginning of 'client_first_message_bare'. Save it,
	 * because it's needed later in the verification of client-proof.
	 */
	state->client_first_message_bare = pstrdup(input);

	/*
	 * Any mandatory extensions would go here. We don't support any, so
	 * throw an error if we see one.
	 */
	if (*input == 'm')
		ereport(FATAL,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("SCRAM mandatory extension not supported")));

	/* read username and nonce */
	/* FIXME: Should we check that the username is empty? */
	state->client_username = read_attr_value(&input, 'n');
	state->client_nonce = read_attr_value(&input, 'r');

	/*
	 * There can be any number of optional extensions after this. We don't
	 * support any extensions, so ignore them.
	 */
	while (*input != '\0')
		read_any_attr(&input, NULL);

	/* success! */
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

	/* Extract the ClientKey that the client calculated from the proof */
	for (i = 0; i < SCRAM_KEY_LEN; i++)
		ClientKey[i] = state->ClientProof[i] ^ ClientSignature[i];

	/* Hash it one more time, and compare with StoredKey */
	scram_H(ClientKey, SCRAM_KEY_LEN, client_StoredKey);

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

	/*----------
	 * According to RFC 5820, the syntax of the server-first-message is:
	 *
	 * server-first-message =
	 *                   [reserved-mext ","] nonce "," salt ","
	 *                   iteration-count ["," extensions]
	 *
	 * nonce           = "r=" c-nonce [s-nonce]
	 *                   ;; Second part provided by server.
	 *
	 * c-nonce         = printable
	 *
	 * s-nonce         = printable
	 *
	 * salt            = "s=" base64
	 *
	 * iteration-count = "i=" posit-number
	 *                   ;; A positive number.
	 *
	 * Example:
	 * r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,s=QSXCR+Q6sek8bf92,i=4096
	 *----------
	 */

	/* Use the nonce that was pre-generated before forking */
	/* FIXME: use RAND_bytes() here if built with OpenSSL? */
	memcpy(nonce, MyProcPort->scramNonce, SCRAM_NONCE_LEN);

	state->server_nonce = palloc(b64_enc_len(nonce, SCRAM_NONCE_LEN) + 1);
	encoded_len = b64_encode(nonce, SCRAM_NONCE_LEN, state->server_nonce);

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
static void
read_client_final_message(scram_state *state, char *input)
{
	char		attr;
	char	   *channel_binding;
	char	   *value;
	char	   *begin,
			   *proof;
	char	   *p;
	char	   *client_proof;

	begin = p = pstrdup(input);

	/*----------
	 * According to RFC 5820, the syntax of the client-final-message is:
	 *
	 * cbind-input   = gs2-header [ cbind-data ]
	 *                 ;; cbind-data MUST be present for
	 *                 ;; gs2-cbind-flag of "p" and MUST be absent
	 *                 ;; for "y" or "n".
	 *
	 * channel-binding = "c=" base64
	 *                   ;; base64 encoding of cbind-input.
	 *
	 * proof           = "p=" base64
	 *
	 * client-final-message-without-proof =
	 *               channel-binding "," nonce ["," extensions]
	 *
	 * client-final-message =
	 *              client-final-message-without-proof "," proof
	 *----------
	 */

	/*
	 * Since we don't support channel binding, nor authorization identity,
	 * cbind-input is always "n,,". That's "biws" when base64-encoded.
	 */
	channel_binding = read_attr_value(&p, 'c');
	if (strcmp(channel_binding, "biws") != 0)
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("invalid channel binding input in SCRAM message")));
	state->client_final_nonce = read_attr_value(&p, 'r');

	/* ignore optional extensions */
	do
	{
		proof = p - 1;
		value = read_any_attr(&p, &attr);
	} while (attr != 'p');

	client_proof = palloc(b64_dec_len(value, strlen(value)));
	if (b64_decode(value, strlen(value), client_proof) != SCRAM_KEY_LEN)
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (invalid ClientProof)")));
	memcpy(state->ClientProof, client_proof, SCRAM_KEY_LEN);
	pfree(client_proof);

	if (*p != '\0')
		ereport(COMMERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
				 errmsg("malformed SCRAM message (unexpected attribute %c at end of message)", attr)));

	state->client_final_message_without_proof = palloc(proof - begin + 1);
	memcpy(state->client_final_message_without_proof, input, proof - begin);
	state->client_final_message_without_proof[proof - begin] = '\0';

	/* XXX: check channel_binding field if support is added */
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

	server_signature_base64 = palloc(b64_enc_len((const char *) ServerSignature,
												 SCRAM_KEY_LEN) + 1);
	siglen = b64_encode((const char *) ServerSignature,
						SCRAM_KEY_LEN, server_signature_base64);
	server_signature_base64[siglen] = '\0';

	/*----------
	 * According to RFC 5820, the syntax for the server-final message is:
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
	 *
	 * verifier        = "v=" base64
	 *               ;; base-64 encoded ServerSignature.
	 *
	 * server-final-message = (server-error / verifier)
	 *                ["," extensions]
	 *----------
	 */
	return psprintf("v=%s", server_signature_base64);
}

/*
 * Functions for serializing/deserializing SCRAM password verifiers in
 * pg_authid.
 *
 * The password stored in pg_authid consists of the salt, iteration count,
 * StoredKey and ServerKey.
 */

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
	int			i;

	if (iterations <= 0)
		iterations = SCRAM_ITERATIONS_DEFAULT;

	/* FIXME: use RAND_bytes() here if built with OpenSSL? */
	for (i = 0; i < SCRAM_SALT_LEN; i++)
		salt[i] = random() % 255 + 1;

	encoded_salt = palloc(b64_enc_len(salt, SCRAM_SALT_LEN) + 1);
	encoded_len = b64_encode(salt, SCRAM_SALT_LEN, encoded_salt);
	encoded_salt[encoded_len] = '\0';

	/* Calculate StoredKey, and encode it in hex */
	scram_ClientOrServerKey(password, salt, SCRAM_SALT_LEN,
							iterations, SCRAM_CLIENT_KEY_NAME, keybuf);
	scram_H(keybuf, SCRAM_KEY_LEN, keybuf);		/* StoredKey */
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
 * Returns true if it is a SCRAM verifier, and false otherwise.
 */
bool
is_scram_verifier(const char *verifier)
{
	return parse_scram_verifier(verifier, NULL, NULL, NULL, NULL);
}

/*
 * Parse and validate format of given SCRAM verifier.
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
	iterations_res = strtol(p, &p, 10);
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
