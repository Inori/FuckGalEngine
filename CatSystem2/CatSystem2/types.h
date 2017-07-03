/******************************************************************************\
*  Copyright (C) 2014 Fuyin
*  ALL RIGHTS RESERVED
*  Author: Fuyin
*  Description: 常用类型定义
\******************************************************************************/
#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uchar;
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef void VOID;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned long* PULONG;
typedef ULONG ULONG_PTR;
//

#define countof(a)   (sizeof(a)/sizeof(*(a)))

#endif