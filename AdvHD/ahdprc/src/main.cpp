
// main.cpp 主程序

#include "usr.h"

using namespace std;

/**
  * n  ror opr-num
  * o  output
  * i  input (specified)
  * t  text-out
  * b  bin-out
  * h  help
  * u  un-pack
  * r  re-pack
  * c  for chinese translation
*/
static const char* argstr = "l:n:o:i:t:b:hurc?";
g_opt_t g_opt; // 全局参数存储

int main(int argc, char** argv)
{
    if(argc < 2) {
        help(mdmain);
    }

    g_opt.clear();
    int opt;
    while(~(opt = getopt(argc - 1, argv + 1, argstr))) { // 第一个参数作为模式选择, 绕过
        switch(opt) {
        case 'l':
            g_opt.list = optarg;
            break;
        case 'n':
            g_opt.opr = optarg;
            break;
        case 'o':
            g_opt.output = optarg;
            break;
        case 'i':
            g_opt.input = optarg;
            break;
        case 't':
            g_opt.text = optarg;
            break;
        case 'b':
            g_opt.binf = optarg;
            break;
        case 'u':
            g_opt.mode |= argunpk;
            break;
        case 'r':
            g_opt.mode |= argrepk;
            break;
        case 'c':
            g_opt.mode |= argchtr;
            break;
        case 'h':
        case '?':
            g_opt.mode |= arghelp;
            break;
        }
    }
    g_opt.argc = (argc - 1) - optind; // 用于批量处理文件
    g_opt.argv = (argv + 1) + optind;

    int tmp = g_opt.mode & arghelp;
    char* mode = argv[1];
    if(!strcmp(mode, "arc")) {
        if(tmp) help(mdpack);
        package();
    } else if(!strcmp(mode, "pna")) {
        if(tmp) help(mdpnap);
        pnapack();
    } else if(!strcmp(mode, "ws2")) {
        if(tmp) help(mdwpsc);
        wpscrpt();
    } else if(!strcmp(mode, "ror")) {
        if(tmp) help(mdrorf);
        rorfile();
    } else if(!strcmp(mode, "rep")) {
        if(tmp) help(mdrepl);
        repl();
    } else {
        help(mdmain);
    }
    return 0;
}

void help(const int m)
{
    switch(m) {
    case mdmain:
        cerr << "usage: ahdprc MODE arguments...\n"
             << "    mode:\n"
             << "        arc\n"
             << "        pna\n"
             << "        ror\n"
             << "        ws2\n"
             << "        rep\n"
             << "  ahdprc MODE -h  for details\n"
             ;
        break;
    case mdpack:
        cerr << "ahdprc arc <-u|-r> [-c] FILE1 [FILE2] ...\n"
             << "  files unpacked will be put in a dir having the same name\n"
             ;
        break;
    case mdpnap:
        cerr << "ahdprc pna <-u|-r> FILE1 [FILE2] ...\n"
             << "  files unpacked will be put in a dir having the same name\n"
             ;
        break;
    case mdrorf:
        cerr << "ahdprc ror <-n NUM> FILE1 [FILE2] ...\n"
             << "ahdprc ror <-n NUM> INFILE -o OUTFILE\n"
             << "  NUM is operator number\n"
             ;
        break;
    case mdwpsc:
        cerr << "(pick) ahdprc ws2 -i IN  -t TXTO -b BINO\n"
             << "(filt) ahdprc ws2 -o OUT IN\n"
             << "(link) ahdprc ws2 -o OUT -t TXTI -b BINI\n"
             ;
        break;
    case mdrepl:
        cerr << "ahdprc rep <-l LIST> FILE1 [FILE2 ...]\n"
             << "  ListFmt : Sour;Targ\n"
             << "  Encoding: UTF-8 only\n"
             ;
        break;
    }
    exit(1);
}
