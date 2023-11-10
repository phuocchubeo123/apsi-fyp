#pragma once

#include <vector>
#include <queue>

// APSI
#include "apsi/item.h"

namespace apsi{
    struct BP{  
    public:
        int numNodes;
        int numLeaves;

        std::vector<int> id;

        std::vector<int> leftChild;
        std::vector<int> rightChild;
        std::vector<int> parent;
        std::vector<int> level;
        std::vector<int> isLeaf;

        /*
            Initialize binary tree.
            This tree is implemented in an array-like structure. 
            To initialize it, we create the first element of multiple arrays mentioned in the construction. This first element represents the root node of the tree.
        */
        BP();

        int addChildNode(int parentNode);
        int32_t addItem(const Item& item);
    }; 
}