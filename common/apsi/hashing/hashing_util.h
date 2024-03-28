/*
 * hashing_util.h
 *
 *  Created on: Oct 8, 2014
 *      Author: mzohner
 * github: https://github.com/encryptogroup/PSI.git
 */

#include <cstdint>

#include "apsi/log.h"
#include "apsi/crypto/crypto.h"
#include "apsi/psi_params.h"

#ifndef HASHING_UTIL_H_
#define HASHING_UTIL_H_

typedef std::uint16_t TABLEID_T; // the datatype to store table address

#define NUM_HASH_FUNCTIONS 3

#define MAX_TABLE_SIZE_BYTES sizeof(TABLEID_T)
#define DUMMY_ENTRY_SERVER 0x00
#define DUMMY_ENTRY_CLIENT 0xFF

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


namespace apsi{
	namespace hashing{
		struct LubyRackoff{
		public:
			LubyRackoff(PSIParams params, prf_state_ctx* prf_state);

			void initialize();

			void free_hashing_state();

        	vector<uint32_t> point_and_permute(uint32_t element);
			uint32_t domain_hashing(uint32_t element);

		private:
			PSIParams params_;

			uint32_t hash_func_count;

			vector<uint32_t> hf_values; // hash function values
			uint32_t hash_functions_values_count; // the number of possible hash function values
			std::uint32_t table_size;
			std::uint32_t item_bit_count;
			std::uint32_t inbitlen;
			std::uint32_t addrbitlen; // bit length of address (which bin it is in?)
			std::uint32_t outbitlen; // bit length of output after point-and-permute

			uint32_t* address_used;
			uint32_t mask;
			prf_state_ctx* prf_state_;		
		};
	}
}





#endif /* HASHING_UTIL_H_*/