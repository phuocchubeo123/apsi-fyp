#include "branching_program.h"

using namespace std;
using namespace apsi;
using namespace seal;

namespace apsi{
    BP::BP(): nodes_count_(1), leaves_count_(0){
        clear();
    }

    BP::BP(const CryptoContext &crypto_context): 
        nodes_count_(1), leaves_count_(0), crypto_context_(crypto_context);
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
        return leaves_count_;
    }

    void BP::eval(
        vector<Ciphertext> &ciphertext_bits, 
        MemoryPoolHandle &pool) const
        {
    #ifdef SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT
            static_assert(
                false, "SEAL must be built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF");
    #endif
        auto seal_context = crypto_context_.seal_context();
        auto evaluator = crypto_context_.evaluator();

        uint32_t depth = ciphertext_bits.size();
        queue<int> node_queue;
        node_queue.push(0);

        vector<vector<Ciphertext>> path_costs(depth+1);
        for (int d = 0; d < depth; d++){
            int uint32_t max_sub_path;
            for (int l = 0; (((d+1) >> l) << l) == d+1; l++) max_sub_path = l;
            path_costs[d].resize(max_sub_path);
        }

        while (!nodeQueue.empty()){
            int cur = node_queue.front();
            node_queue.pop();

            if (cur > 0){
                long side = (leftChild[parent[cur]] == cur) ? 0 : 1;
                Plaintext plain_addend(-side);
                path_costs[level[cur]][0] = ciphertext_bits[level[cur]];
                evaluator.sub_inplace(path_costs[level[cur]][0], plain_addend);
                evaluator.square(path_costs[level[cur]][0]);
            }

            if (left_child_[cur] != -1) node_queue.push(left_child_[cur]);
            if (right_child_[cur] != -1) node_queue.push(right_child_[cur]);
        }
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