/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SIM_UTIL_STRING_H
#define SIM_UTIL_STRING_H

#include <unistd.h> 
// Unix Standard的缩写,提供对 POSIX 操作系统 API 的访问功能的头文件的名称
// unistd.h 中所定义的接口通常都是大量针对系统调用的封装（英语：wrapper functions），如 fork、pipe 以及各种 I/O 原语（read、write、close 等等）。
#include <string.h>
#include <errno.h>
//error是一个包含在<errno.h>中的预定义的外部int变量，用于表示最近一个函数调用是否产生了错误。若为0，则无错误，其它值均表示一类错误。
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <algorithm>

// inline加static, inline不强制内联，如果不加static修饰可能出现重定义。重定义可能发生隐藏错误，应尽量避免。
inline static 
int is_empty_str(const char* str){
	const char *p = str;
	while (*p && isspace(*p)){	// 只含有space的字符串也为空
		p++;
	}
	return *p == '\0';
}

/* 返回字符串第一个不是空格的字符指针,trim意思使修剪*/
inline static
char* ltrim(const char* str){
	const char* p = str;
	while (*p && isspace(*p)){
		p++;
	}
	return (char* )p;
}

/* 返回指向字符串非空格结尾的指针,结尾指向\0,需要将新的结尾位置修改为\0，修改字符串*/
inline static
char* rtrim(char* str){
	char* p;
	p = str + strlen(str)-1;	// strlen不包含\0结束符
	while(p >= str && isspace(*p)){
		p--;
	}
	*(++p) = '\0';	// 注意修改字符串
	return p;
}

/* 返回两侧非空格，返回开头指针*/
inline static
char* trim(char* str){
	char *p;
	p = ltrim(str);
	rtrim(p);
	return p;
}

inline static
void strtolower(std::string *str){
	std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

inline static
void strtoupper(std::string *str){
	std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

inline static
std::string real_dirname(const char* filepath){
	std::string dir;
	if (filepath[0] != '/'){
		char buf[1024];
		char* p = getcwd(buf, sizeof(buf));
		if (p != NULL){
			dir.append(p);
		}
		dir.append("/");
	}

	const char* p = strrchr(filepath, '/');	//指向的字符串中搜索最后一次出现字符 c
	if (p != NULL){
		dir.append(filepath, p - filepath);
	}
	return dir;
}

inline static
std::string str_escape(const char *s, int size){
	static const char *hex = "0123456789abcdef";
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		switch(c){
			case '\r':
				ret.append("\\r");
				break;
			case '\n':
				ret.append("\\n");
				break;
			case '\t':
				ret.append("\\t");
				break;
			case '\\':
				ret.append("\\\\");
				break;
			case ' ':
				ret.push_back(c);
				break;
			default:
				if(c >= '!' && c <= '~'){
					ret.push_back(c);
				}else{
					ret.append("\\x");
					unsigned char d = c;
					ret.push_back(hex[d >> 4]);
					ret.push_back(hex[d & 0x0f]);
				}
				break;
		}
	}
	return ret;
}

inline static
std::string str_escape(const std::string &s){
	return str_escape(s.data(), (int)s.size());
}

inline static 
int hex_int(char c){
	if (c >= '0' && c <= '9'){
		return c - '0';
	}else{
		return c - 'a' + 10;	// 16进制字符转整数
	}
}

inline static
std::string str_unescape(const char *s, int size){
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		if(c != '\\'){
			ret.push_back(c);
		}else{
			if(i >= size - 1){
				continue;
			}
			char c2 = s[++i];
			switch(c2){
				case 'a':
					ret.push_back('\a');
					break;
				case 'b':
					ret.push_back('\b');
					break;
				case 'f':
					ret.push_back('\f');
					break;
				case 'v':
					ret.push_back('\v');
					break;
				case 'r':
					ret.push_back('\r');
					break;
				case 'n':
					ret.push_back('\n');
					break;
				case 't':
					ret.push_back('\t');
					break;
				case '\\':
					ret.push_back('\\');
					break;
				case 'x':
					if(i < size - 2){
						char c3 = s[++i];
						char c4 = s[++i];
						ret.push_back((char)((hex_int(c3) << 4) + hex_int(c4)));
					}
					break;
				default:
					ret.push_back(c2);
					break;
			}
		}
	}
	return ret;
}

