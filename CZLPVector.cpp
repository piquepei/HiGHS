//
//  CZLPVector.cpp
//  Update
//
//  Created by pique on 2020/12/29.
//  Copyright © 2020 pique. All rights reserved.
//

#include "CZLPVector.h"



CZLPVector::CZLPVector(int size)
{
    initial(size);
}

CZLPVector::~CZLPVector()
{
    clear();
}

void CZLPVector::initial(int size)
{
    Size = size;              //向量维数
    NonZeroNum = 0;              //初始化非零元素个数
    IsPacked = true;              //初始化为需要被压缩
    Array.assign(size, 0);      //初始化原向量
    Index.resize(size);          //初始化非零元下标
    PackedArray.resize(size); //初始化压缩后的向量
}

void CZLPVector::set(const double *a, int size)
{
    initial(size);
    if (size < Size)
        throw "The array size is too small!";
    else if (size > Size)
        throw "The array size is too large!";
    for (int i = 0; i < Size; ++i)
    {
        Array[i] = (a[i]);
        if (a[i] != 0)
        {
            Index[NonZeroNum] = i;
            NonZeroNum++;
        }
    }
    IsPacked = true;
    pack();
}

void CZLPVector::clear()
{
    //Array.assign(Size, 0); //清空原向量
    Size = 0;        //向量置0
    NonZeroNum = 0; //清空非零元素个数
    IsPacked = false; //重置压缩状态为不需要被压缩
}

void CZLPVector::print_array()
{
    std::cout << "Array: ";
    for (int i = 0; i < Size; i++)
    {
        std::cout << Array[i] << " ";
    }
    std::cout << std::endl;
}

void CZLPVector::print_packed_array()
{
    std::cout << "Packed Array: ";
    for (int i = 0; i < NonZeroNum; i++)
    {
        std::cout << PackedArray[i] << " ";
    }
    std::cout << std::endl;
}

void CZLPVector::print_index()
{
    std::cout << "Index: ";
    for (int i = 0; i < NonZeroNum; i++)
    {
        std::cout << Index[i] << " ";
    }
    std::cout << std::endl;
}


const int CZLPVector::get_nonzeronum()
{
    return NonZeroNum;
}

const int CZLPVector::get_size()
{
    return Size;
}

void CZLPVector::pack()
{
    if (IsPacked)
    {
        IsPacked = false;
        for (int i = 0; i < NonZeroNum; i++)
        {
            const int index = Index[i];       //取非零元下标
            PackedArray[i] = Array[index]; //赋值
        }
    }
}

void CZLPVector::set_size(int size){
    Size = size;
}

void CZLPVector::set_nonzeronum(int n)
{
    NonZeroNum = n;
}

void CZLPVector::set_ispacked(bool i)
{
    IsPacked = i;
}

