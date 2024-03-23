#include "cuckoo.h"

uint32_t cuckoo_hashing(
    const vector<Item> &items;
    uint32_t neles, 
    uint32_t nbins, 
    uint32_t bitlen, 
    uint32_t *outbitlen,
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

    cuckoo_table = (cuckoo_entry_ctx**) calloc(nbins, sizeof(cuckoo_entry_ctx*));

	cuckoo_entries = (cuckoo_entry_ctx*) malloc(neles * sizeof(cuckoo_entry_ctx));
	entry_gen_tasks = (pthread_t*) malloc(sizeof(pthread_t) * ntasks);
	ctx = (cuckoo_entry_gen_ctx*) malloc(sizeof(cuckoo_entry_gen_ctx) * ntasks);

	ctx[0].elements = elements;
	ctx[0].cuckoo_entries = cuckoo_entries;
	ctx[0].hs = &hs;
	ctx[0].startpos = 0;
	ctx[0].endpos = neles;
	gen_cuckoo_entries(ctx);

	for(i = 0; i < neles; i++) {
		if(!(insert_element(cuckoo_table, cuckoo_entries + i, neles))) {
			fails++;
		}
	}

	perm_ptr = perm;
}