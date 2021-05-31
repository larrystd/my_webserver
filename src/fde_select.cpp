#ifndef UTIL_FDE_SELECT_H
#define UTIL_FDE_SELECT_H

// #include "fde.h"

namespace sim{

Fdevents::Fdevents(){
	maxfd = -1;
	FD_ZERO(&readset);	// FD_ZERO(fd_set *fdset);将指定的文件描述符集清空
	FD_ZERO(&writeset);
}

Fdevents::~Fdevents(){
	for (size_t i = 0; i < events.size(); i++){
		delete events[i];
	}
	events.clear();
	read_events.clear();
}

bool Fdevents::isset(int fd, int flag){
	struct Fdevent* fde = get_fde(fd);
	return (bool)(fde->s_flags & flag);
}

int Fdevents::set(int fd, int flags, int data_num, void* data_ptr){
	if (fd > FD_SETSIZE - 1){
		return -1;
	}

	struct Fdevent* fde = get_fde(fd);
	if (fde->s_flags & flags){
		return 0;
	}

	if (flags & FDEVENT_IN)	FD_SET(fd, &readset);
	if (flags & FDEVENT_OUT) FD_SET(fd, &writeset);

	fde->data.num = data_num;
	fde->data.ptr = data_ptr;	// 如DefaultType,当前fde指向所属的session，HandlerType, 则指向Handler对象
	fde->s_flags |= flags;
	maxfd = fd > maxfd ? fd : maxfd;

	return 0;
}

int fdevents::del(int fd){
	FD_CLR(fd, &readset);
	FD_CLR(fd, &writeset);

	struct Fdevent* fde = get_fde(fd);	// 先注册fd个文件描述符，在返回第fd个
	fde->s_flags = FDVENT_NONE;
	while (maxfd >= 0 && this->events[maxfd]->s_flags == 0){
		maxfd--;
	}
	return 0;
}

int Fdevents::clr(int fd, int flags){
	struct Fdevent* fde = get_fde(fd);
	if (!(fde->s_flags & flags)){
		return 0;
	}
	if(flags & FDEVENT_IN)  FD_CLR(fd, &readset);	// 删除一个文件描述符。
	if(flags & FDEVENT_OUT) FD_CLR(fd, &writeset);

	fde->s_flags &= ~flags;
	while(this->events[maxfd]->s_flags == 0){	//注册的事件
		maxfd --;
	}
	return 0;
}

/*

用户首先将需要进行IO操作的socket添加到select中，然后阻塞等待select系统调用返回。
当数据到达时，socket被激活，select函数返回。用户线程正式发起read请求，读取数据并继续执行。
用户可以注册多个socket，然后不断地调用select读取被激活的socket,可达到在同一个线程内同时处理多个IO请求的目的

int select(int maxfdp, fd_set *readset, fd_set *writeset, fd_set *exceptset,struct timeval *timeout);
readfds、writefds、exceptset：分别指向可读、可写和异常等事件对应的描述符集合。

*/
const Fedvents::events_t* Fdevents::wait(int timeout_ms){
	struct timeval tv;
	struct Fdevent* fde;
	int i, ret;

	read_events.clear();
	fd_set t_readset = readset;
	fd_set t_writeset = writeset;

	if (timeout_ms >= 0){
		tv.tv_sec =  timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		ret = ::select(maxfd + 1, &t_readset, &t_writeset, NULL, &tv);
	}else{
		ret = ::select(maxfd+1, &t_readset, &t_writeset, NULL, NULL);
	}
	if (ret < 0){
		if (errno == EINTR){
			return &ready_events;
		}
		return NULL;
	}

	if (ret > 0){	// 成功注册
		for (i = 0; i <= maxfd && (int)ready_events.size() < ret; i++){
			fde = this->events[i];

			fde->events = FDEVENT_NONE;
			/* 如果fd在set中则真　 
			FDEVENT_IN 可读 001
			FDEVENT_OUT 可写 100
			文件描述符一般连续从0开始的整数 */
			if(FD_ISSET(i, &t_readset))  fde->events |= FDEVENT_IN;	// 时间描述
			if(FD_ISSET(i, &t_writeset)) fde->events |= FDEVENT_OUT;

			if(fde->events){	// 如果
				ready_events.push_back(fde);	// push到ready的事件
			}
		}
	}

	return &ready_events;
}
};	// namespace sim

#endif

/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef UTIL_FDE_SELECT_H
#define UTIL_FDE_SELECT_H

namespace sim{

Fdevents::Fdevents(){
	maxfd = -1;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
}

Fdevents::~Fdevents(){
	for(size_t i=0; i<events.size(); i++){
		delete events[i];
	}
	events.clear();
	ready_events.clear();
}

bool Fdevents::isset(int fd, int flag){
	struct Fdevent *fde = get_fde(fd);
	return (bool)(fde->s_flags & flag);
}

int Fdevents::set(int fd, int flags, int data_num, void *data_ptr){
	if(fd > FD_SETSIZE - 1){
		return -1;
	}

	struct Fdevent *fde = get_fde(fd);
	if(fde->s_flags & flags){
		return 0;
	}

	if(flags & FDEVENT_IN)  FD_SET(fd, &readset);
	if(flags & FDEVENT_OUT) FD_SET(fd, &writeset);

	fde->data.num = data_num;
	fde->data.ptr = data_ptr;
	fde->s_flags |= flags;
	maxfd = fd > maxfd? fd: maxfd;

	return 0;
}

int Fdevents::del(int fd){
	FD_CLR(fd, &readset);
	FD_CLR(fd, &writeset);

	struct Fdevent *fde = get_fde(fd);
	fde->s_flags = FDEVENT_NONE;
	while(maxfd >= 0 && this->events[maxfd]->s_flags == 0){
		maxfd --;
	}
	return 0;
}

int Fdevents::clr(int fd, int flags){
	struct Fdevent *fde = get_fde(fd);
	if(!(fde->s_flags & flags)){
		return 0;
	}
	if(flags & FDEVENT_IN)  FD_CLR(fd, &readset);
	if(flags & FDEVENT_OUT) FD_CLR(fd, &writeset);

	fde->s_flags &= ~flags;
	while(this->events[maxfd]->s_flags == 0){
		maxfd --;
	}
	return 0;
}

const Fdevents::events_t* Fdevents::wait(int timeout_ms){
	struct timeval tv;
	struct Fdevent *fde;
	int i, ret;

	ready_events.clear();
	
	fd_set t_readset = readset;
	fd_set t_writeset = writeset;

	if(timeout_ms >= 0){
		tv.tv_sec =  timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		ret = ::select(maxfd + 1, &t_readset, &t_writeset, NULL, &tv);
	}else{
		ret = ::select(maxfd + 1, &t_readset, &t_writeset, NULL, NULL);
	}
	if(ret < 0){
		if(errno == EINTR){
			return &ready_events;
		}
		return NULL;
	}

	if(ret > 0){
		for(i = 0; i <= maxfd && (int)ready_events.size() < ret; i++){
			fde = this->events[i];

			fde->events = FDEVENT_NONE;
			if(FD_ISSET(i, &t_readset))  fde->events |= FDEVENT_IN;
			if(FD_ISSET(i, &t_writeset)) fde->events |= FDEVENT_OUT;

			if(fde->events){
				ready_events.push_back(fde);
			}
		}
	}

	return &ready_events;
}


}; // namespace sim

#endif