inline static
std::string str_unescape(const std::string &s){
	return str_unescape(s.data(), (int)s.size());
}

inline static
std::string hexmem(const void *p, int size){
	return str_escape((char *)p, size);
	/*
	std::string ret;
	char buf[4];
	for(int i=0; i<size; i++){
		char c = ((char *)p)[i];
		if(isalnum(c) || isprint(c)){
			ret.append(1, c);
		}else{
			switch(c){
				case '\r':
					ret.append("\\r", 2);
					break;
				case '\n':
					ret.append("\\n", 2);
					break;
				default:
					sprintf(buf, "\\%02x", (unsigned char)c);
					ret.append(buf, 3);
			}
		}
	}
	return ret;
	*/
}

// TODO: mem_printf("%5c%d%s", p, size);
static inline
void dump(const void *p, int size, const char *msg = NULL){
	if(msg == NULL){
		printf("dump <");
	}else{
		printf("%s <", msg);
	}
	std::string s = hexmem(p, size);
	printf("%s>\n", s.c_str());
}

static inline
std::string str(const char* s){
	return std::string(s);	// c风格指针字符串转为C++风格string对象
}

static inline
std::string str(uint32_t v){
	char buf[21] = {0};	// 全为
	snprintf(buf, sizeof(buf), "%d", v);
	return std::string(buf);
}

static inline
std::string str(int64_t v){
	char buf[21] = {0};
	snprintf(buf, sizeof(buf), "%" PRId64 "", v);
	return std::string(buf);
}

static inline
std::string str(uint64_t v){
	char buf[21] = {0};
	snprintf(buf, sizeof(buf), "%" PRIu64 "", v);
	return std::string(buf);
}

static inline
std::string str(double v){
	char buf[21] = {0};
	if(v - floor(v) == 0){
		snprintf(buf, sizeof(buf), "%.0f", v);
	}else{
		snprintf(buf, sizeof(buf), "%f", v);
	}
	return std::string(buf);
}

static inline
std::string str(float v){
	return str((double)v);
}

// all str_to_xx methods set errno on error

static inline
int str_to_int(const std::string& str){
	const char* start = str.c_str();
	char* end;
	int ret = (int)strtol(start, &end, 10); // strtol是一个C语言函数,作用就是将一个字符串转换为长整型long,
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;	
}

static inline
int str_to_int(const char *p, int size){
	return str_to_int(std::string(p, size));
}

static inline
int64_t str_to_int64(const std::string &str){
	const char *start = str.c_str();
	char *end;
	int64_t ret = (int64_t)strtoll(start, &end, 10);
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;
}

static inline
int64_t str_to_int64(const char *p, int size){
	return str_to_int64(std::string(p, size));
}

static inline
uint64_t str_to_uint64(const std::string &str){
	const char *start = str.c_str();
	char *end;
	uint64_t ret = (uint64_t)strtoull(start, &end, 10);
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;
}

static inline
uint64_t str_to_uint64(const char *p, int size){
	return str_to_uint64(std::string(p, size));
}

static inline
double str_to_double(const char *p, int size){
	return atof(std::string(p, size).c_str());
}

static inline
std::string substr(const std::string& str, int start, int size){
	if (start < 0){
		start = (int)str.size() + start;	// 更新当前值
	}
	if(size < 0){
		// 忽略掉 abs(size) 个字节
		size = ((int)str.size() + size) - start;
	}
	if(start < 0 || size_t(start) >= str.size() || size < 0){
		return "";
	}
	return str.substr(start, size);
}

static inline
int bitcount(const char *p, int size){
	int n = 0;
	for(int i=0; i<size; i++){
		unsigned char c = (unsigned char)p[i];
		while(c){
			n += c & 1;
			c = c >> 1;
		}
	}
	return n;
}

// is big endia. TODO: auto detect
// 条件编译，设置不同的条件，在编译时编译不同的代码
#if 0
	#define big_endian(v) (v)
