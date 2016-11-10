/*-------------------------------------------------------------------------
 *
 * pg_strong_random.c
 *	  pg_strong_random() function to return a strong random number
 *
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/port/pg_strong_random.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include "common/sha2.h"

#include <fcntl.h>
#include <unistd.h>

#ifdef USE_SSL
#include <openssl/rand.h>
#endif
#ifdef WIN32
#include <Wincrypt.h>
#endif

static bool random_from_file(char *filename, void *buf, size_t len);
#ifndef WIN32
static bool random_from_std(void *dst, size_t len);
#endif

#ifdef WIN32
/*
 * Cache a global crypto provider that only gets freed when the process
 * exits, in case we need random numbers more than once.
 */
static HCRYPTPROV hProvider = 0;
#endif

/*
 * Read (random) bytes from a file.
 */
static bool
random_from_file(char *filename, void *buf, size_t len)
{
	int			f;
	char	   *p = buf;
	ssize_t		res;

	f = open(filename, O_RDONLY, 0);
	if (f == -1)
		return false;

	while (len)
	{
		res = read(f, p, len);
		if (res <= 0)
		{
			if (errno == EINTR)
				continue;		/* interrupted by signal, just retry */

			close(f);
			return false;
		}

		p += res;
		len -= res;
	}

	close(f);
	return true;
}

#ifndef WIN32
/*
 * This is not available on Windows, and the cryptography standards available
 * there are enough anyway to generate randomness.
 */
#include <sys/time.h>
#include <time.h>

/*
 * Generate some random data using environment data as entropy
 * through SHA-256.  All the other methods failed, this can be used
 * as a last resort method even if it is predictible, and rather
 * slow compared to the rest.
 */
static bool
random_from_std(void *dst, size_t len)
{
	pid_t	pid;
	size_t	remaining = len;

	/* process ID */
	pid = getpid();

	/*
	 * Compute a random value by generating successive chunks worth
	 * of PG_SHA256_DIGEST_LENGTH bytes.
	 */
	while (remaining != 0)
	{
		int				x;
		struct timeval	tv;
		pg_sha256_ctx	ctx;
		uint8			sha_buf[PG_SHA256_DIGEST_LENGTH];

		pg_sha256_init(&ctx);
		pg_sha256_update(&ctx, (uint8 *) &pid, sizeof(pid));

		/* time */
		gettimeofday(&tv, NULL);
		pg_sha256_update(&ctx, (uint8 *) &tv, sizeof(tv));

		/* pointless, but should not hurt */
		x = random();
		pg_sha256_update(&ctx, (uint8 *) &x, sizeof(x));
		pg_sha256_final(&ctx, sha_buf);

		/* copy the chunk generated in the result */
		memcpy(dst, sha_buf, Min(remaining, PG_SHA256_DIGEST_LENGTH));

		/* and prepare to loop further */
		if (remaining >= PG_SHA256_DIGEST_LENGTH)
			remaining -= PG_SHA256_DIGEST_LENGTH;
		else
			remaining = 0;
		dst = (uint8 *) dst + PG_SHA256_DIGEST_LENGTH;
	}

	return true;
}
#endif

/*
 * pg_strong_random
 *
 * Generate requested number of random bytes. The bytes are
 * cryptographically strong random, suitable for use e.g. in key
 * generation.
 *
 * The bytes can be acquired from a number of sources, depending
 * on what's available. We try the following, in this order:
 *
 * 1. OpenSSL's RAND_bytes()
 * 2. Windows' CryptGenRandom() function
 * 3. /dev/urandom
 * 4. /dev/random
 * 5. Use the internal, predictible method computing a value through SHA-256
 *	  with system-related entropy.
 *
 * Returns true on success, and false if none of the sources
 * were available. NB: It is important to check the return value!
 * Proceeding with key generation when no random data was available
 * would lead to predictable keys and security issues.
 */
bool
pg_strong_random(void *buf, size_t len)
{
#ifdef USE_SSL

	/*
	 * When built with OpenSSL, first try the random generation function from
	 * there.
	 */
	if (RAND_bytes(buf, len) == 1)
		return true;
#endif

#ifdef WIN32

	/*
	 * Windows has CryptoAPI for strong cryptographic numbers.
	 */
	if (hProvider == 0)
	{
		if (!CryptAcquireContext(&hProvider,
								 NULL,
								 MS_DEF_PROV,
								 PROV_RSA_FULL,
								 CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		{
			/*
			 * On failure, set back to 0 in case the value was for some reason
			 * modified.
			 */
			hProvider = 0;
		}
	}

	/* Re-check in case we just retrieved the provider */
	if (hProvider != 0)
	{
		if (CryptGenRandom(hProvider, len, buf))
			return true;
	}
#endif

	/*
	 * If there is no OpenSSL and no CryptoAPI (or they didn't work), then
	 * fall back on reading /dev/urandom or even /dev/random.
	 */
	if (random_from_file("/dev/urandom", buf, len))
		return true;
	if (random_from_file("/dev/random", buf, len))
		return true;

#ifndef WIN32
	/*
	 * As final method, generate some randomness using the in-house method.
	 */
	if (random_from_std(buf, len))
		return true;
#endif

	/* None of the sources were available. */
	return false;
}
