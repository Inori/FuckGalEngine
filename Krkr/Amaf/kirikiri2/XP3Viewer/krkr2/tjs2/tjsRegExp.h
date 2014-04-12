//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Regular Expression Support
//---------------------------------------------------------------------------
#ifndef tjsRegExpH
#define tjsRegExpH

#include <boost/regex.hpp>
#include "tjsNative.h"

#ifdef BOOST_NO_WREGEX
#error "wregex is not usable! see details at tjsRegExp.h"

gcc g++ has a lack of wstring.

you must modify:

include/g++-3/string: uncomment this:
typedef basic_string <wchar_t> wstring;


include/g++-3/std/bastring.h: 
    { if (length () == 0) return ""; terminate (); return data (); }

is not usable in wstring; fix like

    { static charT zero=(charT)0; if (length () == 0) return &zero; terminate (); return data (); }


boost/config/stdlib/libstdcpp3.hpp: insert this to the beginning of the file:
#define _GLIBCPP_USE_WCHAR_T

or uncomment in sgi.hpp

#     define BOOST_NO_STD_WSTRING

#endif

namespace TJS
{
typedef boost::reg_expression<tjs_char> tTJSRegEx;
//---------------------------------------------------------------------------
// tTJSNI_RegExp
//---------------------------------------------------------------------------
class tTJSNI_RegExp : public tTJSNativeInstance
{
public:
	tTJSNI_RegExp();
	tTJSRegEx RegEx;
	tjs_uint32 Flags;
	tjs_uint Start;
	tTJSVariant Array;
	tjs_uint Index;
	ttstr Input;
	tjs_uint LastIndex;
	ttstr LastMatch;
	ttstr LastParen;
	ttstr LeftContext;
	ttstr RightContext;

private:

public:
	void Split(iTJSDispatch2 ** array, const ttstr &target, bool purgeempty);
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNC_RegExp
//---------------------------------------------------------------------------
class tTJSNC_RegExp : public tTJSNativeClass
{
public:
	tTJSNC_RegExp();

	static void Compile(tjs_int numparam, tTJSVariant **param, tTJSNI_RegExp *_this);
	static bool Match(boost::match_results<const tjs_char *>& what,
		ttstr target, tTJSNI_RegExp *_this);
	static bool Exec(boost::match_results<const tjs_char *>& what,
		ttstr target, tTJSNI_RegExp *_this);
	static iTJSDispatch2 * GetResultArray(bool matched, tTJSNI_RegExp *_this,
		const boost::match_results<const tjs_char *>& what);

private:
	tTJSNativeInstance *CreateNativeInstance();

public:
	static tjs_uint32 ClassID;

	static tTJSVariant LastRegExp;
};
//---------------------------------------------------------------------------
extern iTJSDispatch2 * TJSCreateRegExpClass();
//---------------------------------------------------------------------------

} // namespace TJS

#endif
