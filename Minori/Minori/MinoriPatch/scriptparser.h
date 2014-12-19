#ifndef SCRIPTPARSER_H
#define SCRIPTPARSER_H
#include <string>
#include <vector>
#include "types.h"

using std::string;
using std::vector;

typedef void *HANDLE;

//取消对齐
#pragma pack(1)

//脚本封包文件头
typedef struct _acr_header
{
	ulong index_count; //包含索引数，即字符串数
	ulong compress_flag; //是否压缩。 0没有压缩
	ulong compsize; //如果有压缩，为压缩后大小，否则等于orgsize
	ulong orgsize; //如果有压缩，为压缩前【压缩部分】大小，否则为实际大小
}acr_header;

typedef struct _acr_index
{
	ulong hash; //oldstr的hash值，用于map查找
	ulong old_str_off; //oldstr 地址
	ulong old_str_len; //oldstr 字节长度
	ulong new_str_off; //newstr 地址
	ulong new_str_len; //newstr 长度
}acr_index;

#pragma pack()

//接口
class ScriptParser
{
public:
	virtual acr_index *Parse() = 0;
	virtual ulong GetStrCount() = 0;
};

//二进制封包分析
class AcrParser: public ScriptParser
{
public:
	AcrParser();
	AcrParser(string fname);
	AcrParser(const AcrParser& orig);

	bool Init(string fname);
	acr_index *Parse();
	ulong GetStrCount();

	~AcrParser();
private:
	HANDLE hfile; //封包句柄
	acr_index *index_list;

	ulong real_size;
	byte *real_data; //去除文件头后的部分,包括索引和字符串数据.游戏退出前不可释放！

	ulong index_count;
	bool is_compressed;
};


//文本文件分析，主要用于调试阶段
class TextParser: public ScriptParser
{
public:
	TextParser();
	TextParser(string fname);
	TextParser(const TextParser& orig);

	acr_index *Parse();
	ulong GetStrCount();

	~TextParser();

private:
	FILE *fin;
	ulong file_size;

	vector<acr_index> index; //索引数据
	byte *real_data;  //字符串数据

};















#endif 