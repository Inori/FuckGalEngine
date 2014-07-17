#include <Windows.h>
#include <wchar.h>


wchar_t *AnsiToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
	MultiByteToWideChar(932, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}


int main()
{
	char *s = "\139\194\140\252\130\175\130\201\130\200\130\193\130\189\137\180";
	wprintf(L"%s", AnsiToUnicode(s));
	return 0;
}