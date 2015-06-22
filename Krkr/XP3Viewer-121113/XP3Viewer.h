#ifndef _XP3VIEWER_H_07375c08_1527_4736_ac2b_ef0b4b62042e
#define _XP3VIEWER_H_07375c08_1527_4736_ac2b_ef0b4b62042e

#include "pragma_once.h"
#include <Windows.h>
#include "my_headers.h"
// #include "tp_stub.h"
#include <IStream.h>
#include "krkr2/tjs2/tjsCommHead.h"

#pragma pack(1)

typedef struct
{
    LONG    RefCount;
    PWCHAR  pszBuffer;
    WCHAR   szString[1];
} *PSttstr;

#pragma pack()

typedef struct
{
    BOOL                        InitDone;
    ULONG                       SizeOfImage;//, MatchMask, DesireMatchMask;
    ULONG_PTR                   OriginalReturnAddress, FirstSectionAddress, V2Unlink;
    HINSTANCE                   hInstance;
//    OBJECT_NAME_INFORMATION2    AppPath;
} XV_GLOBAL_INFO;

VOID InitExtractionThread();

#define MATCH_APPPATH   0x00000001
#define MATCH_SYSTEM    0x00000002
#define MATCH_PLUGIN    0x00000004

#endif // _XP3VIEWER_H_07375c08_1527_4736_ac2b_ef0b4b62042e
