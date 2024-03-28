#include "branching_program.h"

using namespace std;
using namespace apsi;
using namespace seal;

namespace apsi{
    BranchingProgram::BranchingProgram(): nodes_count_(1), leaves_count_(0){
        clear();
    }

    BranchingProgram::BranchingProgram(const CryptoContext &crypto_context): 
        nodes_count_(1), leaves_count_(0), crypto_context_(crypto_context)
    {
        clear();
    }

    void BranchingProgram::clear(){
        id.clear(); id.push_back(0);

        vector<uint64_t> zero_const(crypto_context_.encoder()->slot_count(), 0);
        vector<uint64_t> one_const(crypto_context_.encoder()->slot_count(), 1);

        crypto_context_.encoder()->encode(zero_const, zero_ptx);
        crypto_context_.encoder()->encode(one_const, one_ptx);
    }

    template <>
    int32_t BranchingProgram::addItem(const PlaintextBits &item){
        item_.push_back(item);
        return item_.size();
    }

    Ciphertext BranchingProgram::eval(
        const vector<Ciphertext> &ciphertext_bits, 
        const CryptoContext &crypto_context,
        MemoryPoolHandle &pool) const
    {
        auto seal_context = crypto_context.seal_context();
        auto evaluator = crypto_context.evaluator();
        auto relin_keys = crypto_context.relin_keys();

        Ciphertext tot_ctx;
        Ciphertext com_ctx;

        uint32_t depth = ciphertext_bits.size();
        vector<vector<Ciphertext>> path_costs(depth);
        for (int d = 0; d < depth; d++){
            uint32_t max_sub_path;
            // if (depth == 16) max_sub_path = 4;
            // else max_sub_path = 5;
            for (int l = 0; (((d+1) >> l) << l) == d+1; l++) max_sub_path = l;
            // if (max_sub_path > 5) max_sub_path = 5;
            path_costs[d].resize(max_sub_path + 1);
            // APSI_LOG_DEBUG("Num paths: " << d << " " << max_sub_path + 1);
        }

        bool met_leaf = false;
        
        for (int item_idx = 0; item_idx < item_.size(); item_idx++){
            bool first_idx = true;
            for (int level = 0; level < depth; level++){
                path_costs[level][0] = ciphertext_bits[level];
                Plaintext ptx;
                crypto_context.encoder()->encode(item_[item_idx].get_bit(level), ptx);
                // vector<uint64_t> ptx_vt;
                // crypto_context.encoder()->decode(ptx, ptx_vt);
                // std::cout << "Plaintext: ";
                // for (int i = 0; i < 10; i++) cout << ptx_vt[i] << " ";
                // cout << "\n";
                evaluator->add_inplace(path_costs[level][0], ciphertext_bits[level]);
                evaluator->multiply_plain_inplace(path_costs[level][0], ptx);
                evaluator->sub_plain_inplace(path_costs[level][0], ptx);
                evaluator->sub_inplace(path_costs[level][0], ciphertext_bits[level]);
                evaluator->add_plain_inplace(path_costs[level][0], one_ptx);
            }

            for (int level = 0; level < depth; level++){
                for (int l = 1; l < path_costs[level].size(); l++){
                    APSI_LOG_INFO("Computing level: " << level << " with log path: " << l);
                    path_costs[level][l] = path_costs[level][l-1];
                    if (level - (1 << (l-1)) >= 0){
                        APSI_LOG_INFO("Multiplying");
                        evaluator->multiply_inplace(path_costs[level][l], path_costs[level-(1<<(l-1))][l-1]);
                        evaluator->relinearize_inplace(path_costs[level][l], *relin_keys, pool);
                    }
                    else{
                        APSI_LOG_INFO("Adding");
                        evaluator->add_plain_inplace(path_costs[level][l], zero_ptx);
                    }
                    evaluator->mod_switch_to_next_inplace(path_costs[level][l]);
                }
                // evaluator->add_inplace(path_costs[level][0], path_costs[level-1][0]);
            }

            int d = depth;
            if (depth == 24){
                com_ctx = path_costs[d-1][3];
                evaluator->mod_switch_to_next_inplace(com_ctx);
                evaluator->multiply_inplace(com_ctx, path_costs[15][4]);
                evaluator->relinearize_inplace(com_ctx, *relin_keys, pool);
                evaluator->mod_switch_to_next_inplace(com_ctx);
            }
            else{
                com_ctx = path_costs[d-1].back();
            }

            if (met_leaf){
                // evaluator->add_inplace(tot_ctx, path_costs[depth-1].back());
                // evaluator->add_inplace(tot_ctx, path_costs[depth-1][0]);
                evaluator->add_inplace(tot_ctx, com_ctx);
            } else{
                // tot_ctx = path_costs[depth-1].back();
                tot_ctx = com_ctx;
                met_leaf = true;
            }
        }

        return tot_ctx;
    }

    Ciphertext BranchingProgram::eval_less_mult(
        const vector<Ciphertext> &ciphertext_bits, 
        const CryptoContext &crypto_context,
        MemoryPoolHandle &pool) const
    {
        auto seal_context = crypto_context.seal_context();
        auto evaluator = crypto_context.evaluator();
        auto relin_keys = crypto_context.relin_keys();

        Ciphertext tot_ctx;
        Ciphertext com_ctx;

        uint32_t depth = ciphertext_bits.size();
        uint32_t ps_degree = ceil(sqrt(depth));

        Ciphertext total_bit_compare;

        for (int item_idx = 0; item_idx < item_.size(); item_idx++){
            Ciphertext bit_compare;
            for (int level = 0; level < depth; level++){
                Ciphertext curr_bit = ciphertext_bits[level];
                Plaintext ptx;
                crypto_context.encoder()->encode(item_[item_idx].get_bit(level), ptx);

                evaluator->add_inplace(curr_bit, ciphertext_bits[level]);
                evaluator->multiply_plain_inplace(curr_bit, ptx);
                evaluator->sub_plain_inplace(curr_bit, ptx);
                evaluator->sub_inplace(curr_bit, ciphertext_bits[level]);
                evaluator->add_plain_inplace(curr_bit, one_ptx);
                evaluator->add_inplace(bit_compare, curr_bit);
            }
        }
        
        // for (int item_idx = 0; item_idx < item_.size(); item_idx++){

        //     for (int level = 0; level < depth; level++){
        //         for (int l = 1; l < path_costs[level].size(); l++){
        //             APSI_LOG_INFO("Computing level: " << level << " with log path: " << l);
        //             path_costs[level][l] = path_costs[level][l-1];
        //             if (level - (1 << (l-1)) >= 0){
        //                 APSI_LOG_INFO("Multiplying");
        //                 evaluator->multiply_inplace(path_costs[level][l], path_costs[level-(1<<(l-1))][l-1]);
        //                 evaluator->relinearize_inplace(path_costs[level][l], *relin_keys, pool);
        //             }
        //             else{
        //                 APSI_LOG_INFO("Adding");
        //                 evaluator->add_plain_inplace(path_costs[level][l], zero_ptx);
        //             }
        //             evaluator->mod_switch_to_next_inplace(path_costs[level][l]);
        //         }
        //         // evaluator->add_inplace(path_costs[level][0], path_costs[level-1][0]);
        //     }

        //     int d = depth;
        //     if (depth == 24){
        //         com_ctx = path_costs[d-1][3];
        //         evaluator->mod_switch_to_next_inplace(com_ctx);
        //         evaluator->multiply_inplace(com_ctx, path_costs[15][4]);
        //         evaluator->relinearize_inplace(com_ctx, *relin_keys, pool);
        //         evaluator->mod_switch_to_next_inplace(com_ctx);
        //     }
        //     else{
        //         com_ctx = path_costs[d-1].back();
        //     }

        //     if (met_leaf){
        //         // evaluator->add_inplace(tot_ctx, path_costs[depth-1].back());
        //         // evaluator->add_inplace(tot_ctx, path_costs[depth-1][0]);
        //         evaluator->add_inplace(tot_ctx, com_ctx);
        //     } else{
        //         // tot_ctx = path_costs[depth-1].back();
        //         tot_ctx = com_ctx;
        //         met_leaf = true;
        //     }
        // }

        return tot_ctx;
    }
}