
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 3)
    {
        printf("yks <in.yks> <out.yks>\n    all text part do XOR 0xAA.");
        return 1;
    }

    FILE* fp = _wfopen(argv[1], L"rb");
    if (fp == 0)
    {
        wprintf(L"%s fail\n", argv[1]);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = (char*)malloc(len);
    fread(buf, 1, len, fp);
    fclose(fp);
    fp = 0;


    int offset = *((int*)(buf + 0x20));
    int length = *((int*)(buf + 0x24));

    for (int i = 0; i < length; ++i)
    {
        *(buf + offset + i) ^= 0xAA;
    }

    fp = _wfopen(argv[2], L"wb");
    if(fp ==0)
    {
        wprintf(L"%s fail\n", argv[2]);
        exit(1);
    }
    fwrite(buf, 1, len, fp);
    fclose(fp);
    free(buf);

    return 0;
}
