#include <unistd.h>
#include <string>
#include <list>
#include <vector>
#include <pthread.h>
using std::list;
using std::vector;


struct TaskResult
{
    ////////////////////////////////////////
    int layer=-1;
    int start=-1;
    int end=-1;
    bool done=false;
    ////////////////////////////////////////
};


class Task
{
private:
    friend class Pool;    //使得Pool类能够访问Task类的私有和保护成员

protected:
    void* pData;    //pData代表Task内部受操作的Data.在本例中是切割后的待排序defined struct块
    Pool* pPool;    //增加了Pool指针属性
    uint64_t end_index,start_index,middle_index;

public:
    //初始化函数
    Task()
    {
        pData = nullptr;
        pPool = nullptr; 
    }

    //设置pData
    void setData(void* data,uint64_t start,uint64_t mid,uint64_t end)
    {
        pData = data;
        end_index = end;
        start_index = start;
        middle_index = mid;
    }

    //必须实现的Run()函数,代表该Task的操作内容
    virtual void Run() = 0;

    void setResult(int task_id,int task_layer,uint64_t start,uint64_t end)
    {
        pthread_mutex_lock(&pPool->pResult);
        ///////////////////////////////////////////////////////////
        TaskResult res;
        res.layer = task_layer;
        res.done = true;
        res.start = start;
        res.end = end;
        pPool->pResultList[task_id] = res;
        ///////////////////////////////////////////////////////////
        pthread_mutex_unlock(&pPool->pResult);
    }
};



class Pool
{
public:
    list<Task*> pTaskList;    //任务列表,存储所有需要安排给线程的任务.
    TaskResult* pResultList; //结果列表，存储所有任务返回的结果
    pthread_mutex_t pMutex;    //用于同步的互斥锁，需要抢这个锁的场景是每次操作TaskList时，Pool add和Thread erase
    pthread_mutex_t pResult;    //用于同步结果的互斥锁，抢这个锁的场景是每次有任务完成时通知pool进行修改
    pthread_cond_t pCond;// 条件变量.通过条件变量来避免每个线程对临界区变量的轮询.
    bool pDestroyAll;   // 线程池销毁的标志.
    vector<pthread_t> pTid;   // 线程ID数组

protected:
    // 销毁所有线程
    void DestroyAll()
    {
        if (!pDestroyAll)
        {
            pDestroyAll = true;//设置线程销毁标志,由销毁标志触发pthread_exit,退出线程
            
            pthread_cond_broadcast(&pCond);//唤醒所有线程,此时由于触发pDestroy=true条件的判断会依次自动销毁.
            
            for ( int i = 0;i<pTid.size();++i)//清除cond,mutex,pTid
            {
                
                pthread_join(pTid[i],NULL);//pthread_join是以阻塞的方式等待指定线程结束.
                //printf("Destroy thread %d",i);
            }
            pTid.clear();
            pthread_cond_destroy(&pCond);
            pthread_mutex_destroy(&pMutex);
        }
    };

    // 创建所有线程
    void CreateAll(int threadNum)
    {
    //初始化互斥锁pMutex和条件变量pCond,重置pTid <vector>数组大小
    pthread_mutex_init(&pResult,NULL);
    pthread_mutex_init(&pMutex,NULL);
    pthread_cond_init(&pCond,NULL);
    pTid.resize(threadNum);

    for (int i=0;i<threadNum;++i)//使用pthread_create方法创建pthread线程
    {
        pthread_create(&pTid[i],NULL,ThreadFunc,(void *)this);
        printf("%d start",i);
    }
    };

public:
    static void* ThreadFunc(void* threadData);
    Pool(int threadNum)
    {    
        pDestroyAll = false;
        CreateAll(threadNum);
    };
    ~Pool(){DestroyAll();};
    void AddTask(Task* task)
    {
        //显然,AddTask和threadFunc之间有读写者问题,适用生产者-消费者(P-C)模型
        //<?1>:这里unlock和signal的顺序可以颠倒吗?
        pthread_mutex_lock(&pMutex);//添加任务时需要先给mutex上锁.保护临界区
        task->pPool = this;//将Task的pPool指针指向addTask的Pool
        pTaskList.push_back(task);
        pthread_cond_signal(&pCond);//wait&signal机制.
        pthread_mutex_unlock(&pMutex);//释放锁.
        
    };
};