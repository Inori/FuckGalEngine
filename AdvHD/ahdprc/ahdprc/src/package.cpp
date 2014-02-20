
#include "usr.h"

using namespace std;

static void unpack(const char*);
static void repack(const char*);

void package()
{
    int mode = g_opt.mode & (argunpk | argrepk);

    // 判断模式, 处理所有参数中的文件
    if(mode == argunpk) {
        for(int i = 0; i < g_opt.argc; ++i) {
            unpack(g_opt.argv[i]);
        }
    } else if(mode == argrepk) {
        for(int i = 0; i < g_opt.argc; ++i) {
            repack(g_opt.argv[i]);
        }
    } else {
        help(mdpack);
    }
}

struct ARCHDR {
    uint32_t entry_count;
    uint32_t toc_length;
};

struct ARCENTRY {
    uint32_t length;
    uint32_t offset;
    // wchar_t filename[1];
};

void unpack(const char* fn)
{
    char* buf; // 存储整个包裹
    if(!princess::rfile(fn, buf)) {
        cerr << "*** E: unable to process " << fn << "\n";
        return;
    }
    string cmd("mkdir "); // 在文件所在地建立目录
    string fnstr(fn);
    princess::cpfnamdir(fnstr, fnstr);
    cmd += fnstr;
    cmd += " 1>NUL 2>&1";
    ::system(cmd.c_str());

    ARCHDR* phdr = (ARCHDR*) buf;
    char* p = buf + sizeof(ARCHDR);
    char* pdata = p + phdr->toc_length;

    for(uint32_t i = 0; i < phdr->entry_count; ++i) {
        ARCENTRY* entry = (ARCENTRY*) p;
        p += sizeof(ARCENTRY);

        wchar_t* wfnam = (wchar_t*) p; // 转码文件名并连接
        string afnstr;
        wstring wfnstr(wfnam);
        const char dft = '_';
        if(g_opt.mode & argchtr) {
            princess::cpwa(afnstr, 932, wfnstr, &dft, NULL);
        } else {
            princess::cpwa(afnstr, 0, wfnstr, &dft, NULL);
        }
        string ofn(fnstr);
        ofn += "\\";
        ofn += afnstr;

        p += (wfnstr.size() + 1) * sizeof(wchar_t);

        if(!princess::wfile(ofn.c_str(), pdata + entry->offset, entry->length)) {
            cerr << "*** W: unable to process " << ofn << "\n";
        }
    }
    delete [] buf;
}

/// 用于创建文件头
struct PACKENTRY {
    uint32_t length;
    uint32_t offset;
    wchar_t* pwfn;
};

struct ARCTOC {
    void write(const char*) const;
    ARCTOC& operator += (const string&);
    inline ARCTOC();
private:
    PACKENTRY tmp; // 文件的数据在前方, 需要添零
    vector<PACKENTRY> lst; // 记录每个子文件的信息
    uint32_t toc_len;
};

inline ARCTOC::ARCTOC()
{
    tmp.length = 0;
    tmp.offset = 0;
    tmp.pwfn = NULL;
    toc_len = 0;
}

// 文件头建立主要通过此函数
ARCTOC& ARCTOC::operator+=(const string& fn)
{
    ifstream inf;
    inf.open(fn.c_str(), ios::in | ios::binary);
    if(!inf) {
        cerr << "*** E: unable to process " << fn << "\n";
        return *this;
    }
    inf.seekg(0, ios::end);
    tmp.offset += tmp.length;
    tmp.length = (uint32_t)inf.tellg();
    inf.close();

    string sngl_fn;
    princess::cpfnamext(sngl_fn, fn);

    if(g_opt.mode & argchtr) { // 依然是按需转码
        princess::cpaw(tmp.pwfn, NULL, sngl_fn.c_str(), sngl_fn.size(), 936);
    } else {
        princess::cpaw(tmp.pwfn, NULL, sngl_fn.c_str(), sngl_fn.size(), 0);
    }

    lst.push_back(tmp);

    toc_len += sizeof(PACKENTRY) - sizeof(wchar_t*);
    toc_len += (::wcslen(tmp.pwfn) + 1) * sizeof(wchar_t);

    return *this;
}

// 写入文件
void ARCTOC::write(const char* fn) const
{
    ofstream ofile;
    ofile.open(fn, ios::out | ios::trunc | ios::binary);
    if(!ofile) {
        cerr << "*** E: unable to process " << fn << "\n";
        ::exit(1);
    }

    char* p = NULL;
    uint32_t entry_count = lst.size();
    p = (char*) &entry_count;
    ofile.write(p, sizeof(uint32_t));
    p = (char*) &toc_len;
    ofile.write(p, sizeof(uint32_t));
    for(uint32_t i = 0; i < entry_count; ++i) {
        p = (char*) &lst[i];
        ofile.write(p, (sizeof(PACKENTRY) - sizeof(wchar_t*)));
        p = (char*) lst[i].pwfn;
        ofile.write(p, (::wcslen(lst[i].pwfn) + 1) * sizeof(wchar_t));
        delete [] lst[i].pwfn;
    }
    ofile.close();
}

void repack(const char* dn)
{
    string ofn(dn);
    ofn += ".arc";

    princess::filelist dlst;
    dlst.list_dir(dn);

    uint32_t entry_count = (uint32_t)dlst.count();
    ARCTOC arctoc;
    for(uint32_t i = 0; i < entry_count; ++i) {
        arctoc += dlst[i];
    }
    arctoc.write(ofn.c_str());

    for(uint32_t i = 0; i < entry_count; ++i) { // 合成文件
        char* tmp;
        long tmplen;
        if(!princess::rfile(dlst[i].c_str(), tmp, &tmplen)) {
            cerr << "*** E: unable to process " << dlst[i] << "\n";
            ::exit(1);
        }
        if(!princess::wfile(ofn.c_str(), tmp, tmplen, true, ios::app | ios::out | ios::binary)) {
            cerr << "*** E: unable to combine " << ofn << "\n";
            delete [] tmp;
            ::exit(1);
        }
    }
}
