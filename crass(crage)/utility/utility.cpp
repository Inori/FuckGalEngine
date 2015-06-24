#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <utility.h>
#include <crass/locale.h>

/******* JCrage *******/

static SOCKET jcrage_sock = INVALID_SOCKET;

static DWORD jcrage_print(const TCHAR *msg);

UTILITY_API void jcrage_exit(void)
{
	if (jcrage_sock != INVALID_SOCKET) {
		closesocket(jcrage_sock);
		WSACleanup();
		jcrage_sock = INVALID_SOCKET;
	}
}

static int jcrage_handshake(void)
{
	if (jcrage_sock != INVALID_SOCKET)
		return 0;

	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);

	if (WSAStartup(wVersionRequested, &wsaData))
		return -1;

	if (LOBYTE(wsaData.wVersion) != 1 ||
			HIBYTE(wsaData.wVersion) != 1) {
		WSACleanup();
		return -1;
	}

	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		WSACleanup();
		return -1;
	}

	jcrage_sock = sock;

	struct sockaddr_in sa;
	sa.sin_family = PF_INET;
	sa.sin_port = htons(1987);
	sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa))) {		
		closesocket(sock);
		WSACleanup();
		jcrage_sock = INVALID_SOCKET;
		return -1;
	}

	jcrage_print(_T("crage ready\n"));

#if 0
#define JCRAGE_ACK	"jcrage ack"
	char ack_buf[16];
	int act_ack_len = strlen(JCRAGE_ACK) + 1;
	int ack_len = recv(sock, ack_buf, sizeof(ack_buf), 0);
	if (ack_len != act_ack_len) {
		closesocket(sock);
		WSACleanup();
		jcrage_sock = INVALID_SOCKET;
		return -1;
	}

	if (strncmp(ack_buf, JCRAGE_ACK, act_ack_len)) {
		closesocket(sock);
		WSACleanup();
		jcrage_sock = INVALID_SOCKET;
		return -1;
	}
#endif

	return 0;
}

static DWORD jcrage_print(const TCHAR *msg)
{
	return send(jcrage_sock, (char *)msg, lstrlen(msg) * 2, 0);
}

/******* CrageGui *******/

#pragma data_seg("CrageGUI")
static TCHAR tMsgInfo[64 * 1024] = { 0 }; //消息
static HWND hWnd = NULL;	//目的窗口句柄
#pragma data_seg()
#pragma comment(linker, "/section:CrageGUI,rws")

//定义字符串消息
#define WM_MYSTRING       WM_USER+0x100

static DWORD cragegui_print(const TCHAR *fmt)
{
	DWORD len = lstrlen(fmt);
	lstrcpyn(tMsgInfo, fmt, sizeof(tMsgInfo) / sizeof(TCHAR));
	SendMessage(hWnd, WM_MYSTRING, (WPARAM)0, (LPARAM)len);
	return len;
}

extern "C" UTILITY_API void GetInfo(TCHAR *buff, DWORD dwSize)
{
	lstrcpyn(buff, tMsgInfo, dwSize);
}

extern "C" UTILITY_API LRESULT SetMsgWnd(HWND hwnd)
{
	hWnd = hwnd;
	return 0;
}

UTILITY_API unsigned int warnning_verbose0;
UTILITY_API unsigned int warnning_exit;
UTILITY_API unsigned int use_gui_wrapper;
UTILITY_API unsigned int use_jcrage_wrapper;

static HANDLE hconsole = INVALID_HANDLE_VALUE;
static HANDLE hconsole_error = INVALID_HANDLE_VALUE;
static HANDLE hconsole_input = INVALID_HANDLE_VALUE;
static TCHAR options_line[1024];
static char options_line_ansi[1024];
static unsigned int options_nr;

static DWORD wscanf_console(TCHAR *fmt, SIZE_T fmt_size)
{
	DWORD ret = 0;

	if (hconsole_input != INVALID_HANDLE_VALUE) {
		FlushConsoleInputBuffer(hconsole_input);
		ReadConsole(hconsole_input, fmt, fmt_size, &ret, NULL);
	} else {
		hconsole_input = GetStdHandle(STD_INPUT_HANDLE);
		if (hconsole_input != INVALID_HANDLE_VALUE) {
			FlushConsoleInputBuffer(hconsole_input);
			ReadConsole(hconsole_input, fmt, fmt_size, &ret, NULL);
		}
	}

	return ret;
}

