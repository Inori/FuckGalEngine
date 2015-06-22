//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// tjs common header
//---------------------------------------------------------------------------


/*
	Add headers that would not be frequently changed.
*/
#ifndef tjsCommHeadH
#define tjsCommHeadH

#ifdef TJS_SUPPORT_VCL
#include <vcl.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif


#include <string.h>
#ifndef __USE_UNIX98
#define __USE_UNIX98
#endif
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include <vector>
#include <string>
#include <stdexcept>

#include "tjsConfig.h"

#include "tjs.h"

typedef std::basic_string<TJS::tjs_char> stdstring;
typedef std::basic_string<TJS::tjs_nchar> stdnstring;

#ifdef TJS_SUPPORT_VCL
	#pragma intrinsic strcpy
	#pragma intrinsic strcmp  // why these are needed?
#endif

//---------------------------------------------------------------------------
#endif


