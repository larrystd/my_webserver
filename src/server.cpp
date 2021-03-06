#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "util/log.h"
#include "sim.h"
#include "fde.h"
#include "server.h"

#include "util/strings.h"

static std::string msg_str(const sim::Message &msg){
	std::string ret;
	const std::map<int, std::string>* fields = msg.fields();
	std::map<int, std::string>::const_iterator it;	
	// const iterator, iterator不能变，指向的内容可以变
	// const_iterator的意义与const iterator恰好相反，const修饰的是iterator所指向的内容，即const_iterator所指向的内容不能被改变，但是其本身可以被改变。
	char buf[50];
	int count = 0;
	for (it = fields->begin(); it != fields->end(); it++){
		if (ret.size() > 100){
			snprintf(buf, sizeof(buf), "[%d more...]", (int)fields->size() - count);
			ret.append(buf);
			break;
		}

		int tag = it->first;
		const std::string& val = it->second;	// 不能修改string值
		ret.append(str(tag));
		ret.push_back('=');
		if (val.size() < 30){
			std::string h = sim.encode(val, true);
			ret.append(h);
		}else{
			sprintf(buf, "[%d]", (int)val.size());
			ret.append(buf);
		}
		count++;
		if(count != (int)fields->size()){
			ret.push_back(' ');
		}
	}
	return ret;
}

namespace sim{

const static int DEFAULT_TYPE = 0;
const static int HANDLER_TYPE = 1;

Server::Server(){
	signal(SIGPIPE, SIG_IGN);
	link_count = 0;
	serv_link = NULL;
	fdes = new Fdevents();
}

Server::~Server(){
	for (int i = 0; i < this->handlers.size(); i++){
		Handler* handler = this->handlers[i];
		handler->m_free();
		delete handler;
	}
	this->handlers.clear();

	delete serv_link;
	delete fdes;
}

Server* Server::listen(const std::string& ip, int port){
	Link* serv_link = Link::listen(ip, port);
	if (!serv_link){
		return NULL;
	}

	Server* ret = new Server();
	ret->serv_link = serv_link;
	ret->fdes->set(serv_link->fd(), FDEVENT_IN, DEFAULT_TYPE, serv_link);

	return ret;
}

// server消息处理
// server对象push一个handler，并且注册到fdes
void Server::add_handler(Handler* handler){
	handler->m_init();
	this->handlers.push_back(handler);
	if (handler->fd() > 0){
		fdes->set(handler->fd(), FDEVENT_IN, HANDLER_TYPE, handler);	//fds是fd的集合
	}
}

/*
link->acception同时建立session对象

Handler 返回操作成功否
一个session可认为一个连接
session内部连接处理通过session->link
*/
Session* Server::accept_session(){
	Link* link = serv_link->accept();
	if (link == NULL){
		log_error("accept failed! %s", strerror(errno));
		return NULL;
	}

	link->nodelay();
	link->noblock();
	link->create_time = microtime();
	link->active_time = link->create_time;

	Session* sess = new Session();
	sess->link = link;
	this->sessions[sess->id] = sess;

	for (int i = 0; i < this->handlers.size(); i++){
		Handler* handler = this->handlers[i];
		HandlerState state = handler->accept(*sess);

		if (state == HANDLE_FAIL){
			delete link;
			delete sess;
			return NULL;
		}
	}
}

int Server::close_session(Session* sess){
	Link* link = sess->link;
	for (int i = 0; i < this->handlers.size(); i++){
		Handler* handler = this->handlers[i];
		handler->close(*sess);
	}

	this->link_count--;
	log_debug("delete link %s:%d, fd: %d, links:%d",
		link->remote_ip, link->remote_port, link->fd(), this->link_count);
	fdes->del(link->fd());

	this->sessions.erase(sess->id);
	delete link;
	delete sess;
}

// 通过session内部link来读取传来的信息
int Server::read_session(Session* sess){
	Link* link = sess->link;
	if (link->error()){
		return 0;
	}

	int len = link->read();
	if (len <= 0){
		this->close_session(sess);
		return -1;
	}

	while(1){
		Request req;
		int ret = link->recv(&req.msg);	// 读消息
		if (ret == -1){
			log_info("fd: %d, parse error, delete link", link->fd());
			this->close_session(sess);
		}else if(ret == 0){
			break;
		}
		req.stime = microtime();
		req.sess = *sess;

		Response resp;
		for (int i = 0; i < this->handlers.size(); i++){
			Handler* handler = this->handlers[i];
			req.time_wait = 1000 * (microtime() - req.stime);
			HandlerState state = handler->proc(req, &resp);	//处理消息，结果到resp，proc函数可被重写
			req.time_proc = 1000 * (microtime() - req.stime) - req.time_wait;
			if (state == HANDLE_RESP){
				link->send(resp.msg);	//发送到缓冲区

				if (link && !link->output.empty()){
					fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);	// 文件描述符注册
				}

				if (log_level() >= Logger::LEVEL_DEBUG){
					log_debug("w:%.3f, p:%.3f, req: %s, resp: %s",
					req.time_wait, req.time_proc,
					msg_str(req.msg).c_str(),
					msg_str(resp.msg).c_str());
				}
			}else if(state == HANDLE_FAIL){
				this->close_session(sess);
				return -1;
			}
		}
	}
	return 0;
}

