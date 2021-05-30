#ifndef SIM_SERVER_H
#define SIM_SERVER_H

#include <string>
#include <vector>
#include <map>
#include "link.h"
#include "handler.h"

// 命名空间的作用是建立一些互相分隔的作用域,以免产生名字冲突
namespace sim{

class Fdevents;

class Server{
public:
	// 静态成员在类的所有对象中是共享的。
	static Server* listen(const std::string& ip, int port);
	Server();
	~Server();

	void add_handler(Handler* handler);
	void loop();
	int loop_once();

private:
	Fdevents* fdes;
	Link* serv_link;
	int link_count;
	std::map<int64_t, Session* > sessions;
	std::vector<Handler* > handlers;

	Session* accept_session();
	Session* get_session(int64_t sess_id);
	int close_session(Session* sess);
	int read_session(Session* sess);
	int write_session(Session* sess);
};
};
#endif

#ifndef SIM_SERVER_H
#define SIM_SERVER_H

#include <string>
#include <vector>
#include <map>
#include "link.h"
#include "handler.h"

namespace sim{

class Fdevents;

class Server
{
public:
	static Server* listen(const std::string &ip, int port);
	//static Server* create(...);
	Server();
	~Server();
	
	void add_handler(Handler *handler);
	void loop();
	int loop_once();
	
	//int send(int64_t sess_id, const Message &msg);
	//int send_all(const Message &msg);
private:
	Fdevents *fdes;
	Link *serv_link;
	int link_count;
	std::map<int64_t, Session *> sessions;
	std::vector<Handler *> handlers;

	Session* accept_session();
	Session* get_session(int64_t sess_id);
	int close_session(Session *sess);
	int read_session(Session *sess);
	int write_session(Session *sess);
};

}; // namespace sim

#endif
