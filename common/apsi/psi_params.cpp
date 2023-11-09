// STD
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

// APSI
#include "apsi/psi_params.h"
#include "apsi/psi_params_generated.h"
#include "apsi/version.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/context.h"
#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

#ifndef APSI_DISABLE_JSON
// JSON
#include "json/json.h"
#endif

using namespace std;
using namespace seal;
using namespace seal::util;

#ifndef APSI_DISABLE_JSON
namespace {
    const Json::Value &get_non_null_json_value(const Json::Value &parent, const string &name)
    {
        const auto &value = parent[name];
        if (value.isNull()) {
            stringstream ss;
            ss << name << " is not present in " << parent;
            throw runtime_error(ss.str());
        }

        return value;
    }

    uint32_t json_value_ui32(const Json::Value &value)
    {
        if (!value.isUInt()) {
            stringstream ss;
            ss << value.asCString() << " should be an unsigned int32";
            throw runtime_error(ss.str());
        }

        return value.asUInt();
    }

    uint32_t json_value_ui32(const Json::Value &parent, const string &name)
    {
        const auto &value = get_non_null_json_value(parent, name);
        return json_value_ui32(value);
    }

    uint64_t json_value_ui64(const Json::Value &parent, const string &name)
    {
        const auto &value = get_non_null_json_value(parent, name);

        if (!value.isUInt64()) {
            stringstream ss;
            ss << name << " should be an unsigned int64";
            throw runtime_error(ss.str());
        }

        return value.asUInt64();
    }

    int json_value_int(const Json::Value &value)
    {
        if (!value.isInt()) {
            stringstream ss;
            ss << value.asCString() << " should be an int";
            throw runtime_error(ss.str());
        }

        return value.asInt();
    }

    int json_value_int(const Json::Value &parent, const string &name)
    {
        const auto &value = get_non_null_json_value(parent, name);
        return json_value_int(value);
    }
} // namespace
#endif

namespace apsi {
    void PSIParams::initialize()
    {
        // Checking the validity of parameters
        if (!table_params_.table_size) {
            throw invalid_argument("table_size cannot be zero");
        }
        if (!table_params_.max_items_per_bin) {
            throw invalid_argument("max_items_per_bin cannot be zero");
        }
        if (table_params_.hash_func_count < TableParams::hash_func_count_min ||
            table_params_.hash_func_count > TableParams::hash_func_count_max) {
            throw invalid_argument("hash_func_count is too large or too small");
        }
        if (item_params_.item_bit_count < ItemParams::item_bit_count_min ||
            item_params_.item_bit_count > ItemParams::item_bit_count_max) {
            throw invalid_argument("item_bit_count is too large or too small");
        }
        
        // Create a SEALContext (with expand_mod_chain == false) to check validity of parameters
        SEALContext seal_context(seal_params_, false, sec_level_type::tc128);
        if (!seal_context.parameters_set()) {
            stringstream ss;
            ss << "Microsoft SEAL parameters are invalid: "
               << seal_context.parameter_error_message();
            throw invalid_argument(ss.str());
        }
        if (!seal_context.key_context_data()->qualifiers().using_batching) {
            throw invalid_argument(
                "Microsoft SEAL parameters do not support batching; plain_modulus must be a prime "
                "congruent to 1 modulo 2*poly_modulus_degree");
        }

        max_bins_per_bundle_ = seal_params_.poly_modulus_degree();
        if (table_params_.receiver_bins_per_bundle > max_bins_per_bundle_){
            throw invalid_argument(
                "Receiver's number of bins per bundle exceeds the maximum batch size!"
            );
        }
        if (table_params_.sender_bins_per_bundle > max_bins_per_bundle_){
            throw invalid_argument(
                "Sender's number of bins per bundle exceeds the maximum batch size!"
            );
        }

        // table_size must be a multiple of items_per_bundle_
        if (table_params_.table_size % table_params_.receiver_bins_per_bundle) {
            throw invalid_argument(
                "table_size must be a multiple of floor(poly_modulus_degree / felts_per_item)");
        }

        sender_bins_per_bundle_ = table_params_.sender_bins_per_bundle;
        receiver_bins_per_bundle_ = table_params_.receiver_bins_per_bundle;

        // Compute the number of bundle indices; this is now guaranteed to be greater than zero
        bundle_idx_count_ = table_params_.table_size / receiver_bins_per_bundle_;

        item_bit_count_ = item_params_.item_bit_count;
    }

