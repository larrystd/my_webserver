#ifndef SIM_SIM_H
#define SIM_SIM_H

#include <sys/time.h>
#include <string>
#include "message.h"
#include "decoder.h"
#include "fde.h"
#include "link.h"
#include "server.h"
#include "handler.h"
#include "util/config.h"
#include "util/app.h"
#include "util/log.h"

namespace sim{

const static char KV_SEP_BYTE = '=';
const static char KV_END_BYTE = ' ';
const static char MSG_END_BYTE = '\n';

inline static 
double microtime(){
	struct timeval now;
	gettimeofday(&now, NULL);
	double ret = now.tv_sec + now.tv_usec/1000.0/1000.0;
	return ret;
}

std::string encode(const std::string s, bool force_ascii=false);
std::string decode(const std::string s);

};	// namespace sim
#endif

#ifndef SIM_SIM_H
#define SIM_SIM_H

#include <sys/time.h>
#include <string>
#include "message.h"
#include "decoder.h"
#include "fde.h"
#include "link.h"
#include "server.h"
#include "handler.h"
#include "util/config.h"
#include "util/app.h"
#include "util/log.h"

namespace sim{

const static char KV_SEP_BYTE = '=';
const static char KV_END_BYTE = ' ';
const static char MSG_END_BYTE = '\n';


inline static
double microtime(){
	struct timeval now;
	gettimeofday(&now, NULL);
	double ret = now.tv_sec + now.tv_usec/1000.0/1000.0;
	return ret;
}

std::string encode(const std::string s, bool force_ascii=false);
std::string decode(const std::string s);

}; // namespace sim

#endif