static DWORD wscanf_jcrage(TCHAR *fmt, SIZE_T fmt_size)
{
	if (jcrage_sock == INVALID_SOCKET) 
		return 0;

	return recv(jcrage_sock, (char *)fmt, fmt_size - 2, 0);
}

static DWORD wcprintf_console(const TCHAR *fmt)
{
	DWORD ret = 0;

	if (hconsole != INVALID_HANDLE_VALUE) {
		WriteConsole(hconsole, fmt, _tcslen(fmt), &ret, NULL);
		FlushConsoleInputBuffer(hconsole);
	} else {
		hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hconsole != INVALID_HANDLE_VALUE) {			
			WriteConsole(hconsole, fmt, _tcslen(fmt), &ret, NULL);
			FlushConsoleInputBuffer(hconsole);
		}
	}

	return ret;
}

static DWORD wcprintf_error_console(const TCHAR *fmt)
{
	DWORD ret = 0;

	if (hconsole_error != INVALID_HANDLE_VALUE)
		WriteConsole(hconsole_error, fmt, _tcslen(fmt), &ret, NULL);
	else {
		hconsole_error = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hconsole_error != INVALID_HANDLE_VALUE)
			WriteConsole(hconsole_error, fmt, _tcslen(fmt), &ret, NULL);
	}
	
	return ret;
}

static DWORD wcprintf_gui(const TCHAR *fmt)
{
	DWORD ret;

	if (hWnd)
		ret = cragegui_print(fmt);
	else
		ret = 0;

	return ret;
}

static DWORD wcprintf_error_gui(const TCHAR *fmt)
{
	DWORD ret;

	if (hWnd)
		ret = cragegui_print(fmt);
	else
		ret = 0;

	return ret;
}

static DWORD wcprintf_jcrage(const TCHAR *fmt)
{
	if (jcrage_sock == INVALID_SOCKET) {
		if (jcrage_handshake()) {
			use_jcrage_wrapper = 0;
			return 0;
		}
	}
	
	return jcrage_print(fmt);
}

UTILITY_API DWORD wcprintf(const TCHAR *fmt)
{
	return use_jcrage_wrapper ? wcprintf_jcrage(fmt) :
		(use_gui_wrapper ? wcprintf_gui(fmt) : wcprintf_console(fmt));
}

UTILITY_API DWORD wscanf(TCHAR *fmt, SIZE_T fmt_size)
{
	return use_jcrage_wrapper ? wscanf_jcrage(fmt, fmt_size) :
		(use_gui_wrapper ? 0 : wscanf_console(fmt, fmt_size));
}

UTILITY_API DWORD wcprintf_error(const TCHAR *fmt)
{
	return use_jcrage_wrapper ? wcprintf_jcrage(fmt) :
		(use_gui_wrapper ? wcprintf_error_gui(fmt) : wcprintf_error_console(fmt));
}

UTILITY_API int crass_printf(const TCHAR *msg, ...)
{
	va_list args;
	int ret;
	TCHAR fmt_buf[16 * 1024];

	va_start(args, msg);
	ret = _vstprintf(fmt_buf, msg, args);
	wcprintf(fmt_buf);
	va_end(args);

	return ret;
}

UTILITY_API int locale_printf(DWORD id, ...)
{
	va_list args;
	int ret;
	TCHAR fmt_buf[16 * 1024];

	if (id != -1)
		*((const TCHAR **)&id) = locale_load_string(id);

	if (!id)
		return 0;

	va_start(args, id);
	ret = _vstprintf(fmt_buf, (const TCHAR *)id, args);
	wcprintf(fmt_buf);
	va_end(args);

	return ret;
}

UTILITY_API int locale_app_printf(DWORD cid, DWORD id, ...)
{
	va_list args;
	int ret;
	TCHAR fmt_buf[16 * 1024];

	if (id != -1)
		*((const TCHAR **)&id) = locale_app_load_string(cid, id);

	if (!id)
		return 0;

	va_start(args, id);
	ret = _vstprintf(fmt_buf, (const TCHAR *)id, args);
	wcprintf(fmt_buf);
	va_end(args);

	return ret;
}

