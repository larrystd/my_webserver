#ifndef SIM_UTIL_FILE_H_
#define SIM_UTIL_FILE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

static inline
bool file_exists(const std::string &filename){
	struct stat st;
	return stat(filename.c_str(), &st) == 0;	// in <sys/stat.h>
}

static inline
bool is_dir(const std::string &filename){
	struct stat st;
	if (stat(filename.c_str(), &st) == -1){
		return false;
	}
	return (bool)S_ISDIR(st.st_mode);
}

static inline
bool is_file(const std::string &filename){
	struct stat st;
	if(stat(filename.c_str(), &st) == -1){
		return false;
	}
	return (bool)S_ISREG(st.st_mode);
}

// 从filename，读取到string content中。
static inline
int file_get_contents(const std::string &filename, std::string* content){
	char buf[8192];
	FILE* fp = fopen(filename.c_str(), "rb");	// 二进制形式读取
	if (!fp){
		return -1;
	}
	int ret = 0;
	while (!feof(fp) && ferror(fp)){
		int n = fread(buf, 1, sizeof(buf), fp);	// 遇到eof会自动结束,返回值为读取的内容数量.
		if (n > 0){
			ret += n;
			content->append(buf,n);
		}
	}
	fclose(fp);
	return ret;	// 返回读取是多少个字节
}

static inline
int file_put_contents(const std::string& filename, const std::string& content){
	FILE* fp = fopen(filename.c_str(), "wb");
	if (!fp){
		return -1;
	}
	int ret = fwrite(content.data(), 1, content.size(), fp);	// size = 1写入流
	fclose(fp);
	return ret == (int)content.size() ? ret : -1;
}

#endif

/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SIM_UTIL_FILE_H_
#define SIM_UTIL_FILE_H_

#include <sys/types.h>	// POSIX, 数据类型
#include <sys/stat.h>	// POSIX 文件的信息
#include <unistd.h>		// 多种必要的POSIX函数与常量
#include <string>

static inline
bool file_exists(const std::string &filename){
	struct stat st;
	return stat(filename.c_str(), &st) == 0;
}

static inline
bool is_dir(const std::string &filename){
	struct stat st;
	if(stat(filename.c_str(), &st) == -1){
		return false;
	}
	return (bool)S_ISDIR(st.st_mode);
}

static inline
bool is_file(const std::string &filename){
	struct stat st;
	if(stat(filename.c_str(), &st) == -1){
		return false;
	}
	return (bool)S_ISREG(st.st_mode);
}

// return number of bytes read
static inline
int file_get_contents(const std::string &filename, std::string *content){
	char buf[8192];
	FILE *fp = fopen(filename.c_str(), "rb");
	if(!fp){
		return -1;
	}
	int ret = 0;
	while(!feof(fp) && !ferror(fp)){
		int n = fread(buf, 1, sizeof(buf), fp);
		if(n > 0){
			ret += n;
			content->append(buf, n);
		}
	}
	fclose(fp);
	return ret;
}

// return number of bytes written
static inline
int file_put_contents(const std::string &filename, const std::string &content){
	FILE *fp = fopen(filename.c_str(), "wb");
	if(!fp){
		return -1;
	}
	int ret = fwrite(content.data(), 1, content.size(), fp);
	fclose(fp);
	return ret == (int)content.size()? ret : -1;
}

#endif
