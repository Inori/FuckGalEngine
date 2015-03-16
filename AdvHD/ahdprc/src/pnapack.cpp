
#include "usr.h"
#define dbgp(x); cerr << x << "\n";

using namespace std;

static void unpna(const char*);
static void repna(const char*);

void pnapack()
{
    int mode = g_opt.mode & (argunpk | argrepk);

    // 判断模式, 处理所有参数中的文件
    if(mode == argunpk) {
        for(int i = 0; i < g_opt.argc; ++i) {
            unpna(g_opt.argv[i]);
        }
    } else if(mode == argrepk) {
        for(int i = 0; i < g_opt.argc; ++i) {
            repna(g_opt.argv[i]);
        }
    } else {
        help(mdpnap);
    }
}

struct PNAPHDR {
    char signature[4]; // "PNAP"
    long unknown1;
    long width;
    long height;
    long entry_count;
};

struct PNAPENTRY {
    long unknown1;
    long index;
    long offset_x;
    long offset_y;
    long width;
    long height;
    long unknown2;
    long unknown3;
    long unknown4;
    long length;
};

void unpna(const char* fn)
{
    dbgp("start");
    char* buf; // 存储整个包裹
    if(!princess::rfile(fn, buf)) {
        cerr << "*** E: unable to process " << fn << "\n";
        return;
    }
    dbgp("fn: " << fn << "cp done");

    string cmd("mkdir "); // 在文件所在地建立目录
    string fnstr(fn);

    dbgp("str: " << fnstr);
    princess::cpfnamdir(fnstr, fnstr);
    dbgp("str: " << fnstr);
    cmd += fnstr;
    cmd += " 1>NUL 2>&1";
    ::system(cmd.c_str());

    dbgp("dir done");
    PNAPHDR* hdr = (PNAPHDR*) buf;
    PNAPENTRY* entries = (PNAPENTRY*) (hdr + 1);
    char* p = (char*)(entries + hdr->entry_count);

    string chtnam = fnstr + "\\_cht.bin"; // 文件头无法破解, 保存一份用于封包
    if(!princess::wfile(chtnam.c_str(), buf, p - buf)) {
        cerr << "*** E: unable to create cheat record\n";
        exit(1);
    }

    for(int i = 0; i < hdr->entry_count; ++i) {
        if(!entries[i].length) {
            continue;
        }
        ostringstream ofnss;
        ofnss << fnstr << "\\" << setfill('0') << setw(3) << i << ".png";

        if(!princess::wfile(ofnss.str().c_str(), p, entries[i].length)) {
            cerr << "*** W: unable to write " << ofnss.str() << "\n";
        }
        p += entries[i].length;
    }
    delete [] buf;
}

void repna(const char* fn)
{
    string chtnam(fn);
    chtnam += "\\_cht.bin";
    char* hdrbuf;
    long hdrlen;
    if(!princess::rfile(chtnam.c_str(), hdrbuf, &hdrlen)) {
        cerr << "*** E: unable to open cheat record\n";
        exit(1);
    }

    PNAPHDR* hdr = (PNAPHDR*) hdrbuf;
    PNAPENTRY* entries = (PNAPENTRY*) (hdr + 1);

    princess::filelist flst;
    flst.list_dir(fn); // _cht.bin 在最后面

    string ofn(fn);
    ofn += ".pna";
    if(!princess::wfile(ofn.c_str(), hdrbuf, hdrlen)) { // 长度相同, 所以先用老信息占位
        cerr << "*** E: unable to combine cheat record\n";
        exit(1);
    }
    for(int i = 0, j = 0; i < hdr->entry_count; ++i) {
        if(!entries[i].length) {
            continue;
        }
        char* buf;
        long buflen; // 我们只需要修改文件头的长度信息, 别的事情先不管了
        if(!princess::rfile(flst[j].c_str(), buf, &buflen)) {
            cerr << "*** E: unable to process " << flst[j] << "\n";
            exit(1);
        }
        if(!princess::wfile(ofn.c_str(), buf, buflen, true, ios::app | ios::out | ios::binary)) {
            cerr << "** E: unable to compine " << flst[j] << "\n";
            delete [] buf;
            exit(1);
        }
        ++j;
    }
    // 使用新信息覆盖原来的文件头
    if(!princess::wfile(ofn.c_str(), hdrbuf, hdrlen, true, ios::binary | ios::in | ios::out)) {
        cerr << "*** E: unable to refresh cheat record\n";
        delete [] hdrbuf;
        exit(1);
    }
    delete [] hdrbuf;
}