int Server::write_session(Session* sess){
	Link* link = sess->link;
	if (link->error()){
		return 0;
	}

	int len = link->write();	// 从缓冲区写入到socket
	if (len <= 0){
		log_debug("fd: %d, write: %d, delete link", link->fd(), len);
		return -1;
	}
	if (link->output.empty()){
		fdes->clr(link->fd(), FDEVENT_OUT);	// 注册fd到epoll
	}
	return 0;
}

// 根据sess_id得到session对象
Session* Server::get_session(int64_t sess_id){
	std::map<int64_t, Session* >::iterator it;
	it = sessions.find(sess_id);
	if (it == sessions.end()){
		return NULL;
	}
	return it->second;
}

void Server::loop(){
	while(1){
		if (this->loop_once() == -1){
			break;
		}
	}
}

/*
session的作用
*/
int Server::loop_once(){
	const Fdevents::events_t *events;
	events = fdes->wait(20);	// IO复用间隔20ms
	if(events == NULL){
		log_fatal("events.wait error: %s", strerror(errno));
		return 0;
	}
	
	for(int i=0; i<(int)events->size(); i++){
		const Fdevent *fde = events->at(i);	//vector->at(pos)
		if(fde->data.ptr == serv_link){	// 连接accept操作
			this->accept_session();	// 连接session

		}else if(fde->data.num == HANDLER_TYPE){	// 要处理的事件
			Handler *handler = (Handler *)fde->data.ptr; // fde内部存储当前fde属于哪个session,故转换成handler，其实已经绑定
			// Handler里面有response, response里面有session
			while(Response *resp = handler->handle()){	// 处理
				Session *sess = this->get_session(resp->sess.id);
				if(sess){
					Link *link = sess->link;
					link->send(resp->msg);
					if(link && !link->output.empty()){
						fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);
					}
				}
				delete resp;
			}
		}else{
			Session *sess = (Session *)fde->data.ptr;
			Link *link = sess->link;
			if(fde->events & FDEVENT_IN){	// 读操作符，读
				if(this->read_session(sess) == -1){
					continue;
				}
			}
			if(fde->events & FDEVENT_OUT){	// 写操作符
				if(this->write_session(sess) == -1){
					continue;
				}
			}
			if(link && !link->output.empty()){	// 注册
				fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);
			}
		}
	}
	return 0;
}

};	// namespace sim

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "util/log.h"
#include "sim.h"
#include "fde.h"
#include "server.h"

#include "util/strings.h"

static std::string msg_str(const sim::Message &msg){
	std::string ret;
	const std::map<int, std::string> *fields = msg.fields();
	std::map<int, std::string>::const_iterator it;
	char buf[50];
	int count = 0;
	for(it=fields->begin(); it!=fields->end(); it++){
		if(ret.size() > 100){
			snprintf(buf, sizeof(buf), "[%d more...]", (int)fields->size() - count);
			ret.append(buf);
			break;
		}
		
		int tag = it->first;
		const std::string &val = it->second;
		ret.append(str(tag));
		ret.push_back('=');
		if(val.size() < 30){
			std::string h = sim::encode(val, true);
			ret.append(h);
		}else{
			sprintf(buf, "[%d]", (int)val.size());
			ret.append(buf);
		}
		
		count ++;
		if(count != (int)fields->size()){
			ret.push_back(' ');
		}
	}
	return ret;
}


