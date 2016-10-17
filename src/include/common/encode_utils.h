/*
 *	encode_utils.h
 *		Encoding and decoding routines for base64, hexadecimal and escape.
 *
 *	Portions Copyright (c) 2001-2016, PostgreSQL Global Development Group
 *
 *	src/include/common/encode_utils.h
 */
#ifndef ENCODE_UTILS_H
#define ENCODE_UTILS_H

/* base 64 */
unsigned b64_encode(const char *src, unsigned len, char *dst);
unsigned b64_decode(const char *src, unsigned len, char *dst);
unsigned b64_enc_len(const char *src, unsigned srclen);
unsigned b64_dec_len(const char *src, unsigned srclen);

/* hex */
unsigned hex_encode(const char *src, unsigned len, char *dst);
unsigned hex_decode(const char *src, unsigned len, char *dst);
unsigned hex_enc_len(const char *src, unsigned srclen);
unsigned hex_dec_len(const char *src, unsigned srclen);

/* escape */
unsigned esc_encode(const char *src, unsigned srclen, char *dst);
unsigned esc_decode(const char *src, unsigned srclen, char *dst);
unsigned esc_enc_len(const char *src, unsigned srclen);
unsigned esc_dec_len(const char *src, unsigned srclen);

#endif   /* ENCODE_UTILS_H */
