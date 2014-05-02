/*
  as-util.h 2011/06/12
  by 7k
  email: blxode [at] gmail.com 

  modified by Fuyin 
  2014/5/2
  email: 811197881 [at] qq.com
*/


#ifndef _AS_UTIL_H_
#define _AS_UTIL_H_

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <fcntl.h>
#include <direct.h>
#include <stdlib.h>
#include <io.h>
#include <cctype>
#include <algorithm>


using namespace std;

namespace as
{
	int open_or_die(string in_filename, const int type)
	{
		const char *fn = in_filename.c_str();
		int fd = open(fn, type);
		if(fd == NULL)
		{
			cerr<<"Could not open "<<fn<<" (No such file or directory)"<<endl;
			exit(1);
		}
		return fd;
	}
	
	unsigned long get_file_size(int fd)
	{
		return filelength(fd);
	}

	string stringtol(string str)
	{
		string s = str.substr(str.find_last_of('/') + 1);
		transform(s.begin(), s.end(), s.begin(), tolower);
		return s;
	}

	string get_file_prefix(string filename, bool cut_dot)
	{
		if (cut_dot)
			return filename.substr(0, filename.find_last_of("."));
		else
			return filename.substr(0, filename.find_last_of(".")+1);
	}

	//考虑难度，这里只写出符号本次使用的代码
	string stringf(const char *fmt, unsigned long c)
	{
		char temp[256] = {0};
		sprintf(temp, fmt, c);
		return string(temp);
	}

	bool is_file_readable(string filename)
	{
		return (access(filename.c_str(), 04) == 0);
	}

 	bool mkdirEx(const char* lpPath)
	{
		string pathname = lpPath;

		if(pathname[pathname.length()-1] != '\\')	pathname += '\\' ;
		int end = pathname.rfind('\\');
		int pt = pathname.find('\\');
		if (pathname[pt-1] == ':')	pt = pathname.find('\\', pt+1);
		const char * path;
		while(pt != -1 && pt<=end)
		{
			string p = pathname.substr(0, pt + 1);
			path = p.c_str();
			if(access(path, 0) == -1)
				mkdir(path);
			pt = pathname.find('\\', pt+1);
		}
		
		return true;
	}

	void make_path(string filename)
	{
		unsigned long i;
		for(i = filename.length(); i >0 ; i--)
		{
			if(filename[i] == '\\')	break;
		}
		string path=filename.substr(0,i);
		if (path == "")
			return;
		mkdirEx(path.c_str());
	}
	
	void write_file(string filename, unsigned char *buff, unsigned long len)
	{
		const char *fn = filename.c_str();
		fstream file;
		file.open(fn,ios::binary | ios::out);
		if(!file.is_open())
		{
			cerr<<"Could not open "<<fn<<" (Permission denied)"<<endl;
			exit(1);
		}
		file.write((const char *)buff,len);
		file.close();
	}
};

#endif