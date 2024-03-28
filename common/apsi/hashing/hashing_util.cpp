#include "hashing_util.h"

namespace apsi{
    namespace hashing{
        LubyRackoff::LubyRackoff(PSIParams params, prf_state_ctx* prf_state) : params_(move(params)), prf_state_(prf_state){
            initialize();
        }

        void LubyRackoff::initialize() {
            table_size = params_.table_params().table_size;
			item_bit_count = params_.item_params().item_bit_count;
            inbitlen = params_.item_params().hashed_item_bit_count;
	        addrbitlen = min((uint32_t) ceil_log2(table_size), inbitlen);

            // Luby-rackoff encryption a.k.a point-and-permute
	        outbitlen = inbitlen - addrbitlen + 1;

            hash_functions_values_count = ceil_divide(ceil_divide(outbitlen, 8), MAX_TABLE_SIZE_BYTES); 

			APSI_LOG_INFO("Done some simple stuff");

	        int nrndbytes;
	        nrndbytes = (1<<(8*MAX_TABLE_SIZE_BYTES)) * sizeof(uint32_t);

            for(int i = 0; i < hash_func_count; i++) {
			    uint32_t x = gen_rnd_bytes(prf_state_, nrndbytes);
				hf_values[i] = x;
	        }

			APSI_LOG_INFO("Done generate seed");

	        address_used = (uint32_t*) calloc(table_size, sizeof(uint32_t));
	        mask = 0xFFFFFFFF;
	        if(ceil_divide(inbitlen, 8) < sizeof(uint32_t)) {
		        mask >>= (sizeof(uint32_t) * 8 - inbitlen - addrbitlen);
	        }
        }

        void LubyRackoff::free_hashing_state(){
            uint32_t i, j;
	        // for(i = 0; i < hash_functions_count; i++) {
		    //     free(hf_values[i]);
	        // }
        }

        vector<uint32_t> LubyRackoff::point_and_permute(uint32_t element) {

	        uint64_t i, j, L, R;
	        TABLEID_T hfmaskaddr;
	        //Store the first hs->addrbitlen bits in L
	        L = element & SELECT_BITS[addrbitlen];

	        //Store the remaining hs->outbitlen bits in R and pad correspondingly
	        R = element & (SELECT_BITS_INV[addrbitlen] >> addrbitlen);
	        R &= mask; //mask = (1<<32-hs->addrbitlen)


	        hfmaskaddr = R * sizeof(uint32_t);
	        //cout << "L = " << L << ", R = " << R << " addresses: ";

			vector<uint32_t> address(hash_func_count);

	        for(i = 0; i < hash_func_count; i++) {
			    address[i] = (L ^ (hf_values[i])) % table_size;
	        }

            for(uint64_t i = 0; i < hash_func_count; i++) {
		        address[i] = (((element+i) ^ HF_MASKS[i]) & SELECT_BITS[addrbitlen]) % table_size;
	        }

			return address;
        }

		uint32_t LubyRackoff::domain_hashing(uint32_t element) {
			uint32_t result;//, *hash_buf;
			result = sha256_hash(element, item_bit_count, inbitlen);
			return result;
		}


    } // namespace hashing
} // namespace apsi
