#include "branchingProgram.h"

BinaryTree::BinaryTree(
        int newMaxDepth): 
    numNodes(0),
    maxDepth(newMaxDepth)
    {
    /*
        Initialize binary tree.
        This tree is implemented in an array-like structure. 
        To initialize it, we create the first element of multiple arrays mentioned in the construction. This first element represents the root node of the tree.
    */

    id.push_back(numNodes++);

    leftChild.push_back(-1);
    rightChild.push_back(-1);
    parent.push_back(-1);
    level.push_back(0);
    isLeaf.push_back(0);

    nodeLabel.push_back(0);

    const helib::EncryptedArray& ea = context.getEA();
    nslots = ea.size();
}

void BinaryTree::addChildNode(int parentNode){
    id.push_back(numNodes++);

    leftChild.push_back(-1);
    rightChild.push_back(-1);
    parent.push_back(parentNode);
    level.push_back(level[parentNode] + 1);

    nodeLabel.push_back(0);

    helib::Ctxt newCtxt(pubKey);
    cost.push_back(newCtxt);
}


void BinaryTree::addNum(long long x){
    int currentNode = 0;
    for (int bit = 0; bit < maxDepth; bit++){
        if (nodeLabel[currentNode] == 0){ // when tree is currently leave
            leftChild[currentNode] = numNodes;
            addChildNode(currentNode);

            rightChild[currentNode] = numNodes;
            addChildNode(currentNode);

            nodeLabel[currentNode] = -1;
        }

        if (((x >> bit) & 1) ^ 1){
            currentNode = leftChild[currentNode];
        }
        else{
            currentNode = rightChild[currentNode];
        }
    }

    nodeLabel[currentNode] = 1;
}

void BinaryTree::resetCost(){
    helib::Ptxt<helib::BGV> zeroPtxt(context), onePtxt(context);  
    for (int i = 0; i < nslots; i++){
        zeroPtxt[i] = 0;
        onePtxt[i] = 1;
    }

    pubKey.Encrypt(cost[0], onePtxt);

    queue<int> nodeQueue;
    nodeQueue.push(0);

    while (!nodeQueue.empty()){
        int currentNode = nodeQueue.front();
        nodeQueue.pop();
        if (nodeLabel[currentNode] == -1){
            pubKey.Encrypt(cost[leftChild[currentNode]], zeroPtxt);
            nodeQueue.push(leftChild[currentNode]);

            pubKey.Encrypt(cost[rightChild[currentNode]], onePtxt);
            nodeQueue.push(rightChild[currentNode]);
        }
    }
}

void BinaryTree::evaluate(vector<helib::Ctxt> ctxts){
    queue<int> nodeQueue;
    nodeQueue.push(0);
    while (!nodeQueue.empty()){
        int currentNode = nodeQueue.front();
        nodeQueue.pop();

        if (nodeLabel[currentNode] != -1) continue;

        cost[leftChild[currentNode]] += ctxts[level[currentNode]];
        cost[leftChild[currentNode]] += 1l;
        nodeQueue.push(leftChild[currentNode]);

        cost[rightChild[currentNode]] += ctxts[level[currentNode]];
        cost[rightChild[currentNode]] += 1l;
        nodeQueue.push(rightChild[currentNode]);
    }
}

void BinaryTree::aggregate(){
    queue<int> nodeQueue;
    nodeQueue.push(0);
    while (!nodeQueue.empty()){
        int currentNode = nodeQueue.front();
        nodeQueue.pop();
        
        if (nodeLabel[currentNode] != -1) continue;

        cost[leftChild[currentNode]] *= cost[currentNode];
        nodeQueue.push(leftChild[currentNode]);

        cost[rightChild[currentNode]] *= cost[currentNode];
        nodeQueue.push(rightChild[currentNode]);
    }
}

helib::Ctxt BinaryTree::evaluateLeaves(){
    helib::Ctxt ans(pubKey);
    helib::Ptxt<helib::BGV> ptxt(context);
    for (int i = 0; i < nslots; i++) ptxt[i] = 0;
    pubKey.Encrypt(ans, ptxt);

    queue<int> nodeQueue;
    nodeQueue.push(0);
    while (!nodeQueue.empty()){
        int currentNode = nodeQueue.front();
        nodeQueue.pop();

        if (nodeLabel[currentNode] != -1){
            cost[currentNode] *= nodeLabel[currentNode];
            ans += cost[currentNode];
        }
        else{
            nodeQueue.push(leftChild[currentNode]);
            nodeQueue.push(rightChild[currentNode]);
        }
    }

    return ans;
}

helib::Ctxt BinaryTree::inSet(vector<helib::Ctxt> ctxts){
    resetCost();
    evaluate(ctxts);
    aggregate();
    helib::Ctxt ans = evaluateLeaves();
    return ans;
}

void BinaryTree::printTree(){
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
}