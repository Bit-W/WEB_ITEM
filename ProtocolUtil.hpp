#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include<iostream>
#include<sstream>
#include<strings.h>
#include<string>
#include<unordered_map>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
	#include<unistd.h>
	#include<sys/types.h>
	#include<sys/socket.h>
	#include<sys/wait.h>
	#include<sys/stat.h>
	#include<sys/sendfile.h>
	#include<fcntl.h>

	using namespace std;


	#define NOT_FOUND 404
	#define OK 200
	#define WEB_ROOT "wwwroot"
	#define HOME_PAGE "index.html"
	#define HTTP_VERSION "http/1.0"


	unordered_map<string,string> suffix_map
	{
		{".html","text/html"},
		{".htm","text/html"},
		{".css","text/css"},
		{".js","application/x-javascript"}

	};
	//工具类
	class StringUtil
	{

		public:
			static void Make_Kv(unordered_map<string,string> kv_,string str_)
			{
				size_t pos = str_.find(": ");
				if(string:: npos == pos)
				{
					return;
				}

				string k_ = str_.substr(0,pos);
				string v_ = str_.substr(pos+2);
				kv_.insert(make_pair(k_,v_));

			}
			static string IntToString(int code)
			{
				stringstream ss;
				ss<<code;
				return ss.str();
			}
			static string CodeToDesc(int code)
			{
				switch(code)
				{
					case 200:
						return "OK";
					case 404:
						return "NOT_FOUND";
					default:
						return "UNKONW";
				}
			}

			static string SuffixToType(string &suffix_)
			{
				return suffix_map[suffix_];
			}

	};


	//HTTP请求
	class Request{
		public:
			string rq_line;  //请求首行
			string rq_head;  //请求头
			string blank;    //空行
			string rq_text;  //正文
		private:
			string method_;    //
			string uri_;
			string version_;  //版本
			bool cgi;         //POST肯定是cgi  GET中带有？则也是cgi
			string path;

			string param;
			int resource_size;
			unordered_map<string,string> head_kv;
			int content_length;
			string resource_suffix;
		public:
			Request()
				:blank("\n"),cgi(false),path(WEB_ROOT),resource_size(0),content_length(-1),resource_suffix(".html")
		{}
			int GetResourceSize()
			{
				return resource_size;
			}
			
			void SetResourceSize(int rs_)
			{
				resource_size = rs_;
			}
			
			string &GetParam()
			{
				return param;
			}
			string &GetSuffix()
			{
				return resource_suffix;
			}

			string& GetPath()
			{
				return path;
			}

			//解析请求行
			void RequestLineParse()
			{
				stringstream ss(rq_line);
				ss>>method_>>uri_>>version_;
				LOG(INFO,"requestlineparse success");
			}

			//判断是否带参并且获取资源，将资源和参数分开保存
			void UriParse()
			{
				LOG(INFO,"UriParse");
				if(strcasecmp(method_.c_str(),"GET") == 0)
				{
				    
					size_t pos_ = uri_.find('?');
					if(string::npos != pos_)
					{
						cgi = true;
						path += uri_.substr(0,pos_);
						param = uri_.substr(pos_ + 1);

					}
					else
					{
					      LOG(INFO,"GET uriparse");
						path += uri_;
					}
				}
				else
				{
					path += uri_;
				}
			  
				//判断path
				if(path[path.size() - 1] == '/')
				{
					LOG(INFO,"path size");
					path += HOME_PAGE;
				}
				LOG(INFO,"UriParse is success");
			}

			//判断是什么请求方法
			bool IsMethodLegal()
			{
				if(strcasecmp(method_.c_str(),"GET") == 0 || (cgi=(strcasecmp(method_.c_str(),"POST") == 0)))
				  {
					LOG(INFO,"ismethodlegal");
					return true;
				  }
				return false;
			}

			//判断资源是否存在
			bool IsPathLegal()
			{  
				struct stat st;
				if(stat(path.c_str(),&st) < 0)
				{
					LOG(WARNING,"path not found");
					return false;
				}
				else
				{
					LOG(INFO,"path is found");
					resource_size = st.st_size;
				       // cout<<"resource size------------->"<<resource_size<<endl; 
					if(S_ISDIR(st.st_mode))
					{
						path += '/';
						path += HOME_PAGE;
						
					}
					else  //判断是否为可执行文件
					{
						if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
							cgi = true;
					}
					size_t pos_ = path.rfind(".");
					if(string:: npos != pos_)
					{
						resource_suffix = path.substr(pos_);

					}
					return true;
				}

			}
			//将报头的键值对先拆分开来，然后在将每个键值对存放在unordered_map中
			bool RequestHeadParse()
			{
				size_t start = 0;
				while(start < rq_head.size())
				{
					size_t pos = rq_head.find('\n',start);
					if(string:: npos == pos)
						break;

					string sub_string_ = rq_head.substr(start,pos - start);   //将每个头的键值对先拆分开来
					if(!sub_string_.empty())
					{
						LOG(INFO,"substr is not empty");
						StringUtil::Make_Kv(head_kv,sub_string_);
					}
					else
					{
						LOG(INFO,"substr is empty");
						break;
					}		
					start = pos + 1;
				}
				  return true;
			}
			//获取正文长度
			int GetContentLength()
			{
				string cl_ = head_kv["Content-Length"];
				if(!cl_.empty())
				{
					stringstream ss(cl_);
					ss>>content_length;
				}
				LOG(INFO,"GetContentLength success");
				return content_length;

			}

			//判断是否需要读取正文
			bool IsNeedRecvText()
			{
				if(strcasecmp(method_.c_str(),"POST") == 0)
					return true;
				return false;
			}

			bool IsCgi()
			{
			       return cgi;
			}

			~Request()
			{

			}

	};

	//HTTP响应
	class Response{
		private:
			string rsp_line;  //响应首行
			string rsp_head;  //响应头
			string blank;     //空行
			string rsp_text;  //正文
		public:
			int code;
			int fd;
		public:
			Response()
				:blank("\n"),code(OK),fd(-1)
			{}

			string& GetRsp_Line()
			{
				return rsp_line;
			}

			string& GetRsp_Head()
			{
				return rsp_head;
			}
			string& GetRsp_Blank()
			{
				return blank;
			}
			string& GetRsp_Text()
			{
				return rsp_text;
			}
			//响应状态行
			void MakeStatusLine()
			{
				rsp_line = HTTP_VERSION;
				rsp_line += " ";
				rsp_line += StringUtil::IntToString(code);
				rsp_line += " ";
				rsp_line += StringUtil::CodeToDesc(code);
				rsp_line += '\n';
			}

			void MakeResponseHead(Request *&rq_)
			{
				rsp_head = "Content-Length: ";
				rsp_head += StringUtil::IntToString(rq_->GetResourceSize());
				
				rsp_head += " ";
				rsp_head += "Content-Type: ";
				string& suffix_ = rq_->GetSuffix();
				rsp_head += StringUtil:: SuffixToType(suffix_);
				rsp_head += '\n';
			}

			void MakeResponseText(Request *&rq_)
			{
				string& path_ = rq_->GetPath();
				fd = open(path_.c_str(),O_RDONLY);
			       // cout<<fd<<endl;
			}

			~Response()
			{
				if(fd >= 0)
					close(fd);
			}

	};

	class Connect{
		private:
			int sock;
		public:
			Connect(int sock_)
				:sock(sock_)
			{}
			//接收每一行
			//因为每一行都是按照\r,\n,\r\n这三种情况进行行分隔，所以为了方便我们将HTTP格式统一看作是一行，然后每次读取一个字符，利用分隔符来进行行的划分，并且为了方便将那三种情况都化为\n
			int RecvOneLine(string& line_)
			{
				char c = 'X';
				while(c != '\n')
				{
					ssize_t s = recv(sock,&c,1,0);
					if(s > 0)
					{
						if(c == '\r')
						{
							recv(sock,&c,1,MSG_PEEK);//MSG_PEEK是窥探功能，可以在没有读取前查看下一个字符是否为行的分隔符
							if(c == '\n')
							{
								recv(sock,&c,1,0);
							}
							else
							{
								c = '\n';
							}       
						}
					  // LOG(INFO,"while");
					   line_.push_back(c);
					}
					else
					{
						break;
					}
				}
				LOG(INFO,"read line success");
				return line_.size();
			}

			//读取请求报头
			void ReadRequestHead(string &head_)
			 {
				head_ = "";
				string line_;
				while(line_ != "\n")
				{
					line_ = "";
					RecvOneLine(line_);
					head_ += line_;           //将报头放到head中
				}
			    LOG(INFO,"read head success");
			}  

			//读取正文
			void RecvRequestText(string &text_,int len_,string &param_)
			{

				char c_;
				int i = 0;
				while(i < len_)
				{
					recv(sock,&c_,1,0);
					text_.push_back(c_);
					++i;
				}
				param_ = text_;
			       LOG(INFO,"read text success");
			}
	//发送响应正文
			void SendResponseText(Response *&rsp_,bool cgi, Request *&rq_)
			{
				
				string& rsp_line_ = rsp_->GetRsp_Line();
				string& rsp_head_ = rsp_->GetRsp_Head();
				string& blank_ = rsp_->GetRsp_Blank();
				string& rsp_text_ = rsp_->GetRsp_Text();
                                int fd_ = rsp_->fd;
				
				
				if(!cgi)
				{
	    
					LOG(INFO,"non cgi");
					send(sock,rsp_line_.c_str(),rsp_line_.size(),0);
					send(sock,rsp_head_.c_str(),rsp_head_.size(),0);
					send(sock,blank_.c_str(),blank_.size(),0);
					sendfile(sock,fd_,NULL,rq_->GetResourceSize());
				}
				else
				{
					 
					send(sock,rsp_line_.c_str(),rsp_line_.size(),0);
					send(sock,rsp_head_.c_str(),rsp_head_.size(),0);
					send(sock,blank_.c_str(),blank_.size(),0);
					send(sock,rsp_text_.c_str(),rsp_text_.size(),0);
				
				}
			}

				~Connect()
				{
					if(sock >= 0)
					{
					   close(sock);
					}
				}


		};

		//处理函数
		class Entry{
			public:

			static void ProcessResponse(Connect *&conn_,Request *&rq_,Response *&rsp_)
			{
				if(rq_->IsCgi())
				{
				    ProcessCgi(conn_,rq_,rsp_);
				}else{
					ProcessNonCgi(conn_,rq_,rsp_);
				}

			}

				static int ProcessNonCgi(Connect *&conn_,Request *&rq_,Response *&rsp_)
				{
					  LOG(INFO,"NONCGI");
					rsp_->MakeStatusLine();
					rsp_->MakeResponseHead(rq_);
					rsp_->MakeResponseText(rq_);
					conn_->SendResponseText(rsp_,false,rq_);
				}

			static void ProcessCgi(Connect *&conn_,Request *&rq_,Response *&rsp_)
			{
			       int& code_ = rsp_->code;
			       string& rsp_text_ = rsp_->GetRsp_Text();
			       string& param_ = rq_->GetParam();
			       int input[2];
			       int output[2];
	    
			       pipe(input);           //创建管道
			       pipe(output);
	    
			       pid_t id = fork();
			       if(id < 0)
				{   
					code_ = NOT_FOUND;
					LOG(ERROR,"create fork faild");
					return;
				}
				else if(id == 0)   //子进程
				{
				       close(input[1]);
				       close(output[0]);
				       string cl_env_ = "Content-Length=";
				       cl_env_ += StringUtil::IntToString(param_.size());
				       putenv((char*)cl_env_.c_str());
				       dup2(input[0],0);
				       dup2(output[1],1);

				       string& path_ = rq_->GetPath();
				       execl(path_.c_str(),path_.c_str(),NULL);    //程序替换  第一个参数是执行的谁  第二个参数是怎么执行
										   //exec系列函数没有返回值，如果返回就是出错了。 
										   //exit(1);
										   //ocessResponse(Connect *&conn_,Request *&rq_,Response *&rsp_)
					   exit(1);

				}
				else              //父进程
				{
				       close(input[0]);
				       close(output[1]);
				       size_t size_ = param_.size();
				       size_t total_ = 0;
				       size_t cur_ = 0;
				       const char* p = param_.c_str();

				       while(total_ < size_ && (cur_ = write(input[1],p + total_ ,size_ - total_)) > 0)
					{
						  total_ += cur_;
					}
					LOG(INFO,"pipe write success");

				       char c_;
				       while(read(output[0],&c_,1) > 0)
				       {
					 rsp_text_.push_back(c_);
				       }

					LOG(INFO,"pipe read success");
					      waitpid(id,NULL,0);
					      close(input[1]);
					      close(output[0]);
					rsp_->MakeStatusLine();
					rq_->SetResourceSize(rsp_text_.size());
					rsp_->MakeResponseHead(rq_);
					conn_->SendResponseText(rsp_,true,rq_);
				}

			 }

			//处理请求
			static void* HandlerRequest(void* arg_)  //处理首行
			{
				//一定要注意坑不能直接将指针转化为int型
				int sock_ =  *(int*)arg_;
				delete (int*)arg_;
				Connect *conn_ = new Connect(sock_);
				Request *rq_ = new Request();
				Response *rsp_ = new Response();        
			       

				conn_->RecvOneLine(rq_->rq_line);
				rq_->RequestLineParse();
				int& code_ = rsp_->code;
				//判断方法
				if(!(rq_->IsMethodLegal()))
				{
					code_ = NOT_FOUND;
					goto out;
				}

				rq_->UriParse();

				if(!rq_->IsPathLegal())
				{
					code_ = NOT_FOUND;
					goto out;
				}
				LOG(INFO,"request path is OK");

				conn_->ReadRequestHead(rq_->rq_head);

				//解析报头
				if(rq_->RequestHeadParse())
				{
					LOG(INFO,"parse head done");
				}
				else
				{
					code_ = NOT_FOUND;
					goto out;
				}

				if(rq_->IsNeedRecvText())
				{
					conn_->RecvRequestText(rq_->rq_text,rq_->GetContentLength(),rq_->GetParam());
				} 

				 ProcessResponse(conn_,rq_,rsp_);

			

	out:
			/*	if(code_ != OK)
				{

				}			
			*/
				delete conn_;
				delete rq_;
				delete rsp_;
		       
			}

};



#endif
