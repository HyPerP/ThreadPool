#include "ThreadPool.h"   
#include <cstdio>         
#include <stdlib.h>         
#include <unistd.h>
 
class Test:public Task{
    public:
        Test()=default;
        void setnum(int x,int y){
            a=x;
            b=y;
        }
      	int Run(){
            printf("Dataptr is %s\n",(char*)DataPtr);
            printf("a + b = %d\n",a+b);
            sleep(3);
            return 0;
        }
        ~Test(){};
    private:
    int a,b;
        
};
 
int main()
{
    Test test;
    char Tmp[]="using DataPtr!";
    test.setData((void* )Tmp);
    ThreadPool threadpool(3);   //开启三个线程
    
    for(int i=0;i<10;i++)
    {
        test.setnum(1,i);
        threadpool.AddTask(&test);
    }
    
    while(1){
        printf("There are still %d tasks need to handle\n",threadpool.getTaskSize());
        
       
        if(threadpool.getTaskSize()==0) //任务队列已经没有任务了
        {
            //清除线程池
            if(threadpool.StopAll()==-1)
            {
                printf("Thread pool is clear,exit\n");
                exit(0);
            }
        }
        sleep(2);
        printf("2 seconds later...\n");
    }
    return 0;
}