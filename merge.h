#include <unistd.h>
#include <string>
#include <list>
#include <vector>
#include <pthread.h>
using std::list;
using std::vector;

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

class Task
{
private:
    //使得Pool类能够访问Task类的私有和保护成员
    friend class Pool;

protected:
    //pData代表Task内部受操作的Data.在本例中是切割后的待排序defined struct块
    void** pData;
    void* realData;
    void* sortedData;
    uint64_t end_index,start_index,middle_index;
    //内联函数,调用时直接展开为定义内容(即"isFinished=false这两句")
    inline void initTask()
    {
        //初始化(Task完成标志和pData)
        isFinished = false;
        pData = nullptr;
    }

public:
    //可被操作的Task完成标志
    bool isFinished;
    //初始化函数
    Task()
    {
        initTask();
    }
    //必须实现的Run()函数,代表该Task的操作内容
    virtual void Run() = 0;
    //内联函数,设置pData
    inline void setData(void** data,uint64_t start,uint64_t mid,uint64_t end)
    {
        pData = data;
        end_index = end;
        start_index = start;
        middle_index = mid;
    }
    inline void setPinData(void** data,uint64_t start,uint64_t mid,uint64_t end,void* real_data,void* sorted_data)
    {
        pData = data;
        sortedData = sorted_data;
        realData = real_data;
        end_index = end;
        start_index = start;
        middle_index = mid;
    }
    //内联函数,提供阻塞线程的方法
    inline int waitTask()
    {
        if (!isFinished)
        {
            while (!isFinished)
            {
                sleep(5);
            }
        }
        return 0;
    }
};

class Pool
{
public:
    //任务列表,存储所有需要安排给线程的任务.
    list<Task*> pTaskList;
    //用于同步的互斥锁
    pthread_mutex_t pMutex;
    // 条件变量.通过条件变量来避免每个线程对临界区变量的轮询.
    // 详见ThreadFunc成员函数
    pthread_cond_t pCond;
    // 线程池销毁的标志.
    // note:在parallel2024中我们可以考虑直接在最后一次归并前再销毁线程池
    bool pDestroyAll;
    // 线程ID数组
    vector<pthread_t> pTid;

protected:
    // 销毁所有线程
    void DestroyAll();
    // 创建所有线程
    void CreateAll(int threadNum);

public:
    static void* ThreadFunc(void* threadData);
    Pool(int threadNum);
    virtual ~Pool();
    void AddTask(Task* t);
    int GetTaskSize();
};

