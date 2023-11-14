// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// STD
#include <stdexcept>

// APSI
#include "apsi/log.h"
#include "apsi/psi_params.h"
#include "apsi/query.h"

using namespace std;
using namespace seal;

namespace apsi {
    using namespace network;

    namespace sender {
        Query Query::deep_copy() const
        {
            Query result;
            result.relin_keys_ = relin_keys_;
            result.data_ = data_;
            result.sender_db_ = sender_db_;
            result.compr_mode_ = compr_mode_;

            return result;
        }

        Query::Query(QueryRequest query_request, shared_ptr<SenderDB> sender_db)
        {
            if (!sender_db) {
                throw invalid_argument("sender_db cannot be null");
            }
            if (!query_request) {
                throw invalid_argument("query_request cannot be null");
            }

            compr_mode_ = query_request->compr_mode;

            sender_db_ = move(sender_db);
            auto seal_context = sender_db_->get_seal_context();

            // Extract and validate relinearization keys
            if (seal_context->using_keyswitching()) {
                relin_keys_ = query_request->relin_keys.extract(seal_context);
                if (!is_valid_for(relin_keys_, *seal_context)) {
                    APSI_LOG_ERROR("Extracted relinearization keys are invalid for SEALContext");
                    return;
                }
            }

            // Extract and validate query ciphertexts
            for (auto &q : query_request->data) {
                APSI_LOG_DEBUG(
                    "Extracting " << q.second.size() << " ciphertexts for exponent " << q.first);
                vector<Ciphertext> cts;
                for (auto &ct : q.second) {
                    cts.push_back(ct.extract(seal_context));
                    if (!is_valid_for(cts.back(), *seal_context)) {
                        APSI_LOG_ERROR("Extracted ciphertext is invalid for SEALContext");
                        return;
                    }
                }
                data_[q.first] = move(cts);
            }

            // Get the PSIParams
            PSIParams params(sender_db_->get_params());

            uint32_t bundle_idx_count = params.bundle_idx_count();

            for (auto &q : data_) {
                // Check that powers in the query data match source nodes in the PowersDag
                if (q.second.size() != bundle_idx_count) {
                    APSI_LOG_ERROR(
                        "Extracted query data is incompatible with PSI parameters: "
                        "query power "
                        << q.first << " contains " << q.second.size()
                        << " ciphertexts which does not "
                           "match with bundle_idx_count ("
                        << bundle_idx_count << ")");
                    return;
                }
            }

            // The query is valid
            valid_ = true;
        }
    } // namespace sender
} // namespace apsi