namespace sim{
	
const static int DEFAULT_TYPE = 0;
const static int HANDLER_TYPE = 1;

Server::Server(){
	signal(SIGPIPE, SIG_IGN);
	link_count = 0;
	serv_link = NULL;
	fdes = new Fdevents();
}

Server::~Server(){
	for(int i=0; i<this->handlers.size(); i++){
		Handler *handler = this->handlers[i];
		handler->m_free();
		delete handler;
	}
	this->handlers.clear();
	
	delete serv_link;
	delete fdes;
}

Server* Server::listen(const std::string &ip, int port){
	Link *serv_link = Link::listen(ip, port);
	if(!serv_link){
		return NULL;
	}
	
	Server *ret = new Server();
	ret->serv_link = serv_link;
	ret->fdes->set(serv_link->fd(), FDEVENT_IN, DEFAULT_TYPE, serv_link);
	return ret;
}

void Server::add_handler(Handler *handler){
	handler->m_init();
	this->handlers.push_back(handler);
	if(handler->fd() > 0){
		fdes->set(handler->fd(), FDEVENT_IN, HANDLER_TYPE, handler);
	}
}

Session* Server::accept_session(){
	Link *link = serv_link->accept();
	if(link == NULL){
		log_error("accept failed! %s", strerror(errno));
		return NULL;
	}
				
	link->nodelay();
	link->noblock();
	link->create_time = microtime();
	link->active_time = link->create_time;
	
	Session *sess = new Session();
	sess->link = link;
	this->sessions[sess->id] = sess;
	
	for(int i=0; i<this->handlers.size(); i++){
		Handler *handler = this->handlers[i];
		HandlerState state = handler->accept(*sess);
		if(state == HANDLE_FAIL){
			delete link;
			delete sess;
			return NULL;
		}
	}
	
	this->link_count ++;				
	log_debug("new link from %s:%d, fd: %d, links: %d",
		link->remote_ip, link->remote_port, link->fd(), this->link_count);
	fdes->set(link->fd(), FDEVENT_IN, DEFAULT_TYPE, sess);
	
	return sess;
}

int Server::close_session(Session *sess){
	Link *link = sess->link;
	for(int i=0; i<this->handlers.size(); i++){
		Handler *handler = this->handlers[i];
		handler->close(*sess);
	}
	
	this->link_count --;
	log_debug("delete link %s:%d, fd: %d, links: %d",
		link->remote_ip, link->remote_port, link->fd(), this->link_count);
	fdes->del(link->fd());

	this->sessions.erase(sess->id);
	delete link;
	delete sess;
	return 0;
}

int Server::read_session(Session *sess){
	Link *link = sess->link;
	if(link->error()){
		return 0;
	}
	
	int len = link->read();
	if(len <= 0){
		this->close_session(sess);
		return -1;
	}
	
	while(1){
		Request req;
		int ret = link->recv(&req.msg);
		if(ret == -1){
			log_info("fd: %d, parse error, delete link", link->fd());
			this->close_session(sess);
			return -1;
		}else if(ret == 0){
			// 报文未就绪, 继续读网络
			break;
		}
		req.stime = microtime();
		req.sess = *sess;

		Response resp;
		for(int i=0; i<this->handlers.size(); i++){
			Handler *handler = this->handlers[i];
			req.time_wait = 1000 * (microtime() - req.stime);
			HandlerState state = handler->proc(req, &resp);
			req.time_proc = 1000 * (microtime() - req.stime) - req.time_wait;
			if(state == HANDLE_RESP){
				link->send(resp.msg);
				if(link && !link->output.empty()){
					fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);
				}
				
				if(log_level() >= Logger::LEVEL_DEBUG){
					log_debug("w:%.3f,p:%.3f, req: %s resp: %s",
						req.time_wait, req.time_proc,
						msg_str(req.msg).c_str(),
						msg_str(resp.msg).c_str());
				}
			}else if(state == HANDLE_FAIL){
				this->close_session(sess);
				return -1;
			}
		}
	}

	return 0;
}

int Server::write_session(Session *sess){
	Link *link = sess->link;
	if(link->error()){
		return 0;
	}

	int len = link->write();
	if(len <= 0){
		log_debug("fd: %d, write: %d, delete link", link->fd(), len);
		this->close_session(sess);
		return -1;
	}
	if(link->output.empty()){
		fdes->clr(link->fd(), FDEVENT_OUT);
	}
	return 0;
}

Session* Server::get_session(int64_t sess_id){
	std::map<int64_t, Session *>::iterator it;
	it = sessions.find(sess_id);
	if(it == sessions.end()){
		return NULL;
	}
	return it->second;
}

void Server::loop(){
	while(1){
		if(this->loop_once() == -1){
			break;
		}
	}
}

int Server::loop_once(){
	const Fdevents::events_t *events;
	events = fdes->wait(20);
	if(events == NULL){
		log_fatal("events.wait error: %s", strerror(errno));
		return 0;
	}
	
	for(int i=0; i<(int)events->size(); i++){
		const Fdevent *fde = events->at(i);
		if(fde->data.ptr == serv_link){
			this->accept_session();
		}else if(fde->data.num == HANDLER_TYPE){
			Handler *handler = (Handler *)fde->data.ptr;
			while(Response *resp = handler->handle()){
				Session *sess = this->get_session(resp->sess.id);
				if(sess){
					Link *link = sess->link;
					link->send(resp->msg);
					if(link && !link->output.empty()){
						fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);
					}
				}
				delete resp;
			}
		}else{
			Session *sess = (Session *)fde->data.ptr;
			Link *link = sess->link;
			if(fde->events & FDEVENT_IN){
				if(this->read_session(sess) == -1){
					continue;
				}
			}
			if(fde->events & FDEVENT_OUT){
				if(this->write_session(sess) == -1){
					continue;
				}
			}
			if(link && !link->output.empty()){
				fdes->set(link->fd(), FDEVENT_OUT, DEFAULT_TYPE, sess);
			}
		}
	}
	return 0;
}

}; // namespace sim
