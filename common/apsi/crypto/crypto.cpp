/*
 * crypto.cpp
 *
 *  Created on: Jul 9, 2014
 *      Author: mzohner
 * github: https://github.com/encryptogroup/PSI.git
 */


#include "crypto.h"

namespace apsi{
	namespace hashing{
		uint32_t gen_rnd_bytes(prf_state_ctx* prf_state, uint32_t nbytes) {
			AES_KEY_CTX* aes_key;
			uint64_t* rndctr;
			uint8_t* tmpbuf;
			uint32_t i, size;
			int32_t dummy;

			aes_key = &(prf_state->aes_key);
			rndctr = prf_state->ctr;
			size = ceil_divide(nbytes, AES_BYTES);
			tmpbuf = (uint8_t*) malloc(sizeof(uint8_t) * size * AES_BYTES);

			//TODO it might be better to store the result directly in resbuf but this would require the invoking routine to pad it to a multiple of AES_BYTES
			for(i = 0; i < size; i++, rndctr[0]++)	{
				EVP_EncryptUpdate(*aes_key, tmpbuf + i * AES_BYTES, &dummy, (uint8_t*) rndctr, AES_BYTES);
			}

			uint32_t* resbuf;

			memcpy(resbuf, tmpbuf, nbytes);

			free(tmpbuf);
			return *resbuf;
		}

		uint32_t sha256_hash(uint32_t inbuf, uint32_t inbitlen, uint32_t outbitlen) {
			SHA256_CTX sha;
			SHA256_Init(&sha);
			SHA256_Update(&sha, &inbuf, (inbitlen + 7) / 8);

			uint8_t* hash_buf;
			SHA256_Final(hash_buf, &sha);

			uint32_t result;
			for (int i = 0; i < outbitlen; i++){
				result &= ((hash_buf[i >> 3] >> (i & 7) & 1) << i);
			}
			return result;
		}
	}
}

