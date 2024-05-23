#include "pool.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
using namespace std;


struct Data {
    int id;
    int age;
    float weight;
    float height;
    std::string name;
    std::string address;
    std::string email;
    std::string phone;
    std::string city;
    std::string country;
    int salary;
    int years_of_experience;
    bool employed;
    float rating;
    float score;
};

class BlockSort : public Task
{
public:
    bool done;
    //实现Task中的virtual函数
    void Run()
    {
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
        //pData是继承自Task的protected属性,通过setData直接设置
        //示例:
        // std::sort(pData->?,pData->?, compare_func);


        done = true;        //done变量用于判断Task是否已经完成,在Run()最后设置 = true
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
    }

};




int main(int args, char* agrv[])
{

    Pool* pool = new Pool(5);
    BlockSort* pTaskList  = new BlockSort[5];
    for ( int index = 0; index < 5; ++index) {
        // *************************************需要填补的第二个位置,使用setData方法每个Task的pData**********************************************
        // set pData
        std::vector<Data>* array;
        // array 指向具体数据
        pTaskList[index].setData(array);
        pool->AddTask(&pTaskList[index]);//加入任务就会直接通过cond变量激活一个对应线程自动开始执行
        //*************************************需要填补的第二个位置,使用setData方法每个Task的pData**********************************************
    }



    //简单轮询所有线程是否已经完成,显然可以优化
    while ( true )
    {
        bool all_finished = true;
        for ( int flag = 0; flag < 5; ++flag)
        {
            if ( !pTaskList[flag].done )
                all_finished = false;
        }
        if (all_finished)
            break;
    }

    //激活析构
    delete pool;
    delete []pTaskList;
    return 1;
}








// get end clock

