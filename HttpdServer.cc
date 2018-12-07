#include"HttpdServer.hpp"
#include<signal.h>
#include<unistd.h>
//using namespace std;
static void Usage(string proc_)
{
	cout<<"Usage"<<proc_<<" "<<"prot"<<endl;
}

int main(int argc,char* argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
		exit(1);
	}
        signal(SIGPIPE,SIG_IGN);
	HttpdServer *hp_ = new HttpdServer(atoi(argv[1]));
        daemon(1,0);
	hp_->InitServer();
	hp_->Start();

	delete hp_;
	return 0;
}
