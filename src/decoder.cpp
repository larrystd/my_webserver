#include "util/strings.h"
#include "util/log.h"
#include "sim.h"

namespace sim{

const static int BUF_RESIZE_TRIGGER = 16 * 1024;

int Decoder::push(const char *buf, int len){
	buffer.append(buf, len);
	//log_debug("'%s'", str_escape(buffer).c_str());
	return len;
}

/*
void *memchr(const void *str, int c, size_t n) 在参数 str 所指向的字符串的前 n 个字节中搜索第一次出现字符 c（一个无符号字符）的位置。
*/
int Decoder::parse(Message *msg){
	msg->reset();	// clear msg

	if(buffer_offset >= BUF_RESIZE_TRIGGER){
		//log_debug("resize buffer");
		buffer = std::string(buffer.data() + buffer_offset, buffer.size() - buffer_offset);
		buffer_offset = 0;
	}
	// 找到写入位置
	while(buffer.size() > buffer_offset && isspace(buffer[buffer_offset])){
		buffer_offset ++;
	}
	 
	if(buffer.size() == buffer_offset){
		return 0;
	}
	// 写入位置
	
	const char *key = buffer.data() + buffer_offset;
	// 搜索结束符的位置，msg_end
	const char *msg_end = (const char *)memchr(key, sim::MSG_END_BYTE, buffer.size() - buffer_offset);
	if(!msg_end){
		return 0;
	}
	int msg_len = msg_end - key + 1;
	int size = msg_len;
	
	int auto_tag = 0;
	while(1){
		int key_len = 0;
		int val_len;
		int tag;

		const char *end;
		end = (const char *)memchr(key, sim::KV_END_BYTE, size); // 找到KV_END符号 ' '
		// 兼容最后一个 空格 被省略的情况
		if(end == NULL){
			end = msg_end;
		}

		const char *val = (const char *)memchr(key, sim::KV_SEP_BYTE, end - key);//val的前一个位置
		if(val == NULL){
			val = key;
			tag = auto_tag;
		}else{
			val ++;
			key_len = val - key - 1;
			size -= key_len + 1;
			std::string key_s(key, key_len);	// key
			tag = str_to_int(key_s);
		}
	
		val_len = end - val;
		size -= val_len + 1;

		if(val_len > 0 && val[val_len - 1] == '\r'){
			val_len -= 1;
		}

		//printf("%u key: %u, val: %u\n", __LINE__, key_len, val_len);

		std::string val_s(val, val_len);
		msg->set(tag, val_s);

		key = end + 1;
		auto_tag = tag + 1;
		
		if(key >= msg_end){		//说明已经到末尾
			std::map<int, std::string>::iterator it;
			for(it=msg->fields_.begin(); it!=msg->fields_.end(); it++){
				it->second = sim::decode(it->second);	// 对val解码
			}
			buffer_offset += msg_len;
			//log_debug("msg.len: %d, buffer.len: %d", msg_len, buffer.size());
			return 1;
		}
	}
	return 0;
}

}; // namespace sim
