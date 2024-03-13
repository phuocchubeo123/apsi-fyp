/*
 * hashing_util.h
 *
 *  Created on: Oct 8, 2014
 *      Author: mzohner
 * github: https://github.com/encryptogroup/PSI.git
 */



#ifndef HASHING_UTIL_H_
#define HASHING_UTIL_H_

typedef uint16_t TABLEID_T; // the datatype to store table address

#define NUM_HASH_FUNCTIONS 3

#define MAX_TABLE_SIZE_BYTES sizeof(TABLEID_T);
#define DUMMY_ENTRY_SERVER 0x00
#define DUMMY_ENTRY_CLIENT 0xFF

typedef struct hashing_state_ctx {
	uint32_t** hf_values[NUM_HASH_FUNCTIONS]; // hash function values
	uint32_t nhfvals; // the number of possible hash function values
	uint32_t nelements; 
	uint32_t nbins; // number of bins
	uint32_t inbitlen; // bit length of input
	uint32_t addrbitlen; // bit length of address (which bin it is in?)
	uint32_t floor_addrbitlen; 
	uint32_t outbitlen; // bit length of output
	//the byte values, are stored separately since they are needed very often
	uint32_t inbytelen;
	uint32_t addrbytelen;
	uint32_t outbytelen;
	uint32_t* address_used;
	uint32_t mask;
} hs_t;

//TODO: generate these randomly for each execution and communicate them between the parties
static const uint32_t HF_MASKS[3] = {0x00000000, 0x33333333, 0x14894568};

// use as mask to address the bits in a uint32_t vector
// kinda like: 00001, 00011, 00111, 01111, 11111
static const uint32_t SELECT_BITS[33] = \
									{0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F, \
									 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, \
									 0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF, \
									 0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, \
									 0xFFFFFFFF };

//can also be computed as SELECT_BITS ^ 0xFFFFFFFF
static const uint32_t SELECT_BITS_INV[33] = \
									{0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0, 0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80, \
									 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800, 0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000, \
									 0xFFFF0000, 0xFFFE0000, 0xFFFC0000, 0xFFF80000, 0xFFF00000, 0xFFE00000, 0xFFC00000, 0xFF800000, \
									 0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000, 0xF0000000, 0xE0000000, 0xC0000000, 0x80000000, \
									 0x00000000 };

static const uint8_t BYTE_SELECT_BITS_INV[8] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};

/**
 * @brief Init the values for the hash function
 * 
 * @param hs Pointer to the hashing state object to be initialized.
 * @param nelements Number of elements in the hashing state.
 * @param inbitlen Input bit length for hashing.
 * @param nbins Number of bins for the hashing state.
 * @param prf_state Pointer to the PRF (Pseudorandom Function) state context.
 
*/
static void init_hashing_state(hs_t* hs, uint32_t nelements, uint32_t inbitlen, uint32_t nbins,
		prf_state_ctx* prf_state) {
	uint32_t i, j, nrndbytes;
	hs->nelements = nelements;
	hs->nbins = nbins;

	hs->inbitlen = inbitlen;
	hs->addrbitlen = min((uint32_t) ceil_log2(nbins), inbitlen);
	hs->floor_addrbitlen = min((uint32_t) floor_log2(nbins), inbitlen);

    // Luby-rackoff encryption a.k.a point-and-permute
	hs->outbitlen = hs->inbitlen - hs->addrbitlen+1;

    hs->inbytelen = ceil_divide(hs->inbitlen, 8);
	hs->addrbytelen = ceil_divide(hs->addrbitlen, 8);
	hs->outbytelen = ceil_divide(hs->outbitlen, 8);

    hs->nhfvals = ceil_divide(hs->outbytelen, MAX_TABLE_SIZE_BYTES); 

	nrndbytes = (1<<(8*MAX_TABLE_SIZE_BYTES)) * sizeof(uint32_t);

    for(i = 0; i < NUM_HASH_FUNCTIONS; i++) {
		hs->hf_values[i] = (uint32_t**) malloc(sizeof(uint32_t*) * hs->nhfvals);

		for(j = 0; j < hs->nhfvals; j++) {
			hs->hf_values[i][j] = (uint32_t*) malloc(nrndbytes);
			assert(hs->hf_values[i][j]);
			gen_rnd_bytes(prf_state, (uint8_t*) hs->hf_values[i][j], nrndbytes);
		}
	}

	hs->address_used = (uint32_t*) calloc(nbins, sizeof(uint32_t));
	hs->mask = 0xFFFFFFFF;
	if(hs->inbytelen < sizeof(uint32_t)) {
		hs->mask >>= (sizeof(uint32_t) * 8 - hs->inbitlen - hs->addrbitlen);
	}
}

/*
/**
 * @brief Deallocates memory used by a hashing state object.
 * 
 * @param hs Pointer to the hashing state object (hs_t) whose memory is to be deallocated.
*/


static void free_hashing_state(hs_t* hs){
    uint32_t i, j;
	for(i = 0; i < NUM_HASH_FUNCTIONS; i++) {
		for(j = 0; j  < hs->nhfvals; j++) {
			free(hs->hf_values[i][j]);
		}
		free(hs->hf_values[i]);
	}
	free(hs->address_used);
}

#endif /* HASHING_UTIL_H_*/