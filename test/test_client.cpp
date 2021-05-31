#include <stdio.h>
#include <stdlib.h>
#include "sim/sim.h"

/* 源码静态分析
1. 客户端连接，connect(ip, port);
*/

int main(int argc, char **argv){
	sim::Link *link = sim::Link::connect("127.0.0.1", 8800);


	if(!link){
		log_fatal("%s", strerror(errno));
		exit(0);
	}
	
	sim::Message msg;
	msg.add("ping");	// 内部用pair(tag,string)维护加入的string
	msg.add("hello world!");
	
	link->send(msg);	// 实际上发送到缓冲
	link->flush();	// 刷新发送

	while(1){
		int ret;
		ret = link->read();	// push到decoder的缓冲区中
		log_debug("read %d", ret);
		if(ret == 0){
			exit(0);
		}
		sim::Message msg;
		// 从
		ret = link->recv(&msg);	// 从decoder的缓冲区处理数据，用msg维护
		if(ret == -1){
			exit(0);
		}else if(ret == 0){
			continue;
		}
		log_debug("received msg: %s", msg.encode().c_str());
		break;
	}
	
	return 0;
}
