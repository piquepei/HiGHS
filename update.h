//
//  update.h
//  Update
//
//  Created by pique on 2020/12/28.
//  Copyright © 2020 pique. All rights reserved.
//

#ifndef update_h
#define update_h

#include <vector>
#include<algorithm>
#include "CZLPVector.h"

using std::vector;
using std::copy;

class Update{
public:
    Update(int RowNum,int ColNum);
    void copy_L(vector<int> LpivotLookUp,vector<int> LpivotIndex,vector<int> LStart,vector<int> LIndex,vector<double> LValues,
    vector<int> LRStart,vector<int> LRIndex,
    vector<double> LRValues);
    void copy_U(    vector<int> UpivotLookUp,vector<int> UpivotIndex,vector<double> UpivotValues,
    vector<int> UStart,vector<int> UIndex,vector<double> UValues,
    vector<int> URStart,vector<int> URIndex,vector<double> URValues,vector<int> UEnd,vector<int> UREnd,vector<int> URSpace);
    void updateFT(int out,int in,CZLPVector& aq,CZLPVector& ep);
private:
    int RowNum;
    int ColNum;
    int ColumnIn;//入基的列
    int ColumnOut;//出基列
    int step;//更新次数
    int flag;//是否需要重新分解
    
    //L矩阵
    vector<int> LpivotLookUp;
    vector<int> LpivotIndex;
    
    vector<int> LStart;
    vector<int> LIndex;
    vector<double> LValues;
    
    vector<int> LRStart;
    vector<int> LRIndex;
    vector<double> LRValues;
    
    //U矩阵
    vector<int> UpivotLookUp;
    vector<int> UpivotIndex;
    vector<double> UpivotValues;
    
    vector<int> UStart;
    vector<int> UEnd;
    vector<int> UIndex;
    vector<double> UValues;
    
    vector<int> URStart;
    vector<int> UREnd;
    vector<int> URSpace;
    vector<int> URIndex;
    vector<double> URValues;
    
    //R矩阵
    vector<int> RpivotValues;
    vector<int> RpivotLookUp;
    vector<int> RIndex;
    vector<int> RStarts;
    vector<double> RValues;
};


#endif /* update_h */
