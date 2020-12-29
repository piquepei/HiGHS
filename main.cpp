//
//  main.cpp
//  Update
//
//  Created by pique on 2020/12/28.
//  Copyright © 2020 pique. All rights reserved.
//

#include <iostream>
#include "update.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    Update c(4,3);
    
    vector<int> LpivotLookUp={1,1};
    vector<int> LpivotIndex={1,1};
    vector<int> LStart={2,1};
    vector<int> LIndex={3,2};
    vector<double> LValues={2,4};
    vector<int> LRStart={3,5};
    vector<int> LRIndex={2,4};
    vector<double> LRValues={2,5};
    
    c.copy_L(LpivotLookUp, LpivotIndex,LStart,LIndex,LValues,LRStart,LRIndex,LRValues);
    
    return 0;
}
