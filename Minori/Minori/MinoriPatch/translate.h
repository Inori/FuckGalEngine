/******************************************************************************\
*  Copyright (C) 2014 Fuyin
*  ALL RIGHTS RESERVED
*  Author: Fuyin
*  Description: 定义字符串处理类
\******************************************************************************/

#ifndef STRINGMAP_H
#define STRINGMAP_H

#include <vector>
#include <string>
#include <unordered_map>
#include "scriptparser.h"
#include "types.h"

using std::vector;
using std::string;
using std::wstring;
using std::unordered_map;

class ScriptParser;
///////////////////////////////////////////////////////////////////////////////

//定义内存字符串，以兼容多字节和宽字符字符串
typedef struct _memstr
{
	uint strlen; //字符串字节长度
	uchar *str;    //字符串地址
}memstr;

typedef struct _HashString
{
	uint hash;
	memstr old_str; //为以后能同时显示日文做兼容
	memstr new_str;
}HashString;

//BKDR hash function
inline uint BKDRHash(const uchar *str, const uint len);

typedef unordered_map<uint, memstr> StringMap;

//字符串查找与翻译
class Translator
{
public:
	Translator();
	Translator(ScriptParser& parser);

	memstr SearchStr(uint hash); //根据一个DWORD在字典中查找字符串,DWORD可以是字符串hash值，也可以是内存地址等特征值
	memstr Translate(memstr str); //根据日文字符串获得中文字符串

	~Translator();
private:
	void insertString(acr_index *index, ulong index_count);
	StringMap strmap;
};



///////////////////////////////////////////////////////////////////////////////


class TranslateEngine
{
public:
	TranslateEngine();
	TranslateEngine(Translator& translator);

	bool Init(Translator& translator);
	
	bool Inject(void *dst, ulong dstlen);  //直接向目标地址注入匹配的字符串，注入成功返回真，否则返回假
	memstr MatchString(void *dst, ulong dstlen);  //返回匹配的字符串供手动编写代码处理。
	memstr MatchStringByOffset(ulong offset);  //根据地址直接返回匹配的字符串供手动编写代码处理。

	~TranslateEngine();

private:
	Translator *translator;
};



///////////////////////////////////////////////////////////////////////////////

class LogFile
{
public:
	LogFile();
	LogFile(string filename, uint open_mode);

	bool Init(string filename, uint open_mode);
	void AddLog(wstring logstr);
	void AddLog(string logstr, uint code_page);

	~LogFile();

private:
	HANDLE hfile;
};




#endif