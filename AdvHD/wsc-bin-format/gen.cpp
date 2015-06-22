
#include "pch.h"
#include "usr.h"
#include "wscproc.h"

void gen_after_id(char* src, char* txt, char* out)
{
    FILE* srcfs = 0;
    FILE* txtfs = 0;
    FILE* outfs = 0;

    char* srcbuf = 0;
    wscproc txtbuf;
    char* outbuf = 0;

    do
    {
        srcfs = fopen(src, "rb");
        txtfs = fopen(txt, "r");

        if(srcfs == 0 || txtfs == 0) break;

        fseek(srcfs, 0, SEEK_END); // 读取 src 文件
        int srcsiz = ftell(srcfs);
        srcbuf = new(std::nothrow) char[srcsiz];
        if(srcbuf == 0) break;

        fseek(srcfs, 0, SEEK_SET);
        fread(srcbuf, sizeof(char), srcsiz, srcfs);

        fseek(txtfs, 0, SEEK_SET); // 读取 txt 文件
        char tmp[2048];

        while(1)
        {
            fgets(tmp, 2047, txtfs);
            if(feof(txtfs)) break;

            tmp[strlen(tmp)-1] = 0;

            txtbuf.push(tmp); // 解析行
        }

        outbuf = new(std::nothrow) char[srcsiz * 2];
        if(outbuf == 0) break;

        int block_count = txtbuf.length(); // 开始导入

        char* outiter = outbuf;
        const char* srciter = srcbuf;

        for(int i = 0; i < block_count; ++i)
        {
            int colen = txtbuf[i].befoff; // 代码长度
            memcpy(outiter, srciter, colen);
            outiter += colen;
            srciter += colen;

            ////////////////////////////////
            char gentmp[2048];
            int tmplen = 0;


            if(txtbuf[i].mode == 1) // single
            {
                tmplen = txtbuf[i].strB.length();
                memcpy(gentmp, txtbuf[i].strB.c_str(), tmplen);
            }
            else if(txtbuf[i].mode == 2) // double
            {
                gentmp[0] = 0xf;
                int Asiz = txtbuf[i].strA.length();
                int Bsiz = txtbuf[i].strB.length();
                memcpy(gentmp + 1, txtbuf[i].strA.c_str(), Asiz);
                gentmp[1 + Asiz] = 0;
                memcpy(gentmp + 1 + Asiz + 1, txtbuf[i].strB.c_str(), Bsiz);
                tmplen = 1 + Asiz + 1 + Bsiz;
            }


            //////////////////////////////////
            memcpy(outiter, gentmp, tmplen); // 双方文本长度
            outiter += tmplen;
            srciter += txtbuf[i].aftoff;
        }
        int lastlen = srcsiz - (srciter - srcbuf);
        memcpy(outiter, srciter, lastlen);
        outiter += lastlen;
        int outlen = outiter - outbuf;

        outfs = fopen(out, "wb+");
        if(outfs == 0) break;
        fwrite(outbuf, sizeof(char), outlen, outfs);
    }
    while(0);


    if(srcfs) fclose(srcfs);
    if(txtfs) fclose(txtfs);
    if(outfs) fclose(outfs);

    if(srcbuf) delete [] srcbuf;
    if(outbuf) delete [] outbuf;
}


void gen(char* ipfa, char* ipfb, char* opf)
{
//  首先识别两个输入文件的类型
    FILE* iafs = 0;
    FILE* ibfs = 0;

//  extern const char* marker;
    char test[100];

    int typeflag = 0; // 必须有一个文本和一个源文件
    do
    {
        iafs = fopen(ipfa, "rb");
        ibfs = fopen(ipfb, "rb");
        if(iafs == 0 || ibfs == 0) break;

        fseek(iafs, 0, SEEK_END);
        int asiz = ftell(iafs);
        fseek(ibfs, 0, SEEK_END);
        int bsiz = ftell(ibfs);

        // 开始识别
        fseek(iafs, 0, SEEK_SET);
        fseek(ibfs, 0, SEEK_SET);


        if(asiz >= (int)strlen(marker)) // 测试文件 A 是不是文本
        {
            fread(test, sizeof(char), strlen(marker), iafs);
            if(memcmp(test, marker, strlen(marker)) == 0)
                typeflag += 2;
        }
        if(bsiz >= (int)strlen(marker)) // 测试文件 B 是不是文本
        {
            fread(test, sizeof(char), strlen(marker), ibfs);
            if(memcmp(test, marker, strlen(marker)) == 0)
                typeflag += 4;
        }
    }
    while(0);

    if(iafs) fclose(iafs);
    if(ibfs) fclose(ibfs);

    if(typeflag == 2) gen_after_id(ipfb, ipfa, opf); // A 是文本
    if(typeflag == 4) gen_after_id(ipfa, ipfb, opf); // B 是文本
}
