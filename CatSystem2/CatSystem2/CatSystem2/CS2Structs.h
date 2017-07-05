#pragma once

#include "types.h"

#define CS2_INT_HEADER_MAGIC    ('FIK')
#define CS2_SCENE_MAGIC         ((ulonglong)'StaC' + ((ulonglong)'enec' << 32))
#define CS2_FES_MAGIC           ('SEF')
#define CS2_HG2_MAGIC           ('2-GH')
#define CS2_HG3_MAGIC           ('3-GH')

#pragma pack(1)

typedef struct
{
	uint Magic;
	uint dwFileNum;
	char KeyName[0x40];
	uint dwUnknown;
	uint dwKey;
} CS2IntHeader;

#define CS2_FILENAME_LEN 0x40
typedef struct
{
	char FileName[CS2_FILENAME_LEN];
	uint dwOffset;
	uint dwSize;
} CS2IntEntry;

typedef struct
{
	ulonglong Magic;
	uint CompressedSize;
	uint UncompressedSize;
	byte CompressedData[1];
} CS2SceneHeader, *PCS2SceneHeader;

typedef struct
{
	uint Magic;
	uint CompressedSize;
	uint UncompressedSize;
	uint Reserve;
	byte CompressedData[1];
} CS2FESHeader, *PCS2FESHeader;

typedef struct
{
	uint Magic;
	uint HeadSize;
	uint Version;
} CS2HG3Header;

typedef struct
{
	ulonglong Tag;
	uint AtomSize;
	uint InfoSize;
} CS2HG3AtomHeader;

typedef struct
{
	uint Width;
	uint Heigth;
	uint BitsPerPixel;
} CS2HG3AtomStdInfo;

typedef struct
{
	uint Unknown;
	uint Height;
	uint CompressedSizeRGB;
	uint DecompressedSizeRGB;
	uint CompressedSizeAlpha;
	uint DecompressedSizeAlpha;
} CS2HG3AtomImg;

#pragma pack()