    size_t PSIParams::save(ostream &out) const
    {
        flatbuffers::FlatBufferBuilder fbs_builder(128);

        fbs::TableParams table_params(
            table_params_.table_size,
            table_params_.max_items_per_bin,
            table_params_.hash_func_count,
            table_params_.receiver_bins_per_bundle,
            table_params_.sender_bins_per_bundle
            );

        fbs::ItemParams item_params(
            item_params_.item_bit_count
        );

        vector<seal_byte> temp;
        temp.resize(safe_cast<size_t>(seal_params_.save_size(compr_mode_type::none)));
        size_t size =
            static_cast<size_t>(seal_params_.save(temp.data(), temp.size(), compr_mode_type::none));
        auto seal_params_data =
            fbs_builder.CreateVector(reinterpret_cast<const uint8_t *>(temp.data()), size);
        auto seal_params = fbs::CreateSEALParams(fbs_builder, seal_params_data);

        fbs::PSIParamsBuilder psi_params_builder(fbs_builder);
        psi_params_builder.add_version(apsi_serialization_version);
        psi_params_builder.add_table_params(&table_params);
        psi_params_builder.add_seal_params(seal_params);
        auto psi_params = psi_params_builder.Finish();
        fbs_builder.FinishSizePrefixed(psi_params);

        out.write(
            reinterpret_cast<const char *>(fbs_builder.GetBufferPointer()),
            safe_cast<streamsize>(fbs_builder.GetSize()));

        return fbs_builder.GetSize();
    }

    pair<PSIParams, size_t> PSIParams::Load(istream &in)
    {
        vector<unsigned char> in_data(util::read_from_stream(in));

        auto verifier = flatbuffers::Verifier(
            reinterpret_cast<const uint8_t *>(in_data.data()), in_data.size());
        bool safe = fbs::VerifySizePrefixedPSIParamsBuffer(verifier);
        if (!safe) {
            throw runtime_error("failed to load parameters: invalid buffer");
        }

        auto psi_params = fbs::GetSizePrefixedPSIParams(in_data.data());

        if (!same_serialization_version(psi_params->version())) {
            // Check that the serialization version numbers match
            APSI_LOG_ERROR(
                "Loaded PSIParams data indicates a serialization version number ("
                << psi_params->version()
                << ") incompatible with the current serialization version number ("
                << apsi_serialization_version << ")");
            throw runtime_error("failed to load parameters: incompatible serialization version");
        }

        PSIParams::TableParams table_params;
        table_params.table_size = psi_params->table_params()->table_size();
        table_params.max_items_per_bin = psi_params->table_params()->max_items_per_bin();
        table_params.hash_func_count = psi_params->table_params()->hash_func_count();
        table_params.receiver_bins_per_bundle = psi_params->table_params()->receiver_bins_per_bundle();
        table_params.sender_bins_per_bundle = psi_params->table_params()->sender_bins_per_bundle();

        PSIParams::ItemParams item_params;
        item_params.item_bit_count = psi_params->item_params()->item_bit_count();

        PSIParams::SEALParams seal_params;
        auto &seal_params_data = *psi_params->seal_params()->data();
        try {
            seal_params.load(
                reinterpret_cast<const seal_byte *>(seal_params_data.data()),
                seal_params_data.size());
        } catch (const logic_error &ex) {
            stringstream ss;
            ss << "failed to load parameters: ";
            ss << ex.what();
            throw runtime_error(ss.str());
        } catch (const runtime_error &ex) {
            stringstream ss;
            ss << "failed to load parameters: ";
            ss << ex.what();
            throw runtime_error(ss.str());
        }

        APSI_LOG_INFO("Done loading seal params!");

        if (seal_params.scheme() != scheme_type::bfv) {
            throw runtime_error("failed to load parameters: invalid scheme type");
        }

        return { PSIParams(table_params, item_params, seal_params), in_data.size() };
    }

