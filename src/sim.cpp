#include <string>
#include "sim.h"

namespace sim{
std::string version(){
	return "1.0";
}

std::string encode(const std::string s, bool force_ascii){
	std::string ret;
	int size = (int)s.size();
	// 字符串重新编码
	for (int i = 0; i < size; i++){
		char c = s[i];
		/*
		1. char、short、int、long、bool四种基本类型都可以用于switch语句。
		2. float、double都不能用于switch语句。
		3. enum类型，即枚举类型可以用于switch语句。
		*/
		switch(c){
			case ' ':
				ret.append("\\s");	// 除break外
				break;
			case '\\':
				ret.append("\\\\");
				break;
			case '\a':
				ret.append("\\a");
				break;
			case '\b':
				ret.append("\\b");
				break;
			case '\f':
				ret.append("\\f");
				break;
			case '\v':
				ret.append("\\v");
				break;
			case '\r':
				ret.append("\\r");
				break;
			case '\n':
				ret.append("\\n");
				break;
			case '\t':
				ret.append("\\t");
				break;
			case '\0':
				ret.append("\\0");
				break;
			default:
				if (!force_ascii){
					ret.push_back(c);
				}else{
					static const char *hex = "0123456789abcdef";
					if(c >= '!' && c <= '~'){
						ret.push_back(c);
					}else{
						ret.append("\\x");
						unsigned char d = c;
						ret.push_back(hex[d >> 4]);
						ret.push_back(hex[d & 0x0f]);
					}
				}
				break;
		}

	}
	return ret;
}
//static修饰函数后，静态函数只能在该文件下被调用，对于静态函数，其声明和定义需要放在同一个文件下。
// static修饰过后的变量，只能在被定义的文件内部使用。也就使不能被其他文件用extern引用
// inline不加static,倘若没内联，该函数可能会被其他文件extern引用，从而重复定义。即A定义一次，B调用A又定义一次。
// 加了static，只会本文件可见，其他文件不可extern static变量。static定义的变量并非是全局变量
// 每一个cpp文件生成一个.o，保证该.o文件没有重复定义，编译的行为。
// 解决重复编译，用#ifndef宏定义
// 而头文件可能被多文件include,需要避免重定义的情况。
// extern引用的变量必须是全局变量。
inline static
int hex_int(char c){
	if (c >= '0' && c <= '9'){
		return c - '0';
	}else{
		return c - 'a' + 10;
	}
}

std::string decode(const std::string s){
	int size = (int)s.size();
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		if(c != '\\'){
			ret.push_back(c);
			continue;
		}
		if(i >= size - 1){
			break;
		}
		char c2 = s[++i];
		switch(c2){
			case 's':
				ret.push_back(' ');
				break;
			case '\\':
				ret.push_back('\\');
				break;
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
			case '0':
				ret.push_back('\0');
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
	return ret;
}
};	// namespace sim

#include <string>
#include "sim.h"

namespace sim{

std::string version(){
	return "1.0";
}

std::string encode(const std::string s, bool force_ascii){
	std::string ret;
	int size = (int)s.size();
	for(int i=0; i<size; i++){
		char c = s[i];
		switch(c){
			case ' ':
				ret.append("\\s");
				break;
			case '\\':
				ret.append("\\\\");
				break;
			case '\a':
				ret.append("\\a");
				break;
			case '\b':
				ret.append("\\b");
				break;
			case '\f':
				ret.append("\\f");
				break;
			case '\v':
				ret.append("\\v");
				break;
			case '\r':
				ret.append("\\r");
				break;
			case '\n':
				ret.append("\\n");
				break;
			case '\t':
				ret.append("\\t");
				break;
			case '\0':
				ret.append("\\0");
				break;
			default:
				if(!force_ascii){
					ret.push_back(c);
				}else{
					// TODO: 只对非 UTF-8 字符进行转义
					static const char *hex = "0123456789abcdef";
					if(c >= '!' && c <= '~'){
						ret.push_back(c);
					}else{
						ret.append("\\x");
						unsigned char d = c;
						ret.push_back(hex[d >> 4]);
						ret.push_back(hex[d & 0x0f]);
					}
				}
				break;
		}
	}
	return ret;	
}

inline static
int hex_int(char c){
	if(c >= '0' && c <= '9'){
		return c - '0';
	}else{
		return c - 'a' + 10;
	}
}

std::string decode(const std::string s){
	int size = (int)s.size();
	std::string ret;
	for(int i=0; i<size; i++){
		char c = s[i];
		if(c != '\\'){
			ret.push_back(c);
			continue;
		}
		if(i >= size - 1){
			break;
		}
		char c2 = s[++i];
		switch(c2){
			case 's':
				ret.push_back(' ');
				break;
			case '\\':
				ret.push_back('\\');
				break;
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
			case '0':
				ret.push_back('\0');
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
	return ret;
}

}; // namespace sim

