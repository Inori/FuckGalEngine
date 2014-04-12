#ifndef _CXDEC_H_24665dbf_c70d_4cad_b24e_656d8b1aea0e
#define _CXDEC_H_24665dbf_c70d_4cad_b24e_656d8b1aea0e

#include "my_headers.h"

class CCxdec
{
public:
    typedef ULONG (FASTCALL *GetMaskFunc)(ULONG Hash);
    typedef struct
    {
        PBYTE pbBuffer;
        PBYTE pbBufferCur;
        ULONG BufferSize;
        ULONG Seed;
    } DECRYPT_INFO;

    CCxdec();
    ~CCxdec();

    BOOL Init();
    Void Release();
    ULONG GetMask(ULONG &Hash);

protected:
    BOOL    GenerateCode(ULONG Index);
    BOOL    GenerateFunction(ULONG uLoop);
    BOOL    GenerateBody(ULONG uLoop);
    BOOL    GenerateBody2(ULONG uLoop);
    BOOL    GenerateTail();
    ULONG   GenerateSeed();

    BOOL    AppendBytes(ULONG uCount, PVOID lpBytes);
    BOOL    HasEnoughSpace(ULONG BytesToAppend);

protected:
    DECRYPT_INFO m_Info;
    PBYTE        m_pbFunction;

    static const ULONG m_FuncSize   = 0x80;
    static const ULONG m_FuncCount  = 0x80;
    static const ULONG m_EncryptBlock[0x400];
};


#define AddBytes1(...) \
    { \
        BYTE bytes[] = { __VA_ARGS__ }; \
        if (!AppendBytes(sizeof(bytes), bytes)) \
            return FALSE; \
    }

#define AddDword1(__val) \
    { \
        ULONG __v = (ULONG)(__val); \
        if (!AppendBytes(sizeof(__v), &__v)) \
            return FALSE; \
    }

#pragma warning(disable:437)

#define AddBytes(...) \
    { \
        BYTE bytes[] = { __VA_ARGS__ }; \
        if (!HasEnoughSpace(sizeof(bytes))) \
            return FALSE; \
        CopyStruct(m_Info.pbBufferCur, bytes, sizeof(bytes)); \
        m_Info.pbBufferCur += sizeof(bytes); \
    }

#define AddDword(__val) \
    { \
        ULONG __v = (ULONG)(__val); \
        if (!HasEnoughSpace(sizeof(__v))) \
            return FALSE; \
        CopyStruct(m_Info.pbBufferCur, &__v, sizeof(__v)); \
        m_Info.pbBufferCur += sizeof(__v); \
    }


#define ASM_PUSH_ECX    0x51
#define ASM_PUSH_EDX    0x52
#define ASM_PUSH_EBX    0x53
#define ASM_PUSH_ESI    0x56
#define ASM_PUSH_EDI    0x57

#endif // _CXDEC_H_24665dbf_c70d_4cad_b24e_656d8b1aea0e
