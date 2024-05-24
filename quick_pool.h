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
    //使得Pool类能够访问Task类的私有和保护成员
    friend class Pool;

protected:
    //pData代表Task内部受操作的Data.在本例中是切割后的待排序defined struct块
    void* pData;
    uint64_t end_index,start_index;
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
    inline void setData(void* data, uint64_t start, uint64_t end)
    {
        pData = data;
        end_index=end;
        start_index=start;
    }
    //内联函数,提供阻塞线程的方法
    inline int waitTask()
    {
        if (!isFinished)
        {
            while (!isFinished)
            {
                //挂起当前进程,在当前示例中可以代表进程任务已经完成.
                //显然,挂起当前进程可以节约抢占锁的时间.
                //当前场景中,所需线程数无疑是线性减小的,所以可以直接阻塞到整个任务都完成为止.
                //所有任务都完成后,唤醒并销毁所有线程(这会带来额外的开销吗?)
                //在较为通用的场景中,
                //最适线程数往往是非线性波动的,
                //如果我们还需要自动唤醒进程,
                //可以考虑使用较低秒数的 usleep / nanosleep 来精确控制到毫秒或纳秒
                sleep(5);
            }
        }
        return 0;
    }
};


// 线程池类.
// 提高性能的逻辑:若任务足够多,那么为每个任务单独创建,销毁线程的开销也会很大.
// 线程池并不维护每个线程的所有状态.
// 如果我们监控所有线程,对于每个新到来的任务都匹配到一个空闲的线程中,
// 那么获取线程状态,结果和同步管理会比较麻烦;
// 如果使用任务视角就会简单且通用很多.
// 我们不监控线程,只监控任务队列.
// 在开始时启动x个线程并挂起,有任务时进行唤醒.这也是OS中常用的逻辑.
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