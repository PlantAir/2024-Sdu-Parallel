#include<stdio.h>
#include<omp.h>
#include<time.h>
#include<stdlib.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include "quick_pool.h"
#include <stack>
#include <queue>
Pool::Pool(int threadNum)
{
    pDestroyAll = false;
    CreateAll(threadNum);
}

//析构
Pool::~Pool()
{
    printf("Delete Pool\n");
    DestroyAll();
}

//线程所运行的函数.每个线程均应该执行ThreadFunc函数.
void* Pool::ThreadFunc(void *threadData)
{
    if ( threadData == nullptr ) {
        printf("Thread data is null.");
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


void Pool::CreateAll(int num)
{
    //初始化互斥锁pMutex和条件变量pCond,重置pTid <vector>数组大小
    pthread_mutex_init(&pMutex,NULL);
    pthread_cond_init(&pCond,NULL);
    pTid.resize(num);
    //使用pthread_create方法创建pthread线程
    for (int i=0;i<num;++i)
    {
        pthread_create(&pTid[i],NULL,ThreadFunc,(void *)this);
        printf("%d start",i);
    }
}

//向Pool中添加任务.
void Pool::AddTask(Task* task)
{
    //显然,AddTask和threadFunc之间有读写者问题,适用生产者-消费者(P-C)模型
    //添加任务时需要先给mutex上锁.保护临界区
    pthread_mutex_lock(&pMutex);
    //加入任务.
    pTaskList.push_back(task);
    //wait&signal机制.
    pthread_cond_signal(&pCond);
    //释放锁.
    pthread_mutex_unlock(&pMutex);
    //<?1>:这里unlock和signal的顺序可以颠倒吗?
}


void Pool::DestroyAll()
{
    if (!pDestroyAll)
    {
        //设置线程销毁标志,由销毁标志触发pthread_exit,退出线程
        pDestroyAll = true;
        printf("start to destroy");

        //唤醒所有线程,此时由于触发pDestroy=true条件的判断会依次自动销毁.
        pthread_cond_broadcast(&pCond);

        //清除cond,mutex,pTid
        for ( int i = 0;i<pTid.size();++i)
        {
            //pthread_join是以阻塞的方式等待指定线程结束.
            pthread_join(pTid[i],NULL);
            printf("Destroy thread %d",i);
        }
        pTid.clear();
        pthread_cond_destroy(&pCond);
        pthread_mutex_destroy(&pMutex);
    }
}


class qiuck_Partition : public Task
{
public:
    bool done;
    //实现Task中的virtual函数
    void Run()
    {
        std::vector<Data>* dataVecPtr = static_cast<std::vector<Data>*>(pData);
        std::vector<Data> data = *dataVecPtr;
        Data temp = data[start_index];   //以第一个元素为基准
        while (start_index < end_index) {
          while (start_index < end_index && data[end_index].score >= temp.score)end_index--;   //找到第一个比基准小的数
          data[start_index] = data[end_index];
          while (start_index < end_index && data[start_index].score <= temp.score)start_index++;    //找到第一个比基准大的数
          data[end_index] = data[start_index];
        }
        data[start_index] = temp;   //以基准作为分界线
        //return start_index;
    }

};

// Structure to hold data
struct Data {
    int id;
    int age;
    float weight;
    float height; // Additional attribute
    std::string name;
    std::string address;
    std::string email;
    std::string phone;
    std::string city;
    std::string country;
    int salary;
    int years_of_experience; // Additional attribute
    bool employed; // Additional attribute
    float rating; // Additional attribute
    float score; // Additional attribute
};
int Partition(std::vector<Data>& data, int start, int end)   //划分数据
{
    Data temp = data[start];   //以第一个元素为基准
    while (start < end) {
        while (start < end && data[end].score >= temp.score)end--;   //找到第一个比基准小的数
        data[start] = data[end];
        while (start < end && data[start].score <= temp.score)start++;    //找到第一个比基准大的数
        data[end] = data[start];
    }
    data[start] = temp;   //以基准作为分界线
    return start;
}

void quickSort(std::vector<Data>& data, int start, int end)  //并行快排
{
    
    if (start >= end) {
        return;
    }
     Pool* pool = new Pool(8);
    std::queue<std::pair<int, int>> queue;
    queue.push({start, end});

    while (!queue.empty()) {
        int left = queue.front().first;
        int right = queue.front().second;
        queue.pop();

        int pivot = Partition(data, left, right);

        if (pivot - 1 > left) {
            queue.push({left, pivot - 1});
        }
        if (pivot + 1 < right) {
            queue.push({pivot + 1, right});
        }
    }
}


// Compare function for std::sort by score
bool compare_score(const Data & a, const Data & b) {
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
int main(int argc, char* argv[])
{
     if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <array_size>" << std::endl;
        return 1;
    }

    uint64_t array_size = std::strtoull(argv[1], nullptr, 10);
    if (array_size == 0) {
        std::cout << "Array size must be a positive integer" << std::endl;
        return 1;
    }
    int num = std::strtoull(argv[2], nullptr, 10);
    //int n = atoi(argv[2]), i;   //线程数
    //int size = atoi(argv[1]);   //数据大小
    //int* num = (int*)malloc(sizeof(int) * size);

    //double starttime = omp_get_wtime();
//     srand(time(NULL) + rand());   //生成随机数组
//     for (i = 0; i < size; i++)
//         num[i] = rand();
    
   
    std::vector<Data> array(array_size);
    std::vector<Data> arraystd(array_size);
    
    // Initialize data
    srand(29); // Seed the random number generator
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
    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();
    int num_thread=8;
    quickSort(array, 0, array.size()-1);
    // End timer
    auto end_time = std::chrono::high_resolution_clock::now();
    
    //std::cout << "end_time: " << end_time << " seconds" << std::endl;
    double time_spent = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();


    // Print time taken
    std::cout << "Time taken: " << time_spent << " seconds" << std::endl;
    if (validate_score(array))
        std::cout << "Validation: Array is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Array is not sorted correctly by score" << std::endl;
    //// Print the top ten results
    //std::cout << "Top ten results:" << std::endl;
    //for (int i = 0; i < std::min(10, static_cast<int>(array.size())); ++i) {
    //    std::cout << "ID: " << array[i].id << ", Age: " << array[i].age << ", Weight: " << array[i].weight 
    //              << ", Score: " << array[i].score << std::endl;
    //    // Print other attributes as needed
    //}

    //// Print the last ten results
    //std::cout << "Last ten results:" << std::endl;
    //int start_index = std::max(0, static_cast<int>(array.size()) - 10);
    //for (int i = start_index; i < array.size(); ++i) {
    //    std::cout << "ID: " << array[i].id << ", Age: " << array[i].age << ", Weight: " << array[i].weight 
    //              << ", Score: " << array[i].score << std::endl;
    //    // Print other attributes as needed
    //}
//std in order
    //copy origin array
    auto start_time_std = std::chrono::high_resolution_clock::now();
    std::sort(arraystd.begin(), arraystd.end(), compare_score);
    auto end_time_std = std::chrono::high_resolution_clock::now();
    double time_spent_std = std::chrono::duration_cast<std::chrono::duration<double>>(end_time_std - start_time_std).count();
    std::cout << "std Time taken: " << time_spent_std << " seconds" << std::endl;

    // Validate if the array is sorted correctly by score
    
     // Clean up
    if (validate_score(arraystd))
        std::cout << "Validation: Arraystd is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Arraystd is not sorted correctly by score" << std::endl;
    
    return 0;
}
