#ifndef __HTTPD_SERVER_HPP__         //后边一定要跟#endif
#define __HTTPD_SERVER_HPP__

#include<iostream>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include "Log.hpp"
#include"ProtocolUtil.hpp"
#include "ThreadPoll.hpp"

//server类
class HttpdServer{
	private:
		int listen_sock;
		int port;
                ThreadPool *tp;
	public:
		HttpdServer(int port_)   //构造函数
			   :port(port_),listen_sock(-1)
         	{} 

		//初始化服务器
		/*
		 *1.创建socket
		 *2.绑定地址
		 *3.监听
		 */
		void InitServer()
		{
			listen_sock = socket(AF_INET,SOCK_STREAM,0);
			if(listen_sock < 0)
			{
				LOG(ERROR,"create socket error");
				exit(2);
			}

                            
			int opt_ = 1;
			setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt_,sizeof(opt_));
			//绑定地址
			struct sockaddr_in local_;
			local_.sin_family = AF_INET;
			local_.sin_port = htons(port);
			local_.sin_addr.s_addr = INADDR_ANY;

			int ret = bind(listen_sock,(struct sockaddr*)&local_,sizeof(local_));
			if(ret < 0)
			{
				LOG(ERROR,"bind socket error");
				exit(3);
			}
			//监听
			if(listen(listen_sock,5) < 0)
			{
				LOG(ERROR,"listen socket error");
				exit(4);
			}
                                tp = new ThreadPool();
                                tp->InitThreadPool();
			LOG(INFO,"Init socket success!");
		}
		/*开始发起连接
		 *1.建立连接
		 *2.利用线程来接收
		 */
		void Start()
		{
			LOG(INFO,"start Server success!!");
			//建立连接
			for(;;)
			{
				struct sockaddr_in peer_;
				socklen_t len_ = sizeof(peer_);
				int sock_ = accept(listen_sock,(struct sockaddr*)&peer_,&len_);
				if(sock_ < 0)
				{
					LOG(WARNING,"accept socket error");
					continue;
				}
				
                                Task t;
                                t.SetTask(sock_,Entry::HandlerRequest);
                                tp->PushTask(t);
                                cout<<"id---->"<<pthread_self()<<endl;
                                LOG(INFO,"Get new client");
			
				
			}

		}
		//析构函数
		~HttpdServer(){
			if(listen_sock != -1)
				close(listen_sock);  //关闭listen_sock

			port = -1;

		}
};

#endif