#else
	static inline
	uint16_t big_endian(uint16_t v){
		return (v>>8) | (v<<8);
	}

	static inline
	uint32_t big_endian(uint32_t v){
		return (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
	}

	static inline
	uint64_t big_endian(uint64_t v){
		uint32_t h = v >> 32;
		uint32_t l = v & 0xffffffffull;
		return big_endian(h) | ((uint64_t)big_endian(l) << 32);
	}
#endif


#endif


#ifndef SIM_UTIL_STRING_H
#define SIM_UTIL_STRING_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <algorithm>


inline static
int is_empty_str(const char *str){
	const char *p = str;
	while(*p && isspace(*p)){
		p++;
	}
	return *p == '\0';
}

/* 返回左边不包含空白字符的字符串的指针 */
inline static
char *ltrim(const char *str){
	const char *p = str;
	while(*p && isspace(*p)){
		p++;
	}
	return (char *)p;
}

/* 返回指向字符串结尾的指针, 会修改字符串内容 */
inline static
char *rtrim(char *str){
	char *p;
	p = str + strlen(str) - 1;
	while(p >= str && isspace(*p)){
		p--;
	}
	*(++p) = '\0';
	return p;
}

inline static
char *trim(char *str){
	char *p;
	p = ltrim(str);
	rtrim(p);
	return p;
}

inline static
void strtolower(std::string *str){
	std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

inline static
void strtoupper(std::string *str){
	std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

inline static
std::string real_dirname(const char *filepath){
	std::string dir;
	if(filepath[0] != '/'){
		char buf[1024];
		char *p = getcwd(buf, sizeof(buf));
		if(p != NULL){
			dir.append(p);
		}
		dir.append("/");
	}

	const char *p = strrchr(filepath, '/');
	if(p != NULL){
		dir.append(filepath, p - filepath);
	}
	return dir;
}

inline static
std::string str_escape(const char *s, int size){
	static const char *hex = "0123456789abcdef";
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		switch(c){
			case '\r':
				ret.append("\\r");
				break;
			case '\n':
				ret.append("\\n");
				break;
			case '\t':
				ret.append("\\t");
				break;
			case '\\':
				ret.append("\\\\");
				break;
			case ' ':
				ret.push_back(c);
				break;
			default:
				if(c >= '!' && c <= '~'){
					ret.push_back(c);
				}else{
					ret.append("\\x");
					unsigned char d = c;
					ret.push_back(hex[d >> 4]);
					ret.push_back(hex[d & 0x0f]);
				}
				break;
		}
	}
	return ret;
}

inline static
std::string str_escape(const std::string &s){
	return str_escape(s.data(), (int)s.size());
}

inline static
int hex_int(char c){
	if(c >= '0' && c <= '9'){
		return c - '0';
	}else{
		return c - 'a' + 10;
	}
}

inline static
std::string str_unescape(const char *s, int size){
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		if(c != '\\'){
			ret.push_back(c);
		}else{
			if(i >= size - 1){
				continue;
			}
			char c2 = s[++i];
			switch(c2){
				case 'a':
					ret.push_back('\a');
					break;
				case 'b':
					ret.push_back('\b');
					break;
				case 'f':
					ret.push_back('\f');
					break;
				case 'v':
					ret.push_back('\v');
					break;
				case 'r':
					ret.push_back('\r');
					break;
				case 'n':
					ret.push_back('\n');
					break;
				case 't':
					ret.push_back('\t');
					break;
				case '\\':
					ret.push_back('\\');
					break;
				case 'x':
					if(i < size - 2){
						char c3 = s[++i];
						char c4 = s[++i];
						ret.push_back((char)((hex_int(c3) << 4) + hex_int(c4)));
					}
					break;
				default:
					ret.push_back(c2);
					break;
			}
		}
	}
	return ret;
}

inline static
std::string str_unescape(const std::string &s){
	return str_unescape(s.data(), (int)s.size());
}

inline static
std::string hexmem(const void *p, int size){
	return str_escape((char *)p, size);
	/*
	std::string ret;
	char buf[4];
	for(int i=0; i<size; i++){
		char c = ((char *)p)[i];
		if(isalnum(c) || isprint(c)){
			ret.append(1, c);
		}else{
			switch(c){
				case '\r':
					ret.append("\\r", 2);
					break;
				case '\n':
					ret.append("\\n", 2);
					break;
				default:
					sprintf(buf, "\\%02x", (unsigned char)c);
					ret.append(buf, 3);
			}
		}
	}
	return ret;
	*/
}

// TODO: mem_printf("%5c%d%s", p, size);
static inline
void dump(const void *p, int size, const char *msg = NULL){
	if(msg == NULL){
		printf("dump <");
	}else{
		printf("%s <", msg);
	}
	std::string s = hexmem(p, size);
	printf("%s>\n", s.c_str());
}


static inline
std::string str(const char *s){
	return std::string(s);
}

static inline
std::string str(int v){
	char buf[21] = {0};
	snprintf(buf, sizeof(buf), "%d", v);
	return std::string(buf);
}

static inline
std::string str(uint32_t v){
	char buf[21] = {0};		// 空null 的ascii码表示为0，即\0.'\0'的ASCII码为0,即通常所说的空(NULL)
	snprintf(buf, sizeof(buf), "%d", v);
	return std::string(buf);
}

static inline
std::string str(int64_t v){
	char buf[21] = {0};
	snprintf(buf, sizeof(buf), "%" PRId64 "", v);
	return std::string(buf);
}

static inline
std::string str(uint64_t v){
	char buf[21] = {0};
	snprintf(buf, sizeof(buf), "%" PRIu64 "", v);
	return std::string(buf);
}

static inline
std::string str(double v){
	char buf[21] = {0};
	if(v - floor(v) == 0){
		snprintf(buf, sizeof(buf), "%.0f", v);
	}else{
		snprintf(buf, sizeof(buf), "%f", v);
	}
	return std::string(buf);
}

static inline
std::string str(float v){
	return str((double)v);
}

// all str_to_xx methods set errno on error

static inline
int str_to_int(const std::string &str){
	const char *start = str.c_str();
	char *end;
	int ret = (int)strtol(start, &end, 10);
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;
}

static inline
int str_to_int(const char *p, int size){
	return str_to_int(std::string(p, size));
}

static inline
int64_t str_to_int64(const std::string &str){
	const char *start = str.c_str();
	char *end;
	int64_t ret = (int64_t)strtoll(start, &end, 10);
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;
}

static inline
int64_t str_to_int64(const char *p, int size){
	return str_to_int64(std::string(p, size));
}

static inline
uint64_t str_to_uint64(const std::string &str){
	const char *start = str.c_str();
	char *end;
	uint64_t ret = (uint64_t)strtoull(start, &end, 10);
	// the WHOLE string must be string represented integer
	if(*end == '\0' && size_t(end - start) == str.size()){
		errno = 0;
	}else{
		// strtoxx do not set errno all the time!
		if(errno == 0){
			errno = EINVAL;
		}
	}
	return ret;
}

static inline
uint64_t str_to_uint64(const char *p, int size){
	return str_to_uint64(std::string(p, size));
}

static inline
double str_to_double(const char *p, int size){
	return atof(std::string(p, size).c_str());
}

static inline
std::string substr(const std::string &str, int start, int size){
	if(start < 0){
		start = (int)str.size() + start;
	}
	if(size < 0){
		// 忽略掉 abs(size) 个字节
		size = ((int)str.size() + size) - start;
	}
	if(start < 0 || size_t(start) >= str.size() || size < 0){
		return "";
	}
	return str.substr(start, size);
}

static inline
std::string str_slice(const std::string &str, int start, int end){
	if(start < 0){
		start = (int)str.size() + start;
	}
	int size;
	if(end < 0){
		size = ((int)str.size() + end + 1) - start;
	}else{
		size = end - start + 1;
	}
	if(start < 0 || size_t(start) >= str.size() || size < 0){
		return "";
	}
	return str.substr(start, size);
}

static inline
int bitcount(const char *p, int size){
	int n = 0;
	for(int i=0; i<size; i++){
		unsigned char c = (unsigned char)p[i];
		while(c){
			n += c & 1;
			c = c >> 1;
		}
	}
	return n;
}

// is big endia. TODO: auto detect
// 条件编译，设置不同的条件，在编译时编译不同的代码
#if 0
	#define big_endian(v) (v)
#else
	static inline
	uint16_t big_endian(uint16_t v){
		return (v>>8) | (v<<8);
	}

	static inline
	uint32_t big_endian(uint32_t v){
		return (v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24);
	}

	static inline
	uint64_t big_endian(uint64_t v){
		uint32_t h = v >> 32;
		uint32_t l = v & 0xffffffffull;
		return big_endian(h) | ((uint64_t)big_endian(l) << 32);
	}
#endif


#endif
