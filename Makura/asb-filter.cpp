
// asb-filter.cpp  by  wiki908@github

#include <stdio.h>


int openfs(const wchar_t* fn, char* &buff)
{
    FILE* fs = _wfopen(fn, L"rb");
    if (fs == 0) throw 1;
    fseek(fs, 0, SEEK_END);
    int len = ftell(fs);
    fseek(fs, 0, SEEK_SET);
    buff = new char[len];
    fread(buff, 1, len, fs);
    if (ferror(fs)) throw 1;
    fclose(fs);
    return len;
}

// Line structure:
// 00 00 00 03 ** ** ** ** 07 .N 00 ** 07 .S 00

void filt(const char* buff, const int len, FILE* fp)
{
    for (int i = 0, cnt = 0; i < len; ++i)
    {
        if (buff[i] == 0)
        {
            ++cnt;
        }
        else if (cnt >= 3 && buff[i] == 0x03)
        {
            // test chara name
            i += 5;
            if (i >= len) break;
            if (buff[i] == 0x07)
            {
                // printf("match ");

                fputs(buff + i + 1, fp);
                fputs("\n", fp);
                if (ferror(fp)) throw 1;
                i += strlen(buff + i);

                // test main text
                i += 2;
                if (i >= len) break;
                if (buff[i] == 0x07)
                {
                    fputs(buff + i + 1, fp);
                    fputs("\n", fp);
                    if (ferror(fp)) throw 1;
                    i += strlen(buff + i);
                    cnt = 0;
                }
                else
                {
                    // do not throw
                    cnt = 0;
                }
            }
            else
            {
                cnt = 0;
            }
        }
        else
        {
            cnt = 0;
        }
    }
    fflush(fp);
}

int wmain(int argc, wchar_t* argv[])
{
    try {
        if (argc != 3)
        {
            printf("exe i.asb o.txt\n");
            return 0;
        }

        char* buff;
        int len = openfs(argv[1], buff);

        FILE* fp = _wfopen(argv[2], L"w+");
        if (fp == 0) throw 1;
        filt(buff, len, fp);
        fclose(fp);

        delete[] buff;
    }
    catch (...)
    {
        printf("error\n");
        return 1;
    }
    return 0;
}

