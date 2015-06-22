/*=======================================================
				  DCOO Anti-Cheat Source
=========================================================*/

#ifndef _LOGFILE_H
#define _LOGFILE_H

#include <assert.h>
#include <time.h>
#include <stdio.h>

class LogFile
{
protected:
	CRITICAL_SECTION _csLock;
	char * _szFileName;
	HANDLE _hFile;

	bool OpenFile()//打开文件， 指针到文件尾
	{
		if(IsOpen())return true;
		if(!_szFileName)return false;
		_hFile =  CreateFile(
			_szFileName,
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);

		if(!IsOpen() && GetLastError() == 2)//打开不成功， 且因为文件不存在， 创建文件
			_hFile =  CreateFile(
			_szFileName, 
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL 
			);  

		if(IsOpen())
			SetFilePointer(_hFile, 0, NULL, FILE_END);

		return IsOpen();
	}

	DWORD Write(LPCVOID lpBuffer, DWORD dwLength)
	{
		DWORD dwWriteLength = 0;
		if(IsOpen())
			WriteFile(_hFile, lpBuffer, dwLength, &dwWriteLength, NULL);

		return dwWriteLength;
	}

	virtual void WriteLog( LPCVOID lpBuffer, DWORD dwLength)//写日志, 可以扩展修改
	{
		time_t now;
		char temp[21];
		DWORD dwWriteLength;

		if(IsOpen())
		{
			time(&now);
			strftime(temp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
			WriteFile(_hFile, "\xd\xa#-----------------------------", 32, &dwWriteLength, NULL);
			WriteFile(_hFile, temp, 19, &dwWriteLength, NULL);
			WriteFile(_hFile, "-----------------------------#\xd\xa", 32, &dwWriteLength, NULL);
			WriteFile(_hFile, lpBuffer, dwLength, &dwWriteLength, NULL);
			WriteFile(_hFile, "\xd\xa", 2, &dwWriteLength, NULL);
			//FlushFileBuffers(_hFile);
		}
	}

	void Lock()  { ::EnterCriticalSection(&_csLock); }
	void Unlock() { ::LeaveCriticalSection(&_csLock); }
public:
	LogFile(const char *szFileName = "Log.log")//设定日志文件名
	{
		_szFileName = NULL;
		_hFile = INVALID_HANDLE_VALUE;
		::InitializeCriticalSection(&_csLock);
		SetFileName(szFileName);
	}
	virtual ~LogFile()
	{
		::DeleteCriticalSection(&_csLock);
		Close();
		if(_szFileName)
			delete []_szFileName;
	}
	const char * GetFileName()
	{
		return _szFileName;
	}
	void SetFileName(const char *szName)//修改文件名， 同时关闭上一个日志文件
	{

		assert(szName);



		if(_szFileName)

			delete []_szFileName;



		Close();



		_szFileName = new char[strlen(szName) + 1];

		assert(_szFileName);

		strcpy(_szFileName, szName);

	}



	bool IsOpen()

	{

		return _hFile != INVALID_HANDLE_VALUE;

	}



	void Close()

	{

		if(IsOpen())

		{

			CloseHandle(_hFile);

			_hFile = INVALID_HANDLE_VALUE;

		}

	}



	void AddLog(LPCVOID lpBuffer, DWORD dwLength)//追加日志内容

	{

		assert(lpBuffer);

		__try

		{

			Lock();



			if(!OpenFile())

				return;



			WriteLog(lpBuffer, dwLength);

		}

		__finally

		{

			Unlock();

		} 

	}



	void __cdecl Log(const char * szString, ...)
	{

		TCHAR szEntry[1024];

		va_list args;

		va_start(args, szString);

		vsprintf(szEntry,szString,args);

		AddLog(szEntry, strlen(szEntry));

	}



	void GetErrorMessage(LPCTSTR lpFunction, DWORD dwError)

	{

		LPVOID lpMsgBuf;

		TCHAR szEntry[1024];

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language

			(LPTSTR)&lpMsgBuf, 0, NULL);

		sprintf(szEntry, "Function: %s, %s", lpFunction, (LPCTSTR)lpMsgBuf);

		AddLog(szEntry, strlen(szEntry));

		LocalFree(lpMsgBuf);

	}



private://屏蔽函数



	LogFile(const LogFile&);

	LogFile&operator = (const LogFile&);

};



class LogFileEx : public LogFile

{

protected:



	char *_szPath;

	char _szLastDate[9];

	int _iType;



	void SetPath(const char *szPath)

	{

		assert(szPath);



		WIN32_FIND_DATA wfd;

		char temp[MAX_PATH + 1] = {0};



		if(FindFirstFile(szPath, &wfd) == INVALID_HANDLE_VALUE && CreateDirectory(szPath, NULL) == 0)

		{

			strcat(strcpy(temp, szPath), " Create Fail. Exit Now! Error ID :");

			ltoa(GetLastError(), temp + strlen(temp), 10);

			MessageBox(NULL, temp, "Class LogFileEx", MB_OK);

			exit(1);

		}

		else

		{

			GetFullPathName(szPath, MAX_PATH, temp, NULL);

			_szPath = new char[strlen(temp) + 1];

			assert(_szPath);

			strcpy(_szPath, temp);

		}

	}



public:



	enum LOG_TYPE{YEAR = 0, MONTH = 1, DAY = 2};



	LogFileEx(const char *szPath = ".", LOG_TYPE iType = MONTH)

	{

		_szPath = NULL;

		SetPath(szPath);

		_iType = iType;

		memset(_szLastDate, 0, 9);

	}



	~LogFileEx()

	{

		if(_szPath)

			delete []_szPath;

	}



	const char * __cdecl GetPath()

	{

		return _szPath;

	}



	void __cdecl AddLog(LPCVOID lpBuffer, DWORD dwLength)

	{

		assert(lpBuffer);



		char temp[10];

		static const char format[3][10] = {"%Y", "%Y-%m", "%Y%m%d"};



		__try

		{

			Lock();



			time_t now = time(NULL);



			strftime(temp, 9, format[_iType], localtime(&now));



			if(strcmp(_szLastDate, temp) != 0)//更换文件名

			{

				strcat(strcpy(_szFileName, _szPath), "\\");

				strcat(strcat(_szFileName, temp), ".log");

				strcpy(_szLastDate, temp);

				Close();

			}



			if(!OpenFile())

				return;



			WriteLog(lpBuffer, dwLength);

		}

		__finally

		{

			Unlock();

		}

	}



	void __cdecl Log(const char * szString, ...)

	{

		TCHAR szEntry[1024];

		va_list args;

		va_start(args, szString);

		vsprintf(szEntry,szString,args);

		AddLog(szEntry, strlen(szEntry));

	}



private://屏蔽函数



	LogFileEx(const LogFileEx&);

	LogFileEx&operator = (const LogFileEx&);



};



#endif
