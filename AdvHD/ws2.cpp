
// Warning: Source file CodePage: 932 (Shift_JIS)
// CP932 Mark: Ç≤î—ÅEî¸ñ°ÇµÇ¢Ç≈Ç∑ÅÙ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROC 2
//#define CCR

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3)
    {
#if PROC == 1
        printf("<me> <infile> <outfile>\n");
#elif PROC == 2
        printf("<me> <infile> <textfile> <addrfile>\n");
#endif
        return 0;
    }

    FILE* fi = _wfopen(argv[1], L"rb");
    if (fi == 0) exit(1);
    fseek(fi, 0, SEEK_END);
    int len = ftell(fi);
    fseek(fi, 0, SEEK_SET);

    char* buf = new char[len];
    fread(buf, 1, len, fi);
    fclose(fi);

    /////////////////////////////////////////

#if PROC == 1 // roll-right 2 bits

    unsigned char* ubuf = (unsigned char*)buf;
    for (int i = 0; i < len; ++i)
        ubuf[i] = (ubuf[i] >> 2) | (ubuf[i] << 6);
    FILE* fo = _wfopen(argv[2], L"wb+");
    if (fo == 0) exit(1);
    fwrite(buf, 1, len, fo);

#elif PROC == 2

    FILE* fo = _wfopen(argv[2], L"w+");
    if (fo == 0) exit(1);
    FILE* fa = _wfopen(argv[3], L"w+");
    if (fa == 0) exit(1);

    int noChar = 1;
    int order = 1;
#ifdef CCR
    int mkCCR = 0;
#endif

    char CharStr[200];
    int CharOff = 0;
    char MainStr[800];
    int MainOff = 0;

    for (char* anc = buf; anc < buf + len; ++anc)
    {
        if (anc - buf > 3 && memcmp(anc - 3, "%LC", 3) == 0 || memcmp(anc - 3, "%LF", 3) == 0)
        {
            strcpy(CharStr, anc);
            CharOff = anc - buf;
            noChar = 0;
        }
#ifdef CCR
        if (anc - buf > 15 && memcmp(anc - 15, "29\0CUTIN_32_001", 15) == 0)
        {
            mkCCR = 1;
        }
        if (anc - buf > 13 && memcmp(anc - 13, "7st29\0" "7st30\0" "e", 13) == 0)
        {
            //printf("-");
            mkCCR = 0;
        }
#endif
        if (anc - buf > 5 && memcmp(anc - 5, "char\0", 5) == 0)
        {
            strcpy(MainStr, anc);
            MainOff = anc - buf;
            for (int i = 0; i < strlen(MainStr) - 1; ++i)
            {
                if (MainStr[i] == '%' && MainStr[i + 1] == 'K')
                    MainStr[i] = 0;
            }
            // line A
            fprintf(fo, "Åõ%04dÅõ", order);
            if (!noChar) fprintf(fo, "%sÅõ", CharStr);
            fprintf(fo, " ##\n%s ##\n", MainStr);
            // line B
            fprintf(fo, "Åú%04dÅú", order);
            if (!noChar) fprintf(fo, "%sÅú", CharStr);
#ifdef CCR
            if (mkCCR) fprintf(fo, " < CCR >");
#endif
            fprintf(fo, "\n%s\n", MainStr);
            // blank
            fprintf(fo, "\n");
            // anchor
            if (!noChar) fprintf(fa, "C %04d %08X %04X\n", order, CharOff, strlen(CharStr));
            fprintf(fa, "M %04d %08X %04X\n", order, MainOff, strlen(MainStr));

            noChar = ++order;
        }
    }

    fclose(fa);

#endif

    fclose(fi);
    delete[] buf;
    return 0;
}

