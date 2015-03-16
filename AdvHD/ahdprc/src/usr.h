
// usr.h 主要头文件

#ifndef USR_H_INCLUDED
#define USR_H_INCLUDED

#include "pre.h"

enum argmode {
    arginit = 0, // 操作模式
    arghelp = 1,
    argunpk = 2,
    argrepk = 4,
    argchtr = 8,

    mdmain = 10, // 帮助信息模式
    mdpack = 11,
    mdpnap = 12,
    mdrorf = 13,
    mdwpsc = 14,
    mdrepl = 15,
};

struct g_opt_t { // 参数存储器
    char* list;
    char* input;
    char* output;
    char* text;
    char* binf;
    char* opr;
    int mode;
    int argc;
    char** argv;

    inline void clear();
};

//main.cpp
extern g_opt_t g_opt;
extern void help(const int);

//functions
extern void package(); // arc 包
extern void pnapack(); // pna 包
extern void rorfile(); // ror 功能
extern void wpscrpt(); // 脚本功能
extern void repl();    // 替换功能

///////////////////////////////////////////////////////////////////

inline void g_opt_t::clear()
{
    list = NULL;
    input = NULL;
    output = NULL;
    text = NULL;
    binf = NULL;
    opr = NULL;
    mode = arginit;
    argc = 0;
    argv = NULL;
}

#endif // USR_H_INCLUDED
