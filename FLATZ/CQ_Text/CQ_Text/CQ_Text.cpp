// CQ_Text.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CQ_Text.h"

int wmain(int argc, wchar_t* argv[])
{
    try
    {
        // 程序入口
        if(argc < 2)
        {
            fputs("CQ_Text.exe <input>", stderr);
            throw -1;
        }
        //////////////////////////
        // 获取文件缓存
        FILE* fp = 0;
        _wfopen_s(&fp, argv[1], L"rb");
        if(!fp) throw 2;
        fseek(fp, 0, SEEK_END);
        const long fl = ftell(fp);
        unsigned char* buf = new unsigned char[MEMPRTCT(fl)];
        rewind(fp);
        fread_s(buf, MEMPRTCT(fl), sizeof(char), fl, fp);
        fclose(fp);
        decode(buf, 0, key_r, fl);
        /////////////////////////
        // 建立文件夹
        wchar_t* flag = wcsrchr(argv[1], L'.');
        if(!flag)
        {
            flag = argv[1];
            wcscat_s(argv[1], wcslen(L"_dir"), L"_dir");
        }
        else
        {
            *flag = 0;
        }
        std::wstring dir_name = argv[1];
        CreateDirectoryW(dir_name.c_str(), 0);
        dir_name += L"\\";
        /////////////////////////
        // 拆解文件缓存
        process(buf, fl, dir_name);
        /////////////////////////
        // 退出程序
        delete [] buf;
        return 0;
    } 
    catch(int& e) 
    {
        return e;
    }
    catch(...)
    {
        return -3;
    }
}
