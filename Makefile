bin=HttpdServer
cc=g++
LDFLAGS= -lpthread

.PHONY:all
all:$(bin) cgi

$(bin):HttpdServer.cc
	$(cc) -o $@ $^ $(LDFLAGS) -std=c++11

.PHONY:cgi
cgi:
	g++ -o TestCgi TestCgi.cc
.PHONY:clean
clean:
	rm -f $(bin) TestCgi 
