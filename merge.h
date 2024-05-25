#include <unistd.h>
#include <string>
#include <list>
#include <vector>
#include <pthread.h>
using std::list;
using std::vector;


class Task
{
private:
    friend class Pool;    //使得Pool类能够访问Task类的私有和保护成员

protected:

    void* pData;    //pData代表Task内部受操作的Data.在本例中是切割后的待排序defined struct块
    Pool* pPool;
    uint64_t end_index,start_index,middle_index;

public:
    //初始化函数
    Task(Pool* pool)
    {
        pData = nullptr;
        pPool = pool;
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
};


class Pool
{
public:

    list<Task*> pTaskList;    //任务列表,存储所有需要安排给线程的任务.
    pthread_mutex_t pMutex;    //用于同步的互斥锁
    pthread_cond_t pDone;
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
        pTaskList.push_back(task);
        pthread_cond_signal(&pCond);//wait&signal机制.
        pthread_mutex_unlock(&pMutex);//释放锁.
        
    };
};