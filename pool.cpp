//定义了Pool类和Task类
#include "pool.h"
// <note1>:
// 所有pthread开头的函数均来自pthread.h
// 除pTask外所有p开头均为pool.h自定义的变量,包括pTid,pMutex,pCond,pData,pTaskList.详见.h注释
// pTask在Pool.cpp中定义,是pTaskList中的元素

// <note2>:
// 在 "parallel2024:结构体排序优化" 任务中,
// 基础的优化思路:
// 我们可以考虑整体使用归并,块内使用快排来进行.
// 假设总共划分成x个向量化块并以尽可能高的cache命中率快排,那么一轮排序后可以休眠x/2个线程.
// 考虑:使用Task类实现中的内联函数:waitTask来阻塞/挂起(休眠)线程.详见pool.h

// <note3>:
// 主要学习和使用的知识:
// os basic knowledge: 线程,临界区,互斥和共享
// c++ basic knowledge: .h,.cpp的编写和划分
// c++ basic knowledge: 析构和初始化
// pthread Cond & pthread Mutex: 条件变量和互斥锁
// pthread create & pthread join & pthread exit: 创建,阻塞等待和销毁进程

//初始化threadNum个线程数的Pool.
Pool::Pool(int threadNum)
{
    pDestroyAll = false;
    CreateAll(threadNum);
}

//析构
Pool::~Pool()
{
    printf("Delete Pool");
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
            // <note3-1/> : pthread cond
            // cond(条件变量)类似于更高级的信号量.
            // 一般通过signal,wait实现,还可以broadcast.

            // pthread_cond_wait函数会永久阻塞进程直到'条件满足'(条件变量通过signal/broadcast成立).

            // 使用pthread_cond_signal至少会解除一个线程的"被阻塞"状态
            // 假设此时有x个进程处于cond_wait同一个cond变量的队列中,被解除的线程将由OS调度决定.
            // (signal即信号量中的"+1"操作).

            //pthread_cond_broadcast会解除等待同一cond var队列中所有线程的被阻塞状态.
            // </note3-1>

            // <note3-2/> : pthread cond&mutex联用
            // 一般来说,cond必须和mutex联用.
            // 在多线程编程中,以原子操作方式完成"阻塞线程+解锁"和"加锁+解除阻塞"两个过程.
            // 阻塞/解除阻塞和cond有关,解锁和加锁由mutex控制.

            // 即便满足了cond,仍然需要抢夺锁.为什么?

            // ① 保护临界区,避免race condition
            // 在多线程任务中,多线程共享数据/etc,往往存在临界区问题
            // 仅凭cond变量没办法保证临界区内同时只存在一个线程.
            // 倘使我们使用了第一个signal,我们其实不知道这个被唤醒的线程其何时完成任务
            // (os没办法预测任务完成的时间,因此才会有CPU burst"间断性涌现"的现象)
            // 联用多个signal和使用broadcast就更不必说了.

            // ②进程可能被意外唤醒
            // 进程不一定是被cond_signal或cond_broadcast唤醒.
            // 事实上,尽管cond_wait永久阻塞了线程,但这种阻塞并非具有最高的优先级.
            // os似乎有更高优先级的办法虚假唤醒进程.就像我们可以使用控制台信号ctrl+c强行终止一样.
            // </note3-2>

            //挂起并等待pCond直到signal/broadcast被发送,同时释放锁(mutex)
            //事实上,如果不使用cond_wait,这里需要写一个轮询来不断获取锁.
            //在这里使用cond_wait则只需要被动等待signal,大大减小抢夺锁所需的时间.
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



