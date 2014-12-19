/******************************************************************************\
*  Copyright (C) 2014 Fuyin
*  ALL RIGHTS RESERVED
*  Author: Fuyin
*  Description: 工具函数
\******************************************************************************/

#ifndef TOOLS_H
#define TOOLS_H

#include <Windows.h>
#include <string>
using std::string;
using std::wstring;

#include "types.h"



wstring addenter(wstring oldstr, uint linelen);

wstring deleteenter(wstring oldstr);

void SetNopCode(BYTE* pnop, size_t size);

void memcopy(void* dest, void*src, size_t size);

DWORD wstrlen(wchar_t *ws);

wchar_t *AnsiToUnicode(const char *str, uint code_page);

char *UnicodeToAnsi(const wchar_t *wstr, uint code_page);

string replace_first(string dststr, string oldstr, string newstr);

//全部替换
string replace_all(string dststr, string oldstr, string newstr);

//只换第一次出现的oldstr
wstring replace_first(wstring dststr, wstring oldstr, wstring newstr);

//全部替换
wstring replace_all(wstring dststr, wstring oldstr, wstring newstr);



#endif TOOLS_H