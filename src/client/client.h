#ifndef SIM_CLIENT_H
#define SIM_CLIENT_H

#include "message.h"

namespace sim{

class Link;
std::string encode(const std::string s, bool force_ascii=false);
std::string decoder(const std::string s);

class Client{
public:
	static Client* connect(const char* ip, int port);
	static Client* connect(const std::string& ip, int port);

	int send(const Message& msg);	// 阻塞
	int recv(Message* msg);

	~Client();
private:
	Client();
	Link* link;
};
};	// namespace sim
#endif


#ifndef SIM_CLIENT_H
#define SIM_CLIENT_H

#include "message.h"

namespace sim{

class Link;
std::string encode(const std::string s, bool force_ascii=false);
std::string decode(const std::string s);

class Client
{
public:
	static Client* connect(const char *ip, int port);
	static Client* connect(const std::string &ip, int port);

	int send(const Message &msg); // blocking send
	int recv(Message *msg); // blocking recv

	~Client();
private:
	Client();
	Link *link;
};

}; // namespace sim

#endif
