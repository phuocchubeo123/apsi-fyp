#pragma once

#include <vector>

struct BinaryTree{  
public:
    int numNodes;
    int maxDepth;

    std::vector<int> id;

    std::vector<int> leftChild;
    std::vector<int> rightChild;
    std::vector<int> parent;
    std::vector<int> level;
    std::vector<int> isLeaf;

    BinaryTree(
        int newMaxDepth);
    void addChildNode(int parentNode);
    void addNum(long long x);
    void resetCost();
    // void evaluate(vector<helib::Ctxt> ctxts);
    void aggregate();
    // helib::Ctxt evaluateLeaves();
    // helib::Ctxt inSet(vector<helib::Ctxt> ctxts);

    void printTree();
}; 