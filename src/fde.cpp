#include <unistd.h>
#include "fde.h"

namespace sim{
struct Fdevent* Fdevents::get_fde(int fd){
	while ((int)events.size() <= fd){
		struct Fdevent* fde = new Fdevent();
		fde->fd = events.size();
		fde->s_flags = FDEVENT_NONE;
		fde->data.num = 0;
		fde->data.ptr = NULL;
		events.push_back(fde);
	}
	return events[fd];
}

};	// namespace sim

#ifdef HAVE_EPOLL
#include "fde_epoll.cpp"
#else
#include "fde_select.cpp"
#endif

/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <unistd.h>
#include "fde.h"

namespace sim{

struct Fdevent* Fdevents::get_fde(int fd){
	while((int)events.size() <= fd){
		struct Fdevent *fde = new Fdevent();
		fde->fd = events.size();
		fde->s_flags = FDEVENT_NONE;	// 无事件
		fde->data.num = 0;
		fde->data.ptr = NULL;
		events.push_back(fde);
	}
	return events[fd];
}

}; // namespace sim


#ifdef HAVE_EPOLL
#include "fde_epoll.cpp"
#else
#include "fde_select.cpp"
#endif
