#ifndef __THREAD_POOL_HPP__
#define __THREAD_POLL_HPP__
/*线程池
 *1.线程池中有五个线程
 *2.利用条件变量
 *3.在队列总添加任务
 *  queue<Task> task_queue
 *4.加锁 
 *   pthread_mutex_t lock
 *5.唤醒
 *   pthread_cond_t cond
 *6.任务类
 *
 */
#include<iostream>
#include<pthread.h>
#include<queue>
#include<sys/time.h>
#include<unistd.h>
#define NUM 5
#include"Log.hpp"
using namespace std;

typedef int (*handler_t) (int);

//任务结构
class Task
{
	private:
		int sock_;
		handler_t handler_;
	public:
		Task()
		{
		    sock_ = -1;
		    handler_ = NULL;
		}
                void SetTask(int sock,handler_t handler)
                {
                  sock_ = sock;
                  handler_ = handler;
                }
		void Run()
		{
			handler_(sock_);
		}

		~Task()
		{}
};

//线程池
class ThreadPool
{
	private:
		int thread_total_num;
		int thread_idle_num;
		queue<Task> task_queue;
		pthread_mutex_t lock;
      		pthread_cond_t cond;
                bool isquit;
        private:
               void LockQueue()
                {
                 pthread_mutex_lock(&lock);
                }

                void UnLockQueue()
                {
                 pthread_mutex_unlock(&lock);
                }
                
                void GetTask(Task & t)
                {
                   t = task_queue.front();
                   task_queue.pop();
                }
                bool IsEmpty()
                {

                   if(task_queue.size() == 0)
                      return true;
                   return false;

                }

               void ThreadWait()
                {    
                     //如果线程池退出了则线程退出
                     if(isquit)
                     {
                     //退出前解锁
                     UnLockQueue();
                     thread_total_num--;
                     pthread_exit((void*)0);
                     }
                     pthread_cond_wait(&cond,&lock);
                     thread_idle_num++;
                }

                void WeakUpThread()
                {
                  pthread_cond_signal(&cond);
                  thread_idle_num--;
                }
                void WeakUpAllThread()
                {
                     
                      pthread_cond_broadcast(&cond);
                }

               //如果不是static会有this指针
               static void* Thread_Routine(void* arg)
                {
                    //分离线程
                    ThreadPool* tp = (ThreadPool*)arg;
                    pthread_detach(pthread_self()); 
                    while(1)
                     {
                       tp-> LockQueue();
                        while(tp->IsEmpty())
                        {
                          //等待
                         tp-> ThreadWait();
                        }
                        //取任务
                        Task t;
                        tp->GetTask(t);
                        //解锁
                       tp-> UnLockQueue();
                        //处理任务
                        t.Run();
                     }
                }
	public:
		ThreadPool(int num_ = NUM):thread_total_num(num_),thread_idle_num(0)
	        {
			pthread_mutex_init(&lock,NULL);
			pthread_cond_init(&cond,NULL);
	        }   

		void InitThreadPool()
		{
			int i = 0;
			for(i;i < NUM;i++)
			{
				pthread_t id[NUM];
				pthread_create(&id[i],NULL,Thread_Routine,this);
			}
                     
                    LOG(INFO,"create thread");
		}

                

                void PushTask(Task& t)
                {
                    //加锁
                    LockQueue();
                    //如果是需要退出了则不能在继续添加线程
                    if(isquit)
                      {
                        UnLockQueue();
                        return;
                      }
                    //添加任务
                    task_queue.push(t);
                    //唤醒线程
                    WeakUpThread();
                    //解锁
                    UnLockQueue();
                }
 
                void StopThread()
                {
                    //加锁
                    LockQueue();
                    //isqueue设置为true
                    isquit = true;
                    //解锁
                    UnLockQueue();
                    if(thread_idle_num > 0)
                      {
                        WeakUpAllThread();
                      }

                }
 
		~ThreadPool()
		{
			pthread_mutex_destroy(&lock);
			pthread_cond_destroy(&cond);
		}
};

#endif
