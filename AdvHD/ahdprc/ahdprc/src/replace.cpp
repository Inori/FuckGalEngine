
#include "usr.h"

using namespace std;

struct Rlst_t {
    typedef pair < string, string > Rpele_t; // 替换对
    typedef vector < Rpele_t > Rpair_t;
    typedef Rpair_t::size_type Rpsiz_t;
    typedef Rpair_t::iterator  Riter_t;
    bool good() {return avalible;}
    void build(const char*);
    bool proc(const char*);
private:
    bool avalible;
    Rpair_t rpair;
};

// 转换列表文件
void Rlst_t::build(const char* fn)
{
    ifstream lstf;
    lstf.open(fn);
    if(!lstf) {
        cerr << "*** E: no list file: " << fn << "\n";
        avalible = false;
        return;
    }
    stringstream bone;
    string bonestr;
    getline(lstf, bonestr);
    if(lstf.eof()) {
        cerr << "*** E: empty list\n";
        avalible = false;
        return;
    }
    string tmp1, tmp2;
    while(1) {
        getline(lstf, bonestr);
        if(lstf.eof()) break;
        bone << bonestr;
        getline(bone, tmp1, ';');
        getline(bone, tmp2, ';');
        Rpele_t rpele(tmp1, tmp2);
        rpair.push_back(rpele);
        bone.clear();
    }
    avalible = true;
}

// 查找与替换
bool Rlst_t::proc(const char* fn)
{
    long offset;
    long buflen;

    char* buf;
    if(!princess::rfile(fn, buf, &buflen)) return false;
    vector<char> rstore;
    for(offset = 0; offset < buflen; /* empty */) {
        bool replaced = false;
        for(Rpsiz_t i = 0; i != rpair.size(); ++i) {
            long slen = (long)rpair[i].first.size();
            const char* sp = rpair[i].first.c_str();
            long tlen = (long)rpair[i].second.size();
            const char* tp = rpair[i].second.c_str();

            if(offset + slen > buflen) break;
            if(!memcmp(buf + offset, sp, slen)) {
                for(long j = 0; j < tlen; ++j) {
                    rstore.push_back(tp[j]);
                }
                offset += slen;
                replaced = true;
                break;
            }
        }
        if(!replaced) {
            rstore.push_back(buf[offset++]);
        }
    }

    delete [] buf;

    long olen = (long)rstore.size();
    char* obuf = new char[olen];
    for(long i = 0; i < olen; ++i) {
        obuf[i] = rstore[i];
    }
    if(!princess::wfile(fn, obuf, olen, true)) {
        delete [] obuf;
        return false;
    }
    return true;
}

void repl()
{
    if(!g_opt.list) help(mdrepl);

    Rlst_t rlist;
    rlist.build(g_opt.list);
    if(!rlist.good()) {
        exit(1);
    }
    for(int i = 0; i < g_opt.argc; ++i) {
        if(!rlist.proc(g_opt.argv[i])) {
            cerr << "*** E: unable to process " << g_opt.argv[i] << "\n";
        }
    }
}
