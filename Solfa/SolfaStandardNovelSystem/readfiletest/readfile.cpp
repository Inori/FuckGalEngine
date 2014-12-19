// regex2.cpp : 定义控制台应用程序的入口点。
//

#include <regex>
#include <string>

#include <fcntl.h>
#include <io.h>

int wmain(int argc, wchar_t* argv[])
{
	//if (argc ^ 2) return 0;

	_setmode(_fileno(stdout), _O_U16TEXT);

	FILE* fp;
	fp = _wfopen(L"game_text.txt", L"rb");
	fseek(fp, 0, 2);
	int len = ftell(fp);
	fseek(fp, 0, 0);

	wchar_t* buf = new wchar_t[len / 2 + 1];

	fread(buf, 2, len / 2, fp);
	fclose(fp);

	wchar_t* tmp = new wchar_t[200];

	int bi = 0;
	int ti = 0;

	std::wregex r(L"○([[:xdigit:]]{8})○([[:xdigit:]]{8})●");

	for (; bi < len / 2;)
	{
		if (buf[bi] != 0xd && buf[bi] != 0xa)
		{
			tmp[ti++] = buf[bi++];
			continue;
		}

		ti = tmp[ti] = 0;
		std::wstring ws(tmp);
		std::wsmatch m;
		if (std::regex_match(ws, m, r))
		{
			for (auto& p : m)
			{
				wprintf(L"*: %d ", wcstol(p.str().c_str(), 0, 16));
			}
		}
		else
		{
			if (ws.size() == 0) {}
			else
			{
				wprintf(L"\n%s\n", ws.c_str());
			}
		}

	Recheck:
		if (buf[bi] == 0xd || buf[bi] == 0xa)
		{
			++bi;
			goto Recheck;
		}
	}

	delete[] tmp;
	delete[] buf;

	return 0;
}

