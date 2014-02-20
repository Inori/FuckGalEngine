
#pragma once

#include "stdafx.h"

#ifndef UNICODE
#error Unable to compile without UNICODE
#endif

#define MEMPRTCT(x) (x + 1024)

#pragma pack(1)

struct HEADER
{
	unsigned char sig[2];
	unsigned char uk1[2];  
	DWORD index_length_ex; // full size
	DWORD header_lenth;
	DWORD index_offset;
	DWORD name_table_size; // file names' table size
	DWORD index_length; // real size
	DWORD charset; // WTF? // just use it!
};

 struct ENTRY_NAME
{
    DWORD flag;
    char upper_name[20];
	char lower_name[20];
};

struct ENTRY_INFO
{
    long tbhdr_distnc; // dummy
    DWORD stable_0x20; // dummy
	DWORD uk1;
	DWORD uk2;
	DWORD uk3;
	DWORD uk4;
	DWORD uk5;
	DWORD uk6;
	long file_nohdr_off; // should cut hdr size
	long file_uncprs_sz; // uncompressed size
	long file_inpack_sz; // compressed size
};

#pragma pack()

extern unsigned char key_r[12];
extern void decode(unsigned char* src, unsigned long off, unsigned char* key, unsigned long len);
extern void process(unsigned char* buf, const long len, const std::wstring& dir_n);
extern DWORD uncompress(BYTE *dst, BYTE *src);
