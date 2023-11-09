#include "branching_program.h"

using namespace std;
using namespace apsi;

namespace apsi{
    BP::BP(): 
        numNodes(0)
        {

        id.push_back(numNodes++);

        leftChild.push_back(-1);
        rightChild.push_back(-1);
        parent.push_back(-1);
        level.push_back(0);
        isLeaf.push_back(0);
    }

    int BP::addChildNode(int parentNode){
        id.push_back(numNodes++);

        leftChild.push_back(-1);
        rightChild.push_back(-1);
        parent.push_back(parentNode);
        level.push_back(level[parentNode] + 1);

        return numNodes-1;
    }


    void BP::addItem(const Item &item){
        int currentNode = 0;
        for (int bit = 0; bit < item.get_length(); bit++){
            if (item.get_bit(bit) == 0){
                if (leftChild[currentNode] == -1){
                    leftChild[currentNode] = addChildNode(currentNode);
                }
                currentNode = leftChild[currentNode];
            }
            else{
                if (rightChild[currentNode] == -1){
                    rightChild[currentNode] = addChildNode(currentNode);
                }
                currentNode = rightChild[currentNode];
            }
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

// void BranchingProgram::evaluate(vector<helib::Ctxt> ctxts){
//     queue<int> nodeQueue;
//     nodeQueue.push(0);
//     while (!nodeQueue.empty()){
//         int currentNode = nodeQueue.front();
//         nodeQueue.pop();

//         if (nodeLabel[currentNode] != -1) continue;

//         cost[leftChild[currentNode]] += ctxts[level[currentNode]];
//         cost[leftChild[currentNode]] += 1l;
//         nodeQueue.push(leftChild[currentNode]);

//         cost[rightChild[currentNode]] += ctxts[level[currentNode]];
//         cost[rightChild[currentNode]] += 1l;
//         nodeQueue.push(rightChild[currentNode]);
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