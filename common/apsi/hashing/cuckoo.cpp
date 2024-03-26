#include "cuckoo.h"


namespace apsi{
	namespace hashing{
		CuckooTable::CuckooTable(PSIParams params, Item empty_element, LubyRackoff hs): params_(params), empty_element_(empty_element), hs_(hs){
			initialize();
		}

		void CuckooTable::initialize()
		{
			hash_table.resize(params_.table_params().table_size);
			hash_func_count = params_.table_params().hash_func_count;
		}


		bool CuckooTable::insert_element(const CuckooEntry &element) {
			CuckooEntry evicted, tmp_evicted;
			uint32_t ev_pos, iter_cnt;

			for(iter_cnt = 0, evicted = element; iter_cnt < max_iterations; iter_cnt++) {
				//TODO: assert(addr < MAX_TAB_ENTRIES)
				for(int i = 0; i < hash_func_count; i++) {
					if(hash_table[evicted.address[i]].is_empty()) {
						evicted.pos = i;
						hash_table[evicted.address[i]] = evicted;
						return true;
					}
				}

				//choose random bin to evict other element
				ev_pos = evicted.address[(evicted.pos^iter_cnt) % hash_func_count];

				tmp_evicted = hash_table[ev_pos];
				hash_table[ev_pos] = evicted;

				evicted = tmp_evicted;

				//change position - if the number of HF's is increased beyond 2 this should be replaced by a different strategy
				evicted.increase_pos();
			}

			//the highest number of iterations has been reached
			return false;
		}

	} // namespace hashing
} // namespace apsi
