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

interface iTVPFunctionExporter
{
    virtual bool TJS_INTF_METHOD QueryFunctions(const tjs_char **name, void **function, tjs_uint count) = 0;
    virtual bool TJS_INTF_METHOD QueryFunctionsByNarrowString(const char **name, void **function, tjs_uint count) = 0;
};

#pragma pack()

#endif // _XP3VIEWER_H_07375c08_1527_4736_ac2b_ef0b4b62042e