UTILITY_API int unicode2ansi(int loc, char *ansi, int ansi_len, const TCHAR *unicode, int unicode_len)
{
	BOOL flag = 0;

	/* file name convert */
	if (!WideCharToMultiByte(loc, 0, unicode, unicode_len, ansi, ansi_len, NULL, &flag))
		return -1;	

	if (flag)
		return -1;

	return 0;
}

#ifdef UNICODE
#if 1
UTILITY_API int ansi2unicode(int loc, const char *ansi, int ansi_len, TCHAR *buf, int buf_len)
{
	return !MultiByteToWideChar(loc, 0, (LPCSTR)ansi, ansi_len, buf, buf_len) ? -1 : 0;
}
#else
UTILITY_API int ansi2unicode(int loc, const char *ansi, int ansi_len, TCHAR *buf, int buf_len)
{
	int wide_len;
	LPWSTR wide_str;

	/* file name convert */
	wide_len = MultiByteToWideChar(loc, 0, (LPCSTR)ansi, ansi_len, NULL, 0);
	if (!wide_len)
		return -1;	

	wide_str = (LPWSTR)GlobalAlloc(GMEM_FIXED, wide_len * 2);
	if (!wide_str) {
		_stprintf(fmt_buf, _T("Out of memory for allocate unicode string.\n"));
		wcprintf_error(fmt_buf);
		return -1;	
	}

	if (!MultiByteToWideChar(loc, 0, (LPCSTR)ansi, ansi_len, wide_str, wide_len)) {
		_stprintf(fmt_buf, _T("Can't convert shift-jis to unicode.\n"));
		wcprintf_error(fmt_buf);
		GlobalFree(wide_str);
		return -1;	
	}
	wcsncpy(buf, wide_str, buf_len);
	if (wide_len >= buf_len)
		buf[buf_len - 1] = 0;
	else
		buf[wide_len - 1] = 0;
	GlobalFree(wide_str);
	return 0;
}
#endif
#else
UTILITY_API int ansi2unicode(int loc, const char *ansi, int ansi_len, TCHAR *buf, DWORD buf_len)
{
	int wide_len, multi_chars;
	LPWSTR wide_str;

	/* file name convert */
	wide_len = MultiByteToWideChar(loc, 0, (LPCSTR)ansi, ansi_len, NULL, 0);
	if (!wide_len)
		return -1;	

	wide_str = (LPWSTR)GlobalAlloc(GMEM_FIXED, wide_len * 2);
	if (!wide_str) {
		_stprintf(fmt_buf, _T("Out of memory for allocate unicode string.\n"));
		wcprintf_error(fmt_buf);
		return -1;	
	}

	if (!MultiByteToWideChar(loc, 0, (LPCSTR)ansi, ansi_len, wide_str, wide_len)) {
		_stprintf(fmt_buf, _T("Can't convert shift-jis to unicode.\n"));
		wcprintf_error(fmt_buf);
		GlobalFree(wide_str);
		return -1;	
	}

	/* convert to ANSI locale */
	multi_chars = WideCharToMultiByte(CP_ACP, 0, wide_str, -1,
			NULL, 0, "_", NULL);
	if (!multi_chars) {
		_stprintf(fmt_buf, _T("Can't convert unicode filename to ANSI.\n"));
		wcprintf_error(fmt_buf);
		GlobalFree(wide_str);
		return -1;
	}
	ansi = (char *)GlobalAlloc(GMEM_FIXED, multi_chars + 1);
	if (!ansi) {
		_stprintf(fmt_buf, _T("Out of memory for allocate ANSI string.\n"));
		wcprintf_error(fmt_buf);
		GlobalFree(wide_str);
		return NULL;
	}
	if (!WideCharToMultiByte(CP_ACP, 0, wide_str, -1, ansi, multi_chars, 
			"_", NULL)) {
		_stprintf(fmt_buf, _T("Can't convert unicode filename to ANSI.\n"));
		wcprintf_error(fmt_buf);
		GlobalFree(ansi);
		GlobalFree(wide_str);
		return NULL;
	}
	GlobalFree(wide_str);

	_mbsncpy((BYTE *)buf, (BYTE *)ansi, buf_len - 1);
	buf[buf_len - 1] = 0;
	GlobalFree(ansi);
	return 0;
}
#endif

