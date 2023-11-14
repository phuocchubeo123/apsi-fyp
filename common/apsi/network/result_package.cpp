// STD
#include <sstream>
#include <stdexcept>
#include <thread>

// APSI
#include "apsi/item.h"
#include "apsi/log.h"
#include "apsi/network/result_package.h"
#include "apsi/network/result_package_generated.h"
#include "apsi/network/sender_operation.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/util/common.h"

// GSL
#include "gsl/span"

using namespace std;
using namespace seal;
using namespace seal::util;

namespace apsi {
    namespace network {
        size_t ResultPackage::save(ostream &out) const
        {
            flatbuffers::FlatBufferBuilder fbs_builder(1024);

            if (!Serialization::IsSupportedComprMode(compr_mode)) {
                throw runtime_error("unsupported compression mode");
            }

            vector<unsigned char> temp;
            temp.resize(psi_result.save_size(compr_mode));
            auto size = psi_result.save(temp, compr_mode);
            auto psi_ct_data =
                fbs_builder.CreateVector(reinterpret_cast<const uint8_t *>(temp.data()), size);
            auto psi_ct = fbs::CreateCiphertext(fbs_builder, psi_ct_data);

            fbs::ResultPackageBuilder rp_builder(fbs_builder);
            rp_builder.add_bundle_idx(bundle_idx);
            rp_builder.add_psi_result(psi_ct);
            auto rp = rp_builder.Finish();
            fbs_builder.FinishSizePrefixed(rp);

            out.write(
                reinterpret_cast<const char *>(fbs_builder.GetBufferPointer()),
                safe_cast<streamsize>(fbs_builder.GetSize()));

            return fbs_builder.GetSize();
        }
    } // namespace network
} // namespace apsi