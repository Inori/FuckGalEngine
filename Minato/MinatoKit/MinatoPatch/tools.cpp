#include "tools.h"

///////////////共用工具//////////////////////////////////////////////////////////////
void SetNopCode(BYTE* pnop, size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	for (size_t i = 0; i<size; i++)
	{
		pnop[i] = 0x90;
	}
}

void memcopy(void* dest, void*src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dest, src, size);
}

DWORD wstrlen(wchar_t *ws)
{
	return 2 * wcslen(ws);
}

wchar_t *AnsiToUnicode(const char *str, uint code_page)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(code_page, 0, str, -1, NULL, 0);
	MultiByteToWideChar(code_page, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}

char *UnicodeToAnsi(const wchar_t *wstr, uint code_page)
{
	static char result[1024];
	int len = WideCharToMultiByte(code_page, 0, wstr, -1, NULL, 0, NULL, 0);
	WideCharToMultiByte(code_page, 0, wstr, -1, result, len, NULL, 0);
	result[len] = '\0';
	return result;
}


wstring addenter(wstring oldstr, uint linelen)
{
	wstring newstr;
	uint len = 0;

	for each (auto wc in oldstr)
	{
		newstr += wc;
		len++;
		if (wc == L'\n') len = 0;

		if (len == linelen)
		{
			newstr += L'\n';
			len = 0;
		}
	}

	return newstr;
}

wstring deleteenter(wstring oldstr)
{
	wstring newstr;
	for each (auto wc in oldstr)
	{
		if (wc != L'\n')
			newstr += wc;
	}
	return newstr;
}

//只换第一次出现的oldstr
string replace_first(string dststr, string oldstr, string newstr)
{
	string::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	string::size_type off = dststr.find(oldstr);
	if (off == string::npos)
		return dststr;

	string ret = dststr.replace(off, old_len, newstr);
	return ret;
}
//全部替换
string replace_all(string dststr, string oldstr, string newstr)
{
	string::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	string ret = dststr;
	string::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == string::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}

//只换第一次出现的oldstr
wstring replace_first(wstring dststr, wstring oldstr, wstring newstr)
{
	wstring::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	wstring::size_type off = dststr.find(oldstr);
	if (off == wstring::npos)
		return dststr;

	wstring ret = dststr.replace(off, old_len, newstr);
	return ret;
}
//全部替换
wstring replace_all(wstring dststr, wstring oldstr, wstring newstr)
{
	wstring::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	wstring ret = dststr;
	wstring::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == wstring::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}