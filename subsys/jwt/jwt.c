/*
 * Copyright (C) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <errno.h>

#include <zephyr/jwt.h>
#include <json.h>

#ifdef CONFIG_JWT_SIGN_RSA
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>
#endif

#ifdef CONFIG_JWT_SIGN_ECDSA
#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/sha256.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/constants.h>

#include <random/rand32.h>
#endif

/*
 * Base-64 encoding is typically done by lookup into a 64-byte static
 * array.  As an experiment, lets look at both code size and time for
 * one that does the character encoding computationally.  Like the
 * array version, this doesn't do bounds checking, and assumes the
 * passed value has been masked.
 *
 * On Cortex-M, this function is 34 bytes of code, which is only a
 * little more than half of the size of the lookup table.
 */
#if 1
static int base64_char(int value)
{
	if (value < 26) {
		return value + 'A';
	} else if (value < 52) {
		return value + 'a' - 26;
	} else if (value < 62) {
		return value + '0' - 52;
	} else if (value == 62) {
		return '-';
	} else {
		return '_';
	}
}
#else
static const char b64_table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static inline int base64_char(int value)
{
	return b64_table[value];
}
#endif

/*
 * Add a single character to the jwt buffer.  Detects overflow, and
 * always keeps the buffer null terminated.
 */
static void base64_outch(struct jwt_builder *st, char ch)
{
	if (st->overflowed) {
		return;
	}

	if (st->len < 2) {
		st->overflowed = true;
		return;
	}

	*st->buf++ = ch;
	st->len--;
	*st->buf = 0;
}

/*
 * Flush any pending base64 character data out.  If we have all three
 * bytes are present, this will generate 4 characters, otherwise it
 * may generate fewer.
 */
static void base64_flush(struct jwt_builder *st)
{
	if (st->pending < 1) {
		return;
	}

	base64_outch(st, base64_char(st->wip[0] >> 2));
	base64_outch(st, base64_char(((st->wip[0] & 0x03) << 4) |
				(st->wip[1] >> 4)));

	if (st->pending >= 2) {
		base64_outch(st, base64_char(((st->wip[1] & 0x0f) << 2) |
				(st->wip[2] >> 6)));
	}
	if (st->pending >= 3) {
		base64_outch(st, base64_char(st->wip[2] & 0x3f));
	}

	st->pending = 0;
	memset(st->wip, 0, 3);
}

static void base64_addbyte(struct jwt_builder *st, uint8_t byte)
{
	st->wip[st->pending++] = byte;
	if (st->pending == 3) {
		base64_flush(st);
	}
}

static int base64_append_bytes(const char *bytes, size_t len,
			 void *data)
{
	struct jwt_builder *st = data;

	while (len-- > 0) {
		base64_addbyte(st, *bytes++);
	}

	return 0;
}

struct jwt_header {
	char *typ;
	char *alg;
};

static struct json_obj_descr jwt_header_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_header, alg, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_header, typ, JSON_TOK_STRING),
};

struct jwt_payload {
	s32_t exp;
	s32_t iat;
	const char *aud;
};

static struct json_obj_descr jwt_payload_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, aud, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, exp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct jwt_payload, iat, JSON_TOK_NUMBER),
};

/*
 * Add the JWT header to the buffer.
 */
static void jwt_add_header(struct jwt_builder *builder)
{
	static const struct jwt_header head = {
		.typ = "JWT",
#ifdef CONFIG_JWT_SIGN_RSA
		.alg = "RS256",
#endif
#ifdef CONFIG_JWT_SIGN_ECDSA
		.alg = "ES256",
#endif
	};

	int res = json_obj_encode(jwt_header_desc, ARRAY_SIZE(jwt_header_desc),
				  &head, base64_append_bytes, builder);
	if (res != 0) {
		/* Log an error here. */
		return;
	}
	base64_flush(builder);
}

int jwt_add_payload(struct jwt_builder *builder,
		     s32_t exp,
		     s32_t iat,
		     const char *aud)
{
	struct jwt_payload payload = {
		.exp = exp,
		.iat = iat,
		.aud = aud,
	};

	base64_outch(builder, '.');
	int res = json_obj_encode(jwt_payload_desc,
				  ARRAY_SIZE(jwt_payload_desc),
				  &payload, base64_append_bytes, builder);

	base64_flush(builder);
	return res;
}

#ifdef CONFIG_JWT_SIGN_RSA
int jwt_sign(struct jwt_builder *builder,
	     const char *der_key,
	     size_t der_key_len)
{
	int res;
	mbedtls_pk_context ctx;

	mbedtls_pk_init(&ctx);

	res = mbedtls_pk_parse_key(&ctx, der_key, der_key_len,
				       NULL, 0);
	if (res != 0) {
		return res;
	}

	u8_t hash[32], sig[256];
	size_t sig_len = sizeof(sig);

	/*
	 * The '0' indicates to mbedtls to do a SHA256, instead of
	 * 224.
	 */
	mbedtls_sha256(builder->base, builder->buf - builder->base,
		       hash, 0);

	res = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256,
			      hash, sizeof(hash),
			      sig, &sig_len,
			      NULL, NULL);
	if (res != 0) {
		return res;
	}

	base64_outch(builder, '.');
	base64_append_bytes(sig, sig_len, builder);
	base64_flush(builder);

	return builder->overflowed ? -ENOMEM : 0;
}
#endif

#ifdef CONFIG_JWT_SIGN_ECDSA
static TCCtrPrng_t prng_state;
static bool prng_init;

static const char personalize[] = "zephyr:drivers/jwt/jwt.c";

static int setup_prng(void)
{
	if (prng_init) {
		return 0;
	}
	prng_init = true;

	u8_t entropy[TC_AES_KEY_SIZE + TC_AES_BLOCK_SIZE];

	for (int i = 0; i < sizeof(entropy); i += sizeof(u32_t)) {
		u32_t rv = sys_rand32_get();

		memcpy(entropy + i, &rv, sizeof(uint32_t));
	}

	int res = tc_ctr_prng_init(&prng_state,
				   (const uint8_t *) &entropy, sizeof(entropy),
				   personalize,
				   sizeof(personalize));

	return res == TC_CRYPTO_SUCCESS ? 0 : -EINVAL;
}

int default_CSPRNG(u8_t *dest, unsigned int size)
{
	int res = tc_ctr_prng_generate(&prng_state, NULL, 0, dest, size);
	return res;
}

int jwt_sign(struct jwt_builder *builder,
	     const char *der_key,
	     size_t der_key_len)
{
	struct tc_sha256_state_struct ctx;
	u8_t hash[32], sig[64];
	int res;

	tc_sha256_init(&ctx);
	tc_sha256_update(&ctx, builder->base, builder->buf - builder->base);
	tc_sha256_final(hash, &ctx);

	res = setup_prng();

	if (res != 0) {
		return res;
	}
	uECC_set_rng(&default_CSPRNG);

	/* Note that tinycrypt only supports P-256. */
	res = uECC_sign(der_key, hash, sizeof(hash),
			sig, &curve_secp256r1);
	if (res != TC_CRYPTO_SUCCESS) {
		return -EINVAL;
	}

	base64_outch(builder, '.');
	base64_append_bytes(sig, sizeof(sig), builder);
	base64_flush(builder);

	return 0;
}
#endif

int jwt_init_builder(struct jwt_builder *builder,
		     char *buffer,
		     size_t buffer_size)
{
	builder->base = buffer;
	builder->buf = buffer;
	builder->len = buffer_size;
	builder->overflowed = false;
	builder->pending = 0;

	jwt_add_header(builder);

	return 0;
}
