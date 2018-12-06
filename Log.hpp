#ifndef __LOG_HPP__
#define __LOG_HPP__

#include<iostream>
#include<string>
#include<sys/time.h>
using namespace std;

#define INFO 0
#define DEBUG 1
#define WARNING 2
#define ERROR 3

uint64_t GetTimeStamp()
{
	struct timeval time_;
	gettimeofday(&time_,NULL);
	return time_.tv_sec;
}

string GetLogLevel(int level_){
	switch(level_){

		case 0:
			return "INFO";
		case 1:
			return "DEBUG";
		case 2:
			return "WARNING";
		case 3:
			return "ERROR";
		default:
			return "UNKNOW";
	}
}

//打印日志级别，消息
void LOG(int level_,string message_,string file_ ,int line_)
{
	cout<<"[ "<<GetTimeStamp()<<" ]"<<"[ "<<GetLogLevel(level_)<<" ]"<<"[ "<<file_<<" : "<<line_<<" ]"<<message_<<endl;
}

#define LOG(level_,message_) LOG(level_,message_,__FILE__,__LINE__)

#endif
