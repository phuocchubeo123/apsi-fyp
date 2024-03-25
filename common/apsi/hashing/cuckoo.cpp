#include "cuckoo.h"


namespace apsi{
	namespace hashing{
		CuckooTable::CuckooTable(int hash_table_size, int hash_function_count, Item empty_element){
			hash_table_size_ = hash_table_size;
			hash_table_.resize(hash_table_size);
			hash_function_count_ = hash_function_count;
		}

		void CuckooTable::cuckoo_hashing cuckoo_hashing()
		{
			//The resulting hash table
			uint32_t i, j;
			uint32_t *perm_ptr;
			pthread_t* entry_gen_tasks;
			vector<Item> ctx[params_.item_params().table_size];
			hs_t hs;

    		init_hashing_state(&hs, neles, bitlen, nbins, prf_state);
			*outbitlen = hs.outbitlen;

    		cuckoo_table = (cuckoo_entry_ctx**) calloc(nbins, sizeof(cuckoo_entry_ctx*));

			cuckoo_entries = (cuckoo_entry_ctx*) malloc(neles * sizeof(cuckoo_entry_ctx));
			entry_gen_tasks = (pthread_t*) malloc(sizeof(pthread_t) * ntasks);
			ctx = (cuckoo_entry_gen_ctx*) malloc(sizeof(cuckoo_entry_gen_ctx) * ntasks);

			for(i = 0; i < ntasks; i++) {
				ctx[i].elements = elements;
				ctx[i].cuckoo_entries = cuckoo_entries;
				ctx[i].hs = &hs;
				ctx[i].startpos = i * ceil_divide(neles, ntasks);
				ctx[i].endpos = min(ctx[i].startpos + ceil_divide(neles, ntasks), neles);
				//cout << "Thread " << i << " starting from " << ctx[i].startpos << " going to " << ctx[i].endpos << " for " << neles << " elements" << endl;
				if(pthread_create(entry_gen_tasks+i, NULL, gen_cuckoo_entries, (void*) (ctx+i))) {
					cerr << "Error in creating new pthread at cuckoo hashing!" << endl;
					exit(0);
				}
			}

			for(i = 0; i < ntasks; i++) {
				if(pthread_join(entry_gen_tasks[i], NULL)) {
					cerr << "Error in joining pthread at cuckoo hashing!" << endl;
					exit(0);
				}
			}

			ctx[0].elements = elements;
			ctx[0].cuckoo_entries = cuckoo_entries;
			ctx[0].hs = &hs;
			ctx[0].startpos = 0;
			ctx[0].endpos = neles;
			gen_cuckoo_entries(ctx);

			for(i = 0; i < neles; i++) {
				if(!(insert_element(cuckoo_table, cuckoo_entries + i, neles))) {
					fails++;
            		throw runtime_error("failed to insert item into cuckoo table");
				}
			}

			perm_ptr = perm;

			hash_table = (uint8_t*) calloc(nbins, hs.outbytelen);

			for(i = 0; i < nbins; i++) {
				if(cuckoo_table[i] != NULL) {
					memcpy(hash_table + i * hs.outbytelen, cuckoo_table[i]->val, hs.outbytelen);
					//cout << "copying value: " << (hex) << (unsigned int) cuckoo_table[i]->val[cuckoo_table[i]->pos][0] << (dec) << endl;
					*perm_ptr = cuckoo_table[i]->eleid;
					perm_ptr++;
					nelesinbin[i] = 1;
				} else {
					memset(hash_table + i * hs.outbytelen, DUMMY_ENTRY_CLIENT, hs.outbytelen);
					nelesinbin[i] = 0;
				}
			}

			//Cleanup
			for(i = 0; i < neles; i++) {
				free(cuckoo_entries[i].val);
			}

			free(cuckoo_entries);
			free(cuckoo_table);
			free(entry_gen_tasks);
			free(ctx);

			free_hashing_state(&hs);

			return hash_table;
		}

		inline bool insert_element(CuckooEntry element) {
			CuckooEntry *evicted, *tmp_evicted;
			uint32_t i, ev_pos, iter_cnt;

			for(iter_cnt = 0, evicted = element; iter_cnt < max_iterations; iter_cnt++) {
				//TODO: assert(addr < MAX_TAB_ENTRIES)
				for(i = 0; i < hash_functions_count_; i++) {//, ele_pos=(ele_pos+1)%NUM_HASH_FUNCTIONS) {
					if(hash_table[evicted->address[i]] == NULL) {
						ctable[evicted->address[i]] = evicted;
						evicted->pos = i;
						return true;
					}
				}

				//choose random bin to evict other element
				ev_pos = evicted->address[(evicted->pos^iter_cnt) % NUM_HASH_FUNCTIONS];

				tmp_evicted = ctable[ev_pos];
				ctable[ev_pos] = evicted;

				evicted = tmp_evicted;

				//change position - if the number of HF's is increased beyond 2 this should be replaced by a different strategy
				evicted->pos = (evicted->pos+1) % NUM_HASH_FUNCTIONS;
			}

			//the highest number of iterations has been reached
			return false;
		}

	} // namespace hashing
} // namespace apsi
