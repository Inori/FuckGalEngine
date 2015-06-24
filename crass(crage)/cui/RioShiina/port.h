#ifndef port_h
#define port_h

#include <windows.h>
#include <limits.h>

#undef GLOBALRANGECODER
#undef EXTRAFAST

#define Inline inline

typedef unsigned short uint2;  /* two-byte integer (large arrays)      */
typedef unsigned int   uint4;  /* four-byte integers (range needed)    */

typedef unsigned int uint;     /* fast unsigned integer, 2 or 4 bytes  */

#endif
