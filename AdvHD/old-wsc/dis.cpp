
#include "pch.h"
#include "usr.h"

int dissociate_text(std::string& os, const char* in, const int siz)
{
    const char* const max = in + siz;

    const char* iter = in;
    const char* last = iter;

    int lengthC = 0; // 前叙代码块, 不包含人名起始的 0x0F
    int lengthT = 0; // 完整文本块, 不包含 %K...0x00 结尾, 不包含 0x00 0x0F 开头

    int order = 0;

    char textA[2048]; // 人名
    char textB[2048]; // 句子
    char textC[8]; // 模式
    char textM[16384]; // 文本块

    while(1)
    {
        if(iter >= max) break;

        if(*iter == 0)
        {
            ++iter;
            if(iter >= max) break;

            if(*iter == 0xf)
            {
                ++iter;
                if(iter >= max) break;

                ++order; // 添加计数
                lengthC = iter - last;
                last = iter; // 计算前方长度

                // 开始分离文本
                textA[0] = textB[0] = textC[0] = textM[0] = 0; // 清空缓冲区，防止人名重复出现

                if(*iter == 0xf)
                {
                    sprintf(textC, "double");
                    ++iter;
                    for(int i = 0; iter < max;)
                    {
                        if((textA[i++] = *iter++) == 0) break;
                    }

                    for(int i = 0; iter < max;)
                    {
                        if((textB[i++] = *iter++) == 0) break;
                    }
                }
                else
                {
                    sprintf(textC, "single");
                    for(int i = 0; iter < max;)
                    {
                        if((textB[i++] = *iter++) == 0) break;
                    }
                }

                // textB 应有文本, 此时 iter 指向字符串结尾的 0x00 后面一位
                --iter;

                for(unsigned int i = 0; i < strlen(textB); ++i) // '%' != 0, 另外长度不会达到 2048
                {
                    if(textB[i] == '%' && textB[i+1] == 'K')
                    {
                        iter -= (strlen(textB) - i);
                        textB[i] = 0;
                        break;
                    }
                }

                lengthT = iter - last;
                last = iter; // 计算文本块长度

                // 输出相关信息
                sprintf(textM, "\r\n<%04d, %04X, %04X, %s>\r\n%s\r\n<!%s>\r\n%s\r\n", order, lengthC, lengthT, textC, textA, textB, textB);
                os += textM;
            }
        }
        else
        {
            ++iter;
        }
    }
    return order;
}

void dis(char* ipf, char* opf)
{
    FILE* ipfs = 0;
    FILE* opfs = 0;

    char* ipbuf = 0;

    do
    {
        ipfs = fopen(ipf, "rb");
        if(ipfs == 0) break;

        fseek(ipfs, 0, SEEK_END);
        int fsiz = ftell(ipfs);

        fseek(ipfs, 0, SEEK_SET);

        ipbuf = new(std::nothrow) char[fsiz];
        if(ipbuf == 0) break;

        fread(ipbuf, sizeof(char), fsiz, ipfs);

        std::string out("");
        out += marker;
        out += "\r\n";

        int order = dissociate_text(out, ipbuf, fsiz);
        char tmp[100];
        sprintf(tmp, "\r\n<@%04d, EOF>\r\n", order);
        out += tmp;

        printf("%s, %d\n", opf, order);

        opfs = fopen(opf, "wb+");
        if(opfs == 0) break;
        fputs(out.c_str(), opfs);
    }
    while (0);

    if(ipfs) fclose(ipfs);
    if(opfs) fclose(opfs);

    if(ipbuf) delete [] ipbuf;
}
