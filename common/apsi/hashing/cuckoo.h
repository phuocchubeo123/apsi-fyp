/*
 * cuckoo.h
 *
 *  Created on: Oct 7, 2014
 *      Author: mzohner
 */

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include "hashing_util.h"

struct cuckoo_entry_ctx {
	//id of the element in the source set
	uint32_t eleid;
	//addresses the bin of the cuckoo entry in the cuckoo table, will only work for up to 2^{32} bins
	uint32_t address[NUM_HASH_FUNCTIONS];
	//the value of the entry
	uint8_t* val;
	//which position is the entry currently mapped to
	uint32_t pos;
#ifdef DEBUG_CUCKOO
	uint8_t* element;
#endif
};


uint32_t cuckoo_hashing(
    const vector<Item> &items;
    uint32_t neles, 
    uint32_t nbins, uint32_t bitlen, uint32_t *outbitlen,
	uint32_t* perm,	
    prf_state_ctx* prf_state);


#endif /*CUCKOO_H_*/