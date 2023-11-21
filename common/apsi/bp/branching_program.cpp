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

        crypto_context_.encoder()->encode(zero_const, tot_ptx);
        crypto_context_.encoder()->encode(zero_const, zero_ptx);
        crypto_context_.encoder()->encode(one_const, zero_ptx);
    }

    int BP::addChildNode(int parent_node){
        id.push_back(nodes_count_++);

        left_child_.push_back(-1);
        right_child_.push_back(-1);
        parent_.push_back(parent_node);
        level_.push_back(level_[parent_node] + 1);
        is_leaf_.push_back(0);

        return nodes_count_-1;
    }

    template <>
    int32_t BP::addItem(const Item &item){
        int cur = 0;
        for (int bit = 0; bit < item.get_length(); bit++){
            if (item.get_bit(bit) == 0){
                if (left_child_[cur] == -1){
                    left_child_[cur] = addChildNode(cur);
                }
                cur = left_child_[cur];
            }
            else {
                if (right_child_[cur] == -1){
                    right_child_[cur] = addChildNode(cur);
                }
                cur = right_child_[cur];
            }
        }
        is_leaf_[cur] = 1;
        if (cur == nodes_count_-1) leaves_count_++;
        APSI_LOG_DEBUG(leaves_count_ << " " << nodes_count_ << " " << left_child_[0] << " " << right_child_[0]);
        return leaves_count_;
    }

    template <>
    int32_t BP::addItem(const PlaintextBits &item){
        item_.push_back(item);
        return item_.size();
    }

    // Ciphertext BP::eval(
    //     const vector<Ciphertext> &ciphertext_bits, 
    //     const CryptoContext &crypto_context,
    //     MemoryPoolHandle &pool) const
    //     {
    // #ifdef SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT
    //         static_assert(
    //             false, "SEAL must be built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF");
    // #endif
    //     auto seal_context = crypto_context.seal_context();
    //     auto evaluator = crypto_context.evaluator();
    //     auto relin_keys = crypto_context.relin_keys();

    //     if (nodes_count_ == 1){
    //         APSI_LOG_INFO("Empty BP");
    //         Ciphertext cost = ciphertext_bits[0];
    //         evaluator->multiply_plain_inplace(cost, zero_ptx);
    //         evaluator->relinearize_inplace(cost, *relin_keys, pool);
    //         return cost;
    //     }

    //     uint32_t depth = ciphertext_bits.size();
    //     vector<vector<Ciphertext>> path_costs(depth+1);
    //     for (int d = 0; d < depth; d++){
    //         uint32_t max_sub_path;
    //         for (int l = 0; (((d+1) >> l) << l) == d+1; l++) max_sub_path = l;
    //         path_costs[d].resize(max_sub_path + 1);
    //         // APSI_LOG_INFO("Num paths: " << d << " " << max_sub_path + 1);
    //     }

    //     queue<int> node_queue;
    //     node_queue.push(0);
    //     bool met_leaf = false;

    //     Ciphertext tot_ctx;

    //     while (!node_queue.empty()){
    //         int cur = node_queue.front();
    //         node_queue.pop();

    //         int level = level_[cur];

    //         // APSI_LOG_INFO("Processing node " << cur << " at level " << level);

    //         if (cur > 0){
    //             bool side = (left_child_[parent_[cur]] == cur) ? 0 : 1;
    //             // APSI_LOG_INFO("Side: " << side);

    //             // APSI_LOG_INFO("Set path cost, recall that there are " << path_costs[level].size() << " costs to calculate");
    //             path_costs[level][0] = ciphertext_bits[level];
    //             evaluator->sub_plain_inplace(path_costs[level][0], (side == 0) ? zero_ptx : one_ptx);
    //             // APSI_LOG_INFO("Subtracted");

    //             evaluator->square_inplace(path_costs[level][0], pool);
    //             // APSI_LOG_INFO("Squared");

    //             evaluator->relinearize_inplace(path_costs[level][0], *relin_keys, pool);
    //             // APSI_LOG_INFO("Relinearized");

    //             for (int l = 1; l < path_costs[level].size(); l++){
    //                 path_costs[level][l] = path_costs[level-(1<<(l-1))][l-1];
    //                 evaluator->multiply_inplace(path_costs[level][l], path_costs[level][l-1]);
    //                 evaluator->relinearize_inplace(path_costs[level][l], *relin_keys, pool);
    //             }
    //         }

    //         if (is_leaf_[cur] == 1){
    //             if (met_leaf) evaluator->add_inplace(tot_ctx, path_costs[level].back());
    //             else{
    //                 tot_ctx = path_costs[level].back();
    //                 met_leaf = true;
    //             }
    //         }

    //         if (left_child_[cur] != -1) node_queue.push(left_child_[cur]);
    //         if (right_child_[cur] != -1) node_queue.push(right_child_[cur]);
    //     }

    //     return tot_ctx;
    // }

    Ciphertext BP::eval(
        const vector<Ciphertext> &ciphertext_bits, 
        const CryptoContext &crypto_context,
        MemoryPoolHandle &pool) const
    {
        auto seal_context = crypto_context.seal_context();
        auto evaluator = crypto_context.evaluator();
        auto relin_keys = crypto_context.relin_keys();

        Ciphertext tot_ctx;

        uint32_t depth = ciphertext_bits.size();
        vector<vector<Ciphertext>> path_costs(depth);
        for (int d = 0; d < depth; d++){
            uint32_t max_sub_path;
            for (int l = 0; (((d+1) >> l) << l) == d+1; l++) max_sub_path = l;
            if (max_sub_path > 5) max_sub_path = 5;
            path_costs[d].resize(max_sub_path + 1);
            APSI_LOG_DEBUG("Num paths: " << d << " " << max_sub_path + 1);
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
                evaluator->sub_plain_inplace(path_costs[level][0], ptx);
                evaluator->square_inplace(path_costs[level][0], pool);
                evaluator->relinearize_inplace(path_costs[level][0], *relin_keys, pool);
                // evaluator->mod_switch_to_next_inplace(path_costs[level][0]);
                evaluator->negate_inplace(path_costs[level][0]);
                evaluator->add_plain_inplace(path_costs[level][0], one_ptx);
            }

            for (int level = 1; level < depth; level++){
                for (int l = 1; l < path_costs[level].size(); l++){
                    path_costs[level][l] = path_costs[level][l-1];
                    evaluator->multiply_inplace(path_costs[level][l], path_costs[level-(1<<(l-1))][l-1]);
                    evaluator->relinearize_inplace(path_costs[level][l], *relin_keys, pool);
                    // evaluator->mod_switch_to_next_inplace(path_costs[level][l]);
                }
                // evaluator->add_inplace(path_costs[level][0], path_costs[level-1][0]);
            }

            if (met_leaf){
                evaluator->add_inplace(tot_ctx, path_costs[depth-1].back());
                // evaluator->add_inplace(tot_ctx, path_costs[depth-1][0]);
            } else{
                tot_ctx = path_costs[depth-1].back();
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