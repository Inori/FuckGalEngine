#ifndef _TLGDECODER_H_
#define _TLGDECODER_H_

#include <Windows.h>
#include "my_headers.h"

#pragma pack(1)

#define KRKR2_TLG5_MAGIC "TLG5.0"

typedef struct
{
    BYTE    Magic[0xB];
    BYTE    Colors;
    ULONG   Width;
    ULONG   Height;
    ULONG   BlockHeight;
    ULONG   BlockOffset[1];
} KRKR2_TLG5_HEADER;

typedef struct
{
    BYTE    Magic[0xB];
    BYTE    Colors;
    BYTE    DataFlags;
    BYTE    ColorType;             // currently always zero
    BYTE    ExternalGolombTable;   // currently always zero
    ULONG   Width;
    ULONG   Height;
    ULONG   MaxBitLength;
    BYTE    Data[1];
} KRKR2_TLG6_HEADER;

typedef struct
{
    BYTE MetaHeader[0xF];
    union
    {
        KRKR2_TLG5_HEADER tlg5;
        KRKR2_TLG6_HEADER tlg6;
    };
} KRKR2_TLG_WITH_META_HEADER;

typedef struct
{
    s8 mark[11];		// "TLG6.0\x0raw\x1a"
    u8 colors;
    u8 flag;
    u8 type;
    u8 golomb_bit_length;
    u32 width;
    u32 height;
    u32 max_bit_length;
    u32 filter_length;
    //u8 *filter;
} tlg6_header_t;

#pragma pack()

BOOL DecodeTLG5(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize);
BOOL DecodeTLG6(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize);
LONG TVPTLG5DecompressSlide(UInt8 *out, const UInt8 *in, LONG insize, UInt8 *text, LONG initialr);

#endif /* _TLGDECODER_H_ */