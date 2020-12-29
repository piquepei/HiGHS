//
//  update.cpp
//  Update
//
//  Created by pique on 2020/12/28.
//  Copyright © 2020 pique. All rights reserved.
//

#include "update.h"

Update::Update(int RowNum,int ColNum){
    this->RowNum=RowNum;
    this->ColNum=ColNum;
    step=0;
    flag=false;
}

void Update::copy_L(vector<int> LpivotLookUp,vector<int> LpivotIndex,vector<int> LStart,vector<int> LIndex,vector<double> LValues,
vector<int> LRStart,vector<int> LRIndex,
                    vector<double> LRValues){
    this->LpivotLookUp=LpivotLookUp;
    this->LpivotIndex=LpivotIndex;
    this->LStart=LStart;
    this->LIndex=LIndex;
    this->LValues=LValues;
    this->LRStart=LRStart;
    this->LRIndex=LRIndex;
    this->LRValues=LRValues;
}
void Update::copy_U(vector<int> UpivotLookUp,vector<int> UpivotIndex,vector<double> UpivotValues,
vector<int> UStart,vector<int> UIndex,vector<double> UValues,
            vector<int> URStart,vector<int> URIndex,vector<double> URValues,
                    vector<int> UEnd,vector<int> UREnd,vector<int> URSpace){
    this->UpivotLookUp=UpivotLookUp;
    this->UpivotIndex=UpivotIndex;
    this->UpivotValues=UpivotValues;
    this->UStart=UStart;
    this->UIndex=UIndex;
    this->UValues=UValues;
    this->URStart=URStart;
    this->URIndex=URIndex;
    this->URValues=URValues;
    this->UEnd=UEnd;
    this->UREnd=UREnd;
    this->URSpace=URSpace;
}
void Update::updateFT(int out,int in,CZLPVector& aq,CZLPVector& ep){
    ColumnOut=out;
    ColumnIn=in;
    
    int ColumnP=UpivotLookUp[ColumnOut];
    double pivotvalue=UpivotValues[ColumnP];
    
    //删除第p列
    for(int i=UStart[ColumnP];i<UEnd[ColumnP];i++){
        int back=UpivotLookUp[i];
        for(int k=URStart[back];k<UREnd[back];k++){
            if(URIndex[k]==ColumnOut){
                int iEnd=UREnd[back]-1;
                URIndex[k]=URIndex[iEnd];
                URValues[k]=URValues[iEnd];
                UREnd[back]--;
                URSpace[back]++;
                break;
            }
        }
    }
    //删除第p行
    for(int i=URStart[ColumnP];i<UREnd[ColumnP];i++){
        int back=UpivotLookUp[i];
        for(int k=UStart[back];k<UEnd[back];k++){
            if(UIndex[k]==ColumnOut){
                int iEnd=UEnd[back]-1;
                UIndex[k]=UIndex[iEnd];
                UValues[k]=UValues[iEnd];
                UEnd[back]--;

                break;
            }
        }
    }
    //将新入基的列放到U的最后一列
    UStart.push_back(UIndex.size());
    for(int i=0;i<aq.get_nonzeronum();i++){
        if(i!=out){
            UIndex.push_back(aq.Index[i]);
            UValues.push_back(aq.PackedArray[i]);
    }
    
    }
    UEnd.push_back(UIndex.size());
    
    int Startq=UStart.back();
    int Endq=UEnd.back();
    
    //将新入基的列加入到对应的行存储中
    for(int k=Startq;k<Endq;k++){
        int iRow=UpivotLookUp[UIndex[k]];
        //如果该行没有多余空间则在后面开辟新空间
        if(URSpace[iRow]==0){
            int NewStart=URIndex.size();
            UpivotLookUp[iRow];
            
        }
        URIndex[Endq]=k;//?????
        URValues[Endq]=UValues[k];
        URSpace[iRow]--;
        UREnd[iRow]++;
    }
    //?????
    int newP,newEnd,newURSpace;
    URStart[newP]=NewStart;
    UREnd[newP]=newEnd;
    URSpace[newP]=newURSpace;
    
    //储存R矩阵
    
}