    #ifndef APSI_DISABLE_JSON
    PSIParams PSIParams::Load(const string &in)
    {
        // Parse JSON string
        Json::Value root;
        stringstream ss(in);
        ss >> root;

        // Load TableParams
        PSIParams::TableParams table_params;
        try {
            const auto &json_table_params = get_non_null_json_value(root, "table_params");
            table_params.hash_func_count = json_value_ui32(json_table_params, "hash_func_count");
            table_params.table_size = json_value_ui32(json_table_params, "table_size");
            table_params.max_items_per_bin =
                json_value_ui32(json_table_params, "max_items_per_bin");
            table_params.receiver_bins_per_bundle =
                json_value_ui32(json_table_params, "receiver_bins_per_bundle");
            table_params.sender_bins_per_bundle =
                json_value_ui32(json_table_params, "sender_bins_per_bundle");
        } catch (const exception &ex) {
            APSI_LOG_ERROR("Failed to load table_params from JSON string: " << ex.what());
            throw;
        }

        // Load TableParams
        PSIParams::ItemParams item_params;
        try {
            const auto &json_item_params = get_non_null_json_value(root, "item_params");
            item_params.item_bit_count = json_value_ui32(json_item_params, "item_bit_count");
        } catch (const exception &ex) {
            APSI_LOG_ERROR("Failed to load item_params from JSON string: " << ex.what());
            throw;
        }

        // Load SEALParams
        PSIParams::SEALParams seal_params;
        try {
            const auto &json_seal_params = get_non_null_json_value(root, "seal_params");
            const auto &coeff_modulus_bits =
                get_non_null_json_value(json_seal_params, "coeff_modulus_bits");

            size_t poly_modulus_degree = json_value_ui64(json_seal_params, "poly_modulus_degree");
            seal_params.set_poly_modulus_degree(poly_modulus_degree);

            if (json_seal_params.isMember("plain_modulus") &&
                json_seal_params.isMember("plain_modulus_bits")) {
                throw runtime_error(
                    "only one of plain_modulus and plain_modulus_bits must be specified");
            }
            if (json_seal_params.isMember("plain_modulus")) {
                seal_params.set_plain_modulus(json_value_ui64(json_seal_params, "plain_modulus"));
            } else if (json_seal_params.isMember("plain_modulus_bits")) {
                seal_params.set_plain_modulus(PlainModulus::Batching(
                    poly_modulus_degree, json_value_int(json_seal_params, "plain_modulus_bits")));
            } else {
                throw runtime_error("neither plain_modulus nor plain_modulus_bits was specified");
            }

            vector<int> coeff_modulus_bit_sizes;
            for (const auto &coeff : coeff_modulus_bits) {
                coeff_modulus_bit_sizes.push_back(json_value_int(coeff));
            }

            seal_params.set_coeff_modulus(
                CoeffModulus::Create(poly_modulus_degree, coeff_modulus_bit_sizes));

        } catch (const exception &ex) {
            APSI_LOG_ERROR("Failed to load seal_params from JSON string: " << ex.what());
            throw;
        }

        return PSIParams(table_params, item_params, seal_params);
    }
#else
    PSIParams PSIParams::Load(const string &in)
    {
        throw runtime_error("JSON parameter initialization is disabled");
    }
#endif

    string PSIParams::to_string() const
    {
        stringstream ss;
        ss << "; table_params.table_size: " << table_params_.table_size
           << "; table_params.max_items_per_bin: " << table_params_.max_items_per_bin
           << "; table_params.hash_func_count: " << table_params_.hash_func_count
           << "; seal_params.poly_modulus_degree: " << seal_params_.poly_modulus_degree()
           << "; seal_params.coeff_modulus: "
           << util::to_string(
                  seal_params_.coeff_modulus(),
                  [](const Modulus &mod) { return std::to_string(mod.bit_count()); })
           << "; seal_params.plain_modulus: " << seal_params_.plain_modulus().value();

        return ss.str();
    }
} // namespace apsi