#include "hashing_util.h"

namespace apsi{
    namespace hashing{
        LubyRackoff::LubyRackoff(PSIParams params) : params_(move(params)){
            initialize();
        }

        void LubyRackoff::initialize() {
            table_size = params_.table_params().table_size;
            inbitlen = params.item_params().hashed_item_bit_count;
	        addrbitlen = min((uint32_t) ceil_log2(table_size_), inbitlen_);

            // Luby-rackoff encryption a.k.a point-and-permute
	        outbitlen = inbitlen - addrbitlen + 1;

            inbytelen = ceil_divide(inbitlen, 8);
	        addrbytelen = ceil_divide(addrbitlen, 8);
	        outbytelen = ceil_divide(outbitlen, 8);

            hash_functions_values_count = ceil_divide(outbytelen, MAX_TABLE_SIZE_BYTES); 

	        int nrndbytes;
	        nrndbytes = (1<<(8*MAX_TABLE_SIZE_BYTES)) * sizeof(uint32_t);

            for(int i = 0; i < hash_functions_count; i++) {
		        hf_values[i] = (uint32_t**) malloc(sizeof(uint32_t*) * nhfvals);

		        for(int j = 0; j < hs->nhfvals; j++) {
			        hf_values[i][j] = (uint32_t*) malloc(nrndbytes);
			        gen_rnd_bytes(prf_state, (uint8_t*) hf_values[i][j], nrndbytes);
		        }
	        }

	        address_used = (uint32_t*) calloc(nbins, sizeof(uint32_t));
	        mask = 0xFFFFFFFF;
	        if(inbytelen < sizeof(uint32_t)) {
		        mask >>= (sizeof(uint32_t) * 8 - inbitlen - addrbitlen);
	        }
        }

        void LubyRackoff::free_hashing_state(){
            uint32_t i, j;
	        for(i = 0; i < NUM_HASH_FUNCTIONS; i++) {
		        for(j = 0; j  < nhfvals; j++) {
			        free(hf_values[i][j]);
		        }
		        free(hf_values[i]);
	        }
        }

        void LubyRackoff::point_and_permute(uint32_t element, uint32_t* address, uint8_t* vals) {

	        uint64_t i, j, L, R;
	        TABLEID_T hfmaskaddr;
	        //Store the first hs->addrbitlen bits in L
	        L = element & SELECT_BITS[addrbitlen];

	        //Store the remaining hs->outbitlen bits in R and pad correspondingly
	        R = element & (SELECT_BITS_INV[addrbitlen] >> addrbitlen);
	        R &= mask; //mask = (1<<32-hs->addrbitlen)


	        hfmaskaddr = R * sizeof(uint32_t);
	        //cout << "L = " << L << ", R = " << R << " addresses: ";

	        for(i = 0; i < NUM_HASH_FUNCTIONS; i++) {
			    address[i] = (L ^ (hf_values[i][0])) % table_size;
	        }

            for(uint64_t i = 0; i < NUM_HASH_FUNCTIONS; i++) {
		        address[i] = (((element+i) ^ HF_MASKS[i]) & SELECT_BITS[addrbitlen]) % table_size;
	        }
        }

		uint32_t LubyRackoff::domain_hashing(uint32_t element) {
			uint32_t result;//, *hash_buf;
			result = sha256_hash(element, inbitlen, outbitlen);
			return result;
		}


    } // namespace hashing
} // namespace apsi
