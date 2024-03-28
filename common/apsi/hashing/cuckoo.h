/*
 * cuckoo.h
 *
 *  Created on: Oct 7, 2014
 *      Author: mzohner
 */

#ifndef CUCKOO_H_
#define CUCKOO_H_

#include "hashing_util.h"
#include "apsi/item.h"
#include "apsi/psi_params.h"

namespace apsi{
	namespace hashing{
		struct CuckooEntry{	
			CuckooEntry(){
				address.resize(1);
				address[0] = -1;
			}


			CuckooEntry(const PSIParams &params){
				hash_func_count = params.table_params().hash_func_count;	
				address.resize(hash_func_count);
			}

			void increase_pos(){
				(pos += 1) %= hash_func_count;
			}

			bool is_empty(){
				return (address[0] == -1);
			}

			uint32_t hash_func_count;
			vector<uint32_t> address;

			uint32_t pos;
		};


		class CuckooTable{
		public:
			CuckooTable(PSIParams params, Item empty_element);

			void initialize();
			bool insert_element(const CuckooEntry &element);		

		private:
			PSIParams params_;
			Item empty_element_;

			vector<CuckooEntry> hash_table;
			uint32_t hash_func_count;

			uint32_t max_iterations;
			uint32_t bit_length;
			uint32_t out_bit_length;
			uint32_t* perm;
			prf_state_ctx* prf_state;
		}; // class CuckooHashing
	} // namespace hashing
} // namespace apsi


#endif /*CUCKOO_H_*/