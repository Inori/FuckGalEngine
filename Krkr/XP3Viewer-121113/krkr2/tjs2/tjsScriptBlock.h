//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script Block Management
//---------------------------------------------------------------------------
#ifndef tjsScriptBlockH
#define tjsScriptBlockH

#include "tjsInterface.h"
#include "tjsInterCodeGen.h"
#include "tjsLex.h"
#include "tjs.h"

#include <list>

namespace TJS
{
//---------------------------------------------------------------------------
// tTJSScriptBlock - a class for managing the script block
//---------------------------------------------------------------------------
class tTJS;
class tTJSInterCodeContext;
class tTJSScriptBlock
{
public:
	tTJSScriptBlock(tTJS * owner);
	virtual ~tTJSScriptBlock();

private:
	tTJS * Owner;
	tjs_int RefCount;
	tjs_char *Script;
	tjs_char *Name;
	tjs_int LineOffset;

	std::list<tTJSInterCodeContext *> InterCodeContextList;

	std::stack<tTJSInterCodeContext *> ContextStack;

	tTJSLexicalAnalyzer *LexicalAnalyzer;

	tTJSInterCodeContext *InterCodeContext;

	std::vector<tjs_int> LineVector;
	std::vector<tjs_int> LineLengthVector;

	tTJSInterCodeContext * TopLevelContext;

	tTJSString FirstError;
	tjs_int FirstErrorPos;

	bool UsingPreProcessor;

public:
	tjs_int CompileErrorCount;

	tTJS * GetTJS() { return Owner; }

	void AddRef();
	void Release();

	void Add(tTJSInterCodeContext * cntx);
	void Remove(tTJSInterCodeContext * cntx);

	tjs_uint GetContextCount() const { return InterCodeContextList.size(); }
	tjs_uint GetTotalVMCodeSize() const;  // returns in VM word size ( 1 word = 32bit )
	tjs_uint GetTotalVMDataSize() const;  // returns in tTJSVariant count

	bool IsReusable() const { return GetContextCount() == 1 &&
		TopLevelContext != NULL && !UsingPreProcessor; }

	tjs_char * GetLine(tjs_int line, tjs_int *linelength) const;
	tjs_int SrcPosToLine(tjs_int pos) const;
	tjs_int LineToSrcPos(tjs_int line) const;

	ttstr GetLineDescriptionString(tjs_int pos) const;

	const tjs_char *GetScript() const { return Script; }

	void PushContextStack(const tjs_char *name, tTJSContextType type);
	void PopContextStack(void);
	void Parse(const tjs_char *script, bool isexpr, bool resultneeded);

	void SetFirstError(const tjs_char *error, tjs_int pos);

	tTJSLexicalAnalyzer * GetLexicalAnalyzer() { return LexicalAnalyzer; }
	tTJSInterCodeContext * GetCurrentContext() { return InterCodeContext; }

	const tjs_char *GetName() const { return Name; }
	void SetName(const tjs_char *name, tjs_int lineofs);
	ttstr GetNameInfo() const;

	tjs_int GetLineOffset() const { return LineOffset; }

	void NotifyUsingPreProcessor() { UsingPreProcessor = true; }

	void Dump() const;

private:
	static void ConsoleOutput(const tjs_char *msg, void *data);

public:
	static void (*GetConsoleOutput())(const tjs_char *msg, void *data)
		{ return ConsoleOutput; }

	void SetText(tTJSVariant *result, const tjs_char *text, iTJSDispatch2 * context,
		bool isexpression);

	void ExecuteTopLevelScript(tTJSVariant *result, iTJSDispatch2 * context);

};
//---------------------------------------------------------------------------
}

#endif