UTILITY_API int parse_options(const TCHAR *option)
{
	TCHAR *p, *str;
	
	if (!option)
		return -1;

	memset(options_line, 0, sizeof(options_line));
	_tcsncpy(options_line, option, SOT(options_line));
	p = options_line;
	while ((str = _tcsstr(p, _T(",")))) {
		*str = 0;
		p = str + 1;
		options_nr++;
	}
	if (*p)
		options_nr++;

	return options_nr ? 0 : -1;
}

UTILITY_API const char *get_options(const char *key)
{	
	unsigned int i;
	int key_len = strlen(key);
	char *p = options_line_ansi;
	TCHAR *up = options_line;

	for (i = 0; i < options_nr; i++) {
		unicode2acp(p, sizeof(options_line_ansi), up, -1);
		const char *pre_key = NULL;
		const char *post_key = NULL;
		int pre_key_len = 0;
		int post_key_len = 0;

		for (int k = 0; k < key_len; k++) {
			if (key[k] == '*') {
				if (!k) {
					pre_key = NULL;	
					pre_key_len = 0;
				} else {
					pre_key = key;
					pre_key_len = k;
				}

				if (k == key_len - 1) {
					post_key = NULL;
					post_key_len = 0;
				} else {
					post_key = &key[k + 1];
					post_key_len = key_len - k - 1;
				}
				break;	/* 只允许一个通配符 */
			}
		}

		if (!pre_key && !post_key) {
			if (!strncmp(p, key, key_len)) {
				if (p[key_len] == '=')	/* value */
					return p + key_len + 1;
				else if (!p[key_len])	/* no value */
					return p;
			}
		} else {
			if (pre_key) {
				if (!strncmp(p, pre_key, pre_key_len)) {
					if (post_key) {
						if (strstr(p, post_key))
							return p;
					}
				}		
			}

			if (post_key) {
				if (strstr(p, post_key))
					return p;
			}
		}
		p += strlen(p) + 1;
		up += _tcslen(up) + 1;
	}
	
	return NULL;
}

UTILITY_API const TCHAR *get_options2(const TCHAR *key)
{	
	unsigned int i;
	TCHAR *p;
	int key_len = _tcslen(key);

	p = options_line;
	for (i = 0; i < options_nr; i++) {
		const TCHAR *pre_key = NULL;
		const TCHAR *post_key = NULL;
		int pre_key_len = 0;
		int post_key_len = 0;

		for (int k = 0; k < key_len; k++) {
			if (key[k] == _T('*')) {
				if (!k) {
					pre_key = NULL;	
					pre_key_len = 0;
				} else {
					pre_key = key;
					pre_key_len = k;
				}

				if (k == key_len - 1) {
					post_key = NULL;
					post_key_len = 0;
				} else {
					post_key = &key[k + 1];
					post_key_len = key_len - k - 1;
				}
				break;	/* 只允许一个通配符 */
			}
		}

		if (!pre_key && !post_key) {
			if (!_tcsncmp(p, key, key_len)) {
				if (p[key_len] == _T('='))	/* value */
					return p + key_len + 1;
				else if (!p[key_len])	/* no value */
					return p;
			}
		} else {
			if (pre_key) {
				if (!_tcsncmp(p, pre_key, pre_key_len)) {
					if (post_key) {
						if (_tcsstr(p, post_key))
							return p;
					}
				}		
			}

			if (post_key) {
				if (_tcsstr(p, post_key))
					return p;
			}
		}
		p += _tcslen(p) + 1;
	}
	
	return NULL;
}

UTILITY_API alpha_blending(BYTE *dib, DWORD width, DWORD height, DWORD bpp)
{
	if (bpp == 32) {
		BYTE *p = dib;
		for (DWORD i = 0; i < width * height; ++i) {
			p[0] = p[0] * p[3] / 255 + 255 - p[3];
			p[1] = p[1] * p[3] / 255 + 255 - p[3];
			p[2] = p[2] * p[3] / 255 + 255 - p[3];
			p += 4;
		}
	}
}

UTILITY_API alpha_blending_reverse(BYTE *dib, DWORD width, DWORD height, DWORD bpp)
{
	if (bpp == 32) {
		BYTE *p = dib;
		for (DWORD i = 0; i < width * height; ++i) {
			p[3] = ~p[3];
			p[0] = p[0] * p[3] / 255 + 255 - p[3];
			p[1] = p[1] * p[3] / 255 + 255 - p[3];
			p[2] = p[2] * p[3] / 255 + 255 - p[3];
			p += 4;
		}
	}
}

