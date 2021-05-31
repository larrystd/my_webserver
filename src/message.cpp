#include "util/strings.h"
#include "sim.h"

namespace sim{

Message::Message(){

}
Message::~Message(){

}

void Message::reset(){
	fields_.clear();
}

std::string Message::type() const{
	return type_;
}

void Message::set_type(const std::string& type){
	type_ = type;
}

void Message::set(int tag, int64_t val){
	this->set(tag, str(val));
}

void Message::set(int tag, const char *val){
	this->set(tag, str(val));
}

void Message::set(int tag, const std::string& val){
	if (tag == 0){
		this->set_type(val);	// set type is string val
	}
	fields_[tag] = val;	// message中用map维护tag,val
}

/*
c++ -> equal to *.
this是指针，所以this->

iterator 　　　　　　　　   // 的作用相当于T*，
const_iterator 　　　  　 // 相当于 const T*（也可 T const*）,值不可修改
reverse_iterator 　　　　 // 相当于 T* 反向迭代器
const_reverse_iterator  // 相当于 const T*

rbegin()函数用于返回引用容器的最后一个元素的反向迭代器，反向迭代器沿相反方向移动并递增直到开头
*/
void Message::add(const std::string& val){
	int tag;
	std::map<int, std::string>::const_reverse_iterator it;
	it = fields_.rbegin();	// 最后一个元素
	if (it == fields_.rend()){	// field为空
		tag = 0;
	}else{
		tag = it->first+1;	// 新的tag,最后一个元素的tag值加1
	}
	this->set(tag,val);
}

const std::string* Message::get(int tag) const{
	std::map<int, std::string>::const_iterator it;
	it = fields_.find(tag);
	if(it == fields_.end()){
		return NULL;
	}
	return &(it->second);
}


/*
string
append sub_string of str, 	string& append (const string& str, size_t subpos, size_t sublen);

push_back() : append single character at a time 'a'.
*/
static std::string encode_field(int tag, const std::string &val){
	std::string buffer;
	buffer.append(str(tag));
	buffer.push_back(sim::KV_SEP_BYTE);	// '='
	buffer.append(sim::encode(val));
	buffer.push_back(sim::KV_END_BYTE);	// ' '
	return buffer;
}


/*
*/
std::string Message::encode() const{
	std::string buffer;
	buffer.append(encode_field(0, this->type()));
	std::map<int, std::string>::const_iterator it;
	for(it=fields_.begin(); it!=fields_.end(); it++){
		int tag = it->first;
		if(tag == 0){
			continue;
		}
		const std::string &val = it->second;
		buffer.append(encode_field(tag, val));
	}
	buffer[buffer.size()-1] = sim::MSG_END_BYTE;	// msg结束符
	return buffer;
}

};	// namespace sim

#include "util/strings.h"
#include "sim.h"

namespace sim{

Message::Message(){
}

Message::~Message(){
}

void Message::reset(){
	fields_.clear();
}

std::string Message::type() const{
	return type_;
}

void Message::set_type(const std::string &type){
	type_ = type;
}

void Message::set(int tag, int32_t val){
	this->set(tag, (int64_t)val);
}

void Message::set(int tag, int64_t val){
	this->set(tag, str(val));
}

void Message::set(int tag, const char *val){
	this->set(tag, str(val));
}

void Message::set(int tag, const std::string &val){
	if(tag == 0){
		this->set_type(val);
	}
	fields_[tag] = val;
}

void Message::add(const std::string &val){
	int tag;
	std::map<int, std::string>::const_reverse_iterator it;
	it = fields_.rbegin();
	if(it == fields_.rend()){
		tag = 0;
	}else{
		tag = it->first + 1;
	}
	this->set(tag, val);
}

const std::string* Message::get(int tag) const{
	std::map<int, std::string>::const_iterator it;
	it = fields_.find(tag);
	if(it == fields_.end()){
		return NULL;
	}
	return &(it->second);
}

static std::string encode_field(int tag, const std::string &val){
	std::string buffer;
	buffer.append(str(tag));
	buffer.push_back(sim::KV_SEP_BYTE);
	buffer.append(sim::encode(val));
	buffer.push_back(sim::KV_END_BYTE);
	return buffer;
}

std::string Message::encode() const{
	std::string buffer;
	buffer.append(encode_field(0, this->type()));
	std::map<int, std::string>::const_iterator it;
	for(it=fields_.begin(); it!=fields_.end(); it++){
		int tag = it->first;
		if(tag == 0){
			continue;
		}
		const std::string &val = it->second;
		buffer.append(encode_field(tag, val));
	}
	buffer[buffer.size()-1] = sim::MSG_END_BYTE;
	return buffer;
}

}; // namespace sim
