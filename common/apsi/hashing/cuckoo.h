/*
 * cuckoo.h
 *
 *  Created on: Oct 7, 2014
 *      Author: mzohner
 */

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include "hashing_util.h"

uint32_t cuckoo_hashing(
    const vector<Item> &items;
    uint32_t neles, 
    uint32_t nbins, uint32_t bitlen, uint32_t *outbitlen,
	uint32_t* perm,	
    prf_state_ctx* prf_state)
{
	//The resulting hash table
	uint8_t* hash_table;
	cuckoo_entry_ctx** cuckoo_table;
	cuckoo_entry_ctx* cuckoo_entries;
	uint32_t i, j;
	uint32_t *perm_ptr;
	pthread_t* entry_gen_tasks;
	cuckoo_entry_gen_ctx* ctx;
	hs_t hs;

    init_hashing_state(&hs, neles, bitlen, nbins, prf_state);
	*outbitlen = hs.outbitlen;
}

#endif /*CUCKOO_H_*/