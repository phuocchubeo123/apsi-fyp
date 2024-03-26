#ifndef CRYPTO_H_
#define CRYPTO_H_

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "apsi/util/typedefs.h"

#define AES_BYTES 16
#define AES_BITS AES_BYTES*8

#define SHA256_OUT_BYTES 32

//Check for the OpenSSL version number, since the EVP_CIPHER_CTX has become opaque from >= 1.1.0
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	#define OPENSSL_OPAQUE_EVP_CIPHER_CTX
#endif

#ifdef OPENSSL_OPAQUE_EVP_CIPHER_CTX
typedef EVP_CIPHER_CTX* AES_KEY_CTX;
#else
typedef EVP_CIPHER_CTX AES_KEY_CTX;
#endif

struct prf_state_ctx {
	AES_KEY_CTX aes_key;
#ifdef AES256_HASH
	ROUND_KEYS aes_pipe_key;
#endif
	uint64_t* ctr;
};

namespace apsi{
	namespace hashing{
		uint32_t sha256_hash(uint32_t* inbuf, uint32_t inbitlen, uint32_t outbitlen);
	}
}

#endif