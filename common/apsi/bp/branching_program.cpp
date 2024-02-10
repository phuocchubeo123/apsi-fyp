#include "branching_program.h"

using namespace std;
using namespace apsi;
using namespace seal;

namespace apsi{
    BP::BP(): nodes_count_(1), leaves_count_(0){
        clear();
    }

    BP::BP(const CryptoContext &crypto_context): 
        nodes_count_(1), leaves_count_(0), crypto_context_(crypto_context)
    {
        clear();
    }

    void BP::clear(){
        id.clear(); id.push_back(0);
        left_child_.clear(); left_child_.push_back(-1);
        right_child_.clear(); right_child_.push_back(-1);
        parent_.clear(); parent_.push_back(-1);
        level_.clear(); level_.push_back(-1);
        is_leaf_.clear(); is_leaf_.push_back(0);

        vector<uint64_t> zero_const(crypto_context_.encoder()->slot_count(), 0);
        vector<uint64_t> one_const(crypto_context_.encoder()->slot_count(), 1);

        crypto_context_.encoder()->encode(zero_const, zero_ptx);
        crypto_context_.encoder()->encode(one_const, one_ptx);
    }

    template <>
    int32_t BP::addItem(const PlaintextBits &item){
        item_.push_back(item);
        return item_.size();
    }

    Ciphertext BP::eval(
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
}



// void BranchingProgram::resetCost(){
//     queue<int> nodeQueue;
//     nodeQueue.push(0);

//     while (!nodeQueue.empty()){
//         int currentNode = nodeQueue.front();
//         nodeQueue.pop();
//         if (nodeLabel[currentNode] == -1){
//             nodeQueue.push(leftChild[currentNode]);

//             nodeQueue.push(rightChild[currentNode]);
//         }
//     }
// }

// void BranchingProgram::aggregate(){
//     queue<int> nodeQueue;
//     nodeQueue.push(0);
//     while (!nodeQueue.empty()){
//         int currentNode = nodeQueue.front();
//         nodeQueue.pop();
        
//         if (nodeLabel[currentNode] != -1) continue;

//         cost[leftChild[currentNode]] *= cost[currentNode];
//         nodeQueue.push(leftChild[currentNode]);

//         cost[rightChild[currentNode]] *= cost[currentNode];
//         nodeQueue.push(rightChild[currentNode]);
//     }
// }

// helib::Ctxt BinaryTree::evaluateLeaves(){
//     helib::Ctxt ans(pubKey);
//     helib::Ptxt<helib::BGV> ptxt(context);
//     for (int i = 0; i < nslots; i++) ptxt[i] = 0;
//     pubKey.Encrypt(ans, ptxt);

//     queue<int> nodeQueue;
//     nodeQueue.push(0);
//     while (!nodeQueue.empty()){
//         int currentNode = nodeQueue.front();
//         nodeQueue.pop();

//         if (nodeLabel[currentNode] != -1){
//             cost[currentNode] *= nodeLabel[currentNode];
//             ans += cost[currentNode];
//         }
//         else{
//             nodeQueue.push(leftChild[currentNode]);
//             nodeQueue.push(rightChild[currentNode]);
//         }
//     }

//     return ans;
// }

// helib::Ctxt BinaryTree::inSet(vector<helib::Ctxt> ctxts){
//     resetCost();
//     evaluate(ctxts);
//     aggregate();
//     helib::Ctxt ans = evaluateLeaves();
//     return ans;
// }

// void BranchingProgram::printTree(){
    // queue<TreeNode*> q;
    // queue<TreeNode*> new_q;
    // q.push(root);
    // int pt = 0;
    // while (!q.empty()){
    //     pt++;
    //     // if (pt == 5) break;
    //     cout << "\nNext floor:\n";
    //     while (!q.empty()){
    //         TreeNode* curr = q.front();
    //         cout << "(" << curr->id << ", label: " << curr->node_label << ", cost: " << curr->cost << ", child: ";
    //         q.pop();
    //         if (curr->left_child != NULL){
    //             new_q.push(curr->left_child);
    //             cout << curr->left_child->id << ", ";
    //         }
    //         else{
    //             cout << "NULL, ";
    //         }

    //         if (curr->right_child != NULL){
    //             new_q.push(curr->right_child);
    //             cout << curr->right_child->id << ", ";
    //         }
    //         else{
    //             cout << "NULL, ";
    //         }

    //         cout << "); ";
    //     }

    //     while (!new_q.empty()){
    //         q.push(new_q.front());
    //         new_q.pop();
    //     }
    // }

    // cout << "\n";
    // return;
// }