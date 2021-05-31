#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include "util/log.h"
#include "link.h"

namespace sim{

Link::Link(bool is_server){
	sock = -1;
	noblock_ = false;
	error_ = false;
	remote_ip[0] = '\0';
	remote_port = -1;
}

Link::~Link(){
	this->close();
}

void Link::close(){
	if(sock >= 0){
		::close(sock);
		sock = -1;
	}
}

/*
TCP_NODELAY选项是用来控制是否开启Nagle算法
该算法要求一个TCP连接上最多只能有一个未被确认的小分组
*/
void Link::nodelay(bool enable){
	int opt = enable? 1 : 0;
	::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&opt, sizeof(opt));
}

/*
通过发送心跳包，心跳检测请求维持连接
*/
void Link::keepalive(bool enable){
	int opt = enable? 1 : 0;
	::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&opt, sizeof(opt));
}

/*
fcntl 文件描述符操作函数
改变一个已打开的文件的属性，可以重新设置读、写、追加、非阻塞等标志
阻塞非阻塞是文件本身的特性，不是系统调用read/write本身可以控制的。
非阻塞 read，如果有数据到到来，返回读取到的数据的字节数。如果没有数据到来，返回 -1
非阻塞操作：在不能进行设备操作时，并不挂起，它或者放弃，或者不停地查询，直到可以进行操作。

库函数执行出错时，通常都会返回一个负值，并设置全局变量errno为特定信息的值
*/
void Link::noblock(bool enable){
	noblock_ = enable;
	if(enable){
		::fcntl(sock, F_SETFL, O_NONBLOCK | O_RDWR);
	}else{
		::fcntl(sock, F_SETFL, O_RDWR);
	}
}

Link* Link::connect(const std::string &ip, int port){
	return connect(ip.c_str(), port);
}

/*
bzero 头文件#include <string.h>
void bzero(void *s, int n);
将字符串前n个字符清零

Socket address family
AF_INET, AF_INET6

sockaddr_in Structures for handling internet addresses
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

高位字节，0x2211使用两个字节储存：高位字节是0x22，低位字节是0x11
字节存储方式：
大端字节序：高位字节在前，低位字节在后，这是人类读写数值的方法。
小端字节序：低位字节在前，高位字节在后，即以0x1122形式储存。
htonl()--"Host to Network Long"
htons()--"Host to Network Short"

网络字节序为小端字节序，大部分主机也是，但小部分主机是大端字节序

inet_pton是一个IP地址转换函数，可以在将点分文本的IP地址转换为二进制网络字节序”的IP地址

#include <sys/socket.h>
int sockfd = socket(domain, type, protocol)
成功返回非负整数，即fd
失败返回-1
int setsockopt(int sockfd, int level, int optname,  
                   const void *optval, socklen_t optlen);

connect(SOCKET s, const struct sockaddr FAR * name,int namelen)

:: 是作用域符，是运算符中等级最高的
1)global scope(全局作用域符），用法（::name)
2)class scope(类作用域符），用法(class::name)
3)namespace scope(命名空间作用域符），用法(namespace::name)

调用全局变量a，那么就写成::a；（也可以是全局函数）
extern
（1）当它与"C"一起连用时，如: extern "C" void fun(int a, int b);
则告诉编译器在编译fun这个函数名时按着C的规则去翻译相应的函数名而不是C++
（2）extern不与"C"在一起修饰变量或函数时，如在头文件中: extern int g_Int; 它的作用就是声明函数或全局变量的作用范围的关键字，
extern只是声明，告诉编译器这是全局函数，调用时只需要包含其头文件即可。
*/

Link* Link::connect(const char *ip, int port){
	Link *link;
	int sock = -1;

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);	// 统一字节序
	inet_pton(AF_INET, ip, &addr.sin_addr);	

	if((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
		goto sock_err;
	}
	if(::connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		goto sock_err;
	}

	//log_debug("fd: %d, connect to %s:%d", sock, ip, port);
	link = new Link();
	link->sock = sock;	// fd
	link->keepalive(true);
	return link;	// 返回一个link对象
sock_err:
	//log_debug("connect to %s:%d failed: %s", ip, port, strerror(errno));
	if(sock >= 0){
		::close(sock);
	}
	return NULL;
}

Link* Link::listen(const std::string &ip, int port){
	return listen(ip.c_str(), port);
}

/*
int bind(int sockfd, const struct sockaddr *addr, 
                          socklen_t addrlen);
int listen(int sockfd, int backlog);
when sucessful, return 0, else return -1.
int new_socket= accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int snprintf(char *str, size_t size, const char *format, char* str2)。
按照format格式化字符串str2,拷贝到str中
*/
Link* Link::listen(const char *ip, int port){
	Link *link;
	int sock = -1;

	int opt = 1;
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);
	inet_pton(AF_INET, ip, &addr.sin_addr);

	if((sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1){
		goto sock_err;
	}
	if(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
		goto sock_err;
	}
	if(::bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		goto sock_err;
	}
	if(::listen(sock, 1024) == -1){
		goto sock_err;
	}
	//log_debug("server socket fd: %d, listen on: %s:%d", sock, ip, port);

	link = new Link(true);
	link->sock = sock;	// 建立连接后的设备sock
	snprintf(link->remote_ip, sizeof(link->remote_ip), "%s", ip);
	link->remote_port = port;
	return link;

sock_err:
	//log_debug("listen %s:%d failed: %s", ip, port, strerror(errno));
	if(sock >= 0){
		::close(sock);
	}
	return NULL;
}

/*
慢系统调用(slow system call)：可能永远阻塞的系统调用。若没有客户连接到服务器上，那么服务器的accept调用就没有返回的保证。
 linux 或者 unix 环境中， errno 是一个十分重要的部分。在调用的函数出现问题的时候，我们可以通过 errno 的值来确定出错的原因
*/

Link* Link::accept(){
	Link *link;
	int client_sock;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	while((client_sock = ::accept(sock, (struct sockaddr *)&addr, &addrlen)) == -1){
		if(errno != EINTR){
			//log_error("socket %d accept failed: %s", sock, strerror(errno));
			return NULL;
		}
	}

	struct linger opt = {1, 0};
	int ret = ::setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (void *)&opt, sizeof(opt));
	if (ret != 0) {
		//log_error("socket %d set linger failed: %s", client_sock, strerror(errno));
	}

	link = new Link();
	link->sock = client_sock;
	link->keepalive(true);
	inet_ntop(AF_INET, &addr.sin_addr, link->remote_ip, sizeof(link->remote_ip));
	link->remote_port = ntohs(addr.sin_port);
	return link;
}

/*
unistd.h 中所定义的接口通常都是大量针对系统调用的封装（英语：wrapper functions），
如 fork、pipe 以及各种 I/O 原语（read、write、close 等等）
 #include <unistd.h>
 ssize_t read(int fd, void *buf, size_t count);
 read up to count bytes from file descriptor fd into the buffer starting at buf.
On success, the number of bytes read is returned 
*/

int Link::read(){
	int ret = 0;
	char buf[16 * 1024];
	int want = sizeof(buf);
	while(1){
		// test
		//want = 1;
		int len = ::read(sock, buf, want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//log_debug("fd: %d, read: -1, want: %d, error: %s", sock, want, strerror(errno));
				return -1;
			}
		}else{
			//log_debug("fd: %d, want=%d, read: %d", sock, want, len);
			if(len == 0){
				return 0;
			}
			ret += len;
			decoder_.push(buf, len);	// 将buf push到decoder的缓冲区中
		}
		if(!noblock_){
			break;
		}
	}
	//log_debug("read %d", ret);
	return ret;
}

/*
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);

write() writes up to count bytes from the buffer starting at buf
to the file referred to by the file descriptor fd.

string
c_str()：生成一个const char*指针，指向以空字符'\0'终止的数组
data():与c_str()类似，但是返回的数组不以空字符终止。

将char* 转化成string
*/
int Link::write(){
	int ret = 0;
	int want;
	while((want = output.size()) > 0){
		// test
		//want = 1;
		int len = ::write(sock, output.data(), want);
		if(len == -1){
			if(errno == EINTR){
				continue;
			}else if(errno == EWOULDBLOCK){
				break;
			}else{
				//log_debug("fd: %d, write: -1, error: %s", sock, strerror(errno));
				return -1;
			}
		}else{
			//log_info("fd: %d, want: %d, write: %d", sock, want, len);
			if(len == 0){
				// ?
				break;
			}
			ret += len;
			output = std::string(output.data() + len, output.size() - len);
		}
		if(!noblock_){
			break;
		}
	}
	return ret;
}

int Link::flush(){
	int len = 0;
	while(!output.empty()){
		int ret = this->write();	// ret是实际写入的长度
		if(ret == -1){
			return -1;
		}
		len += ret;
	}
	return len;
}

int Link::recv(Message *msg){
	return decoder_.parse(msg);
}

int Link::send(const Message &msg){
	std::string s = msg.encode();
	output.append(s);
	return (int)s.size();
}

}; // namespace sim
