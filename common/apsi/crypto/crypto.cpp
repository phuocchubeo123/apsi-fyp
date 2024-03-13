/*
 * crypto.cpp
 *
 *  Created on: Jul 9, 2014
 *      Author: mzohner
 * github: https://github.com/encryptogroup/PSI.git
 */


#include "crypto.h"

void gen_rnd_bytes(prf_state_ctx* prf_state, uint8_t* resbuf, uint32_t nbytes) {
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

	memcpy(resbuf, tmpbuf, nbytes);

	free(tmpbuf);
}