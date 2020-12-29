//
//  CZLPVector.h
//  Update
//
//  Created by pique on 2020/12/29.
//  Copyright © 2020 pique. All rights reserved.
//

#ifndef CZLPVECTOR_H
#define CZLPVECTOR_H

#include <iostream>
#include <vector>

class CZLPVector
{
public:
    CZLPVector(int size);
    ~CZLPVector();
    void initial(int size);                 //初始化向量
    void set(const double *a, int size); //赋值
    void pack();                         //压缩向量
    void clear();                         //清空向量
    void print_array();                     //输出Array
    void print_packed_array();             //输出PackedArray
    void print_index();                     //输出非零元素下标
    void add_value();
    const int get_nonzeronum();
    const int get_size();
    void set_size(int size);
    void set_nonzeronum(int n);
    void set_ispacked(bool i);

    std::vector<double> Array;         //原向量
    std::vector<int> Index;             //非零元下标
    std::vector<double> PackedArray; //压缩后的向量
private:
    int Size;                         //向量维数
    int NonZeroNum;                     //非零元素个数
    bool IsPacked;                         //是否需要被压缩
};

#endif
