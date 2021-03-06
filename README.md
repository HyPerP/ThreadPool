
# ThreadPool——c++

## 为什么要用线程池

目前大多数的网络服务器都是在单位时间内处理大量的请求，但每个请求的处理时间都相对较短。
传统的多线程方案中我们采用的服务器模型是接收到一个请求后立即创建一个新的线程来执行任务，执行完毕后线程退出，这就是“即时创建，即时销毁”的策略。尽管创建线程的时间远小于创建进程，但若是任务执行时间短，执行次数多的情况下，服务器的大部分时间就都浪费在创建与销毁进程上了。
我们可以设T1为线程创建时间，T2为任务执行时间，T3为线程销毁时间。
(T1+T3)/(T1+T2+T3)就是我们在创建销毁线程上所消耗的时间。由此我们不难发现，当执行任务所消耗的时间很小的时候，创建、销毁线程消耗的时间就越大。当我们频繁执行的时候，这笔开销将是巨大的。
此外，创建线程池可以减少创建的线程的个数。在传统方案中，如果如果同时请求数目为2000，那么最坏情况下，系统可能需要产生2000个线程。尽管这不是一个很大的数目，但是也有部分机器可能达不到这种要求。
因此线程池就是用来减小频繁创建线程所带来的开销的。

## 线程池原理

线程池采用预创建技术，在程序启动后，立即创建一定数量的线程并放入线程池容器中，这些线程都处于阻塞状态，只占用较小的内存，并不消耗CPU。当所有线程都在处理任务时，新添加的任务会被放入任务队列。在任务执行完毕后线程也不退出，而是继续保持在池中等待下一次的任务。程序结束，线程池自动销毁线程，回收系统资源。

## 实现
### 执行任务类
```c++
class Task
{
protected:
    void* DataPtr; //任务的具体数据
public:
    Task() = default;
    virtual int Run() = 0;    //运行函数
    void setData(void* data); //设置任务数据
    virtual ~Task() {}
};
```
### 线程池管理类
```c++
class ThreadPool
{
private:
    static vector<Task* > taskList; //任务列表
    static bool shutdown;           //线程关闭的标志
    int threadNum;                  //线程池中已启动的线程的数量
    pthread_t* pthread_id;          //线程列表

    static pthread_mutex_t threadMutex; //线程同步互斥锁
    static pthread_cond_t condition;    //线程同步条件变量

protected:
    static void* ThreadFunc(void* threadData); //新线程的回调函数
    int Create();                              //在线程池中创建线程

public:
    ThreadPool(int num);
    int AddTask(Task* task); //把任务添加到队列中
    int StopAll();           //退出线程池中的所有线程
    int getTaskSize();       //获取当前任务队列中的任务数
};
void Task::setData(void *data)
{
    DataPtr = data;
}
```
## 具体定义
```c++
//静态成员初始化
vector<Task* > ThreadPool::taskList;
bool ThreadPool::shutdown = false;
pthread_mutex_t ThreadPool::threadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::condition = PTHREAD_COND_INITIALIZER;

//线程管理类构造函数
ThreadPool::ThreadPool(int num)
{
    this->threadNum = num;
    printf("%d threads will be created.\n", num);
    Create();
}

//线程回调函数
void* ThreadPool::ThreadFunc(void* threadData)
{
    pthread_t tid = pthread_self();
    while (1)
    {
        pthread_mutex_lock(&threadMutex);
        //如果队列为空，等待新任务进入任务队列
        while (taskList.size() == 0 && !shutdown)
            pthread_cond_wait(&condition, &threadMutex);

        //关闭线程
        if (shutdown)
        {
            pthread_mutex_unlock(&threadMutex);
            printf("[tid: %lu]\texit\n", pthread_self());
            pthread_exit(NULL);
        }

        printf("[tid: %lu]\trun: \n", tid);
        vector<Task* >::iterator iter = taskList.begin();
        //利用迭代器取出任务并处理
        Task* task = *iter;
        if (iter != taskList.end())
        {
            task = *iter;
            taskList.erase(iter); //将任务移出任务列表
        }

        pthread_mutex_unlock(&threadMutex);

        task->Run(); //执行任务
        printf("[tid: %lu]\tSuspended\n", tid);
    }

    return (void* )0;
}

//向任务队列里添加任务并发出线程同步信号
int ThreadPool::AddTask(Task* task)
{
    pthread_mutex_lock(&threadMutex);
    taskList.push_back(task);
    pthread_mutex_unlock(&threadMutex);
    pthread_cond_signal(&condition);
    return 0;
}

//创建线程
int ThreadPool::Create()
{
    pthread_id = new pthread_t[threadNum];
    for (int i = 0; i < threadNum; i++)
        pthread_create(&pthread_id[i], NULL, ThreadFunc, NULL);

    return 0;
}

//停止所有线程
int ThreadPool::StopAll()
{
    //避免重复调用
    if (shutdown)
        return -1;
    printf("Stop all threads!\n\n");

    //唤醒所有等待线程，线程池也要销毁了
    shutdown = true;
    pthread_cond_broadcast(&condition);

    //清除存在的僵尸线程
    for (int i = 0; i < threadNum; i++)
        pthread_join(pthread_id[i], NULL);

    delete[] pthread_id;
    pthread_id = NULL;

    //销毁互斥量和条件变量
    pthread_mutex_destroy(&threadMutex);
    pthread_cond_destroy(&condition);

    return 0;
}

//获取当前队列中的任务数
int ThreadPool::getTaskSize()
{
    return taskList.size();
}
```
