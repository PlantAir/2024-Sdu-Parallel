#include "merge.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
using namespace std;

//线程所运行的函数.每个线程均应该执行ThreadFunc函数.
void* Pool::ThreadFunc(void *threadData)
{
    if ( threadData == nullptr ) {
        return NULL;
    }
    //将'threadData'指针铸造为Pool指针'PoolData',以便使用Pool中的方法
    Pool* PoolData = (Pool*)threadData;
    while (true)
    {
        Task* pTask = nullptr;
        //抢夺锁以保护临界区
        pthread_mutex_lock(&PoolData->pMutex);
        //***************临界区开始**************
        while (PoolData->pTaskList.empty()&&!PoolData->pDestroyAll)
        {
            pthread_cond_wait(&PoolData->pCond,&PoolData->pMutex);
        }
        if (!PoolData->pTaskList.empty())
        {
            //如果TaskList不是空的,那么就干嘛呢?
            auto iter = PoolData->pTaskList.begin();
            pTask = *iter;
            PoolData->pTaskList.erase(iter);
        }
        //****************临界区结束**************
        pthread_mutex_unlock(&PoolData->pMutex);

        if (pTask!=nullptr)
        {
            pTask->Run();
            pTask->isFinished = true;
        }

        if (PoolData->pDestroyAll && PoolData->pTaskList.empty())
        {
            printf("Thread exit");
            pthread_exit(NULL);
            break;
        }
    }
    return NULL;
}


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

// Compare function for std::sort by score
bool compare_score(const Data& a, const Data& b) {
    return a.score < b.score;
}

// Validation function to check if the array is sorted correctly by score
bool validate_score(const std::vector<Data>& array) {
    for (size_t i = 1; i < array.size(); i++) {
        if (array[i].score < array[i - 1].score)
            return false; // Array is not sorted correctly
    }
    return true; // Array is sorted correctly
}

class BlockSort : public Task
{
public:
    bool done;
    //实现Task中的virtual函数
    void Run()
    {
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
        //pData是继承自Task的protected属性,通过setData直接设置
        if (pData != nullptr) {
            std::vector<Data>* dataVec = static_cast<std::vector<Data>*>(pData);
            std::sort(dataVec->begin()+start_index, dataVec->begin()+end_index, compare_score);
        }
        done = true;        //done变量用于判断Task是否已经完成,在Run()最后设置 = true
        //***********************************************************需要填补的第一个位置,如何操作自己的pData***********************************************************
    }

};

class MergeTask : public Task {
public:
    bool done;
    // 实现 Task 中的虚函数 Run
    void Run() override {
        std::vector<Data>* dataVec = static_cast<std::vector<Data>*>(pData);
        std::inplace_merge(dataVec->begin() + start_index,
                           dataVec->begin() + middle_index,
                           dataVec->begin() + end_index,
                           compare_score);
    done = true;
    }
};

int main(int argc, char *argv[]){

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <array_size>" <<" <thread_num>"<< std::endl;
        return 1;
    }
    uint64_t array_size = std::strtoull(argv[1],nullptr,10);
    uint64_t num_thread = std::strtoull(argv[2],nullptr,10);

    if (array_size == 0) {
        std::cout << "Array size must be a positive integer" << std::endl;
        return 1;
    }

    std::vector<Data> array(array_size);
    // Initialize data
    srand(time(NULL)); // Seed the random number generator
    for (uint64_t i = 0; i < array_size; i++) {
        array[i].id = i + 1;
        array[i].age = rand() % 100 + 1; // Random age between 1 and 100
        array[i].weight = static_cast<float>(rand()) / RAND_MAX * 100.0f; // Random weight between 0 and 100
        array[i].height = static_cast<float>(rand()) / RAND_MAX * 200.0f; // Random height between 0 and 200
        array[i].name = "Name" + std::to_string(i);
        array[i].address = "Address" + std::to_string(i);
        array[i].email = "Email" + std::to_string(i) + "@example.com";
        array[i].phone = "+1234567890" + std::to_string(i);
        array[i].city = "City" + std::to_string(i);
        array[i].country = "Country" + std::to_string(i);
        array[i].years_of_experience = rand() % 30; // Random years of experience between 0 and 29
        array[i].employed = rand() % 2; // Random employed status (0 or 1)
        array[i].rating = static_cast<float>(rand()) / RAND_MAX * 5.0f; // Random rating between 0 and 5
        array[i].score = static_cast<float>(rand()) / RAND_MAX * 11000000.0f - 1000000.0f; // Random score between -1000000 and 10000000
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    Pool* pool = new Pool(num_thread);
    int num_block = num_thread;

    BlockSort* pTaskList  = new BlockSort[num_block];
    uint64_t block_size = array_size / num_block;

    std::vector<uint64_t> ranges(num_block+1);
    ranges[0] = 0;
    // 为每个任务分配数据
    for (int index = 0; index < num_block; ++index) {
        // 计算当前任务应该处理的数据的起始索引和结束索引
        uint64_t start_index = index * block_size;
        uint64_t end_index = (index == num_block-1) ? array_size : (start_index + block_size);
        //添加归并的边界
        ranges[index+1] = end_index;
        // 为当前任务设置数据
        pTaskList[index].setData(&array,start_index,0,end_index);
        // 将当前任务添加到线程池
        pool->AddTask(&pTaskList[index]);
    }


    //简单轮询所有线程是否已经完成,显然可以优化
    while(true){
        bool flag = true;
        for(int i=0;i<num_block;i++){
             if ( !pTaskList[i].done ){
                flag = false;
             }          
        }
        if(flag){
            break;
        }
    }
    while(ranges.size()>2){
        uint64_t num_merge = (ranges.size()-1)/2;
        MergeTask* mergeList = new MergeTask[num_merge];
        
        for (int index = 0; index < num_merge; ++index) {  
            // 为当前任务设置数据
            mergeList[index].setData(&array,ranges[index],ranges[index+1],ranges[index+2]);
            ranges.erase(ranges.begin()+index+1);                
          // 将当前任务添加到线程池
            pool->AddTask(&mergeList[index]);
        }
         while(true){
            bool flag = true;
            for(int i=0;i<num_merge;i++){
                if ( !mergeList[i].done ){
                    flag = false;
                }          
            }
            if(flag){
                break;
            }
        }
    }
   
    // 激活析构
    // delete pool;
    // delete []pTaskList;

    // End timer
    auto end_time = std::chrono::high_resolution_clock::now();

    double time_spent = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();

    // Print time taken
    std::cout << "Time taken: " << time_spent << " seconds" << std::endl;
    if (validate_score(array))
        std::cout << "Validation: Array is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Array is not sorted correctly by score" << std::endl;


    return 1;
}