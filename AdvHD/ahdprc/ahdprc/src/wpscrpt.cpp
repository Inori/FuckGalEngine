
#include "usr.h"

using namespace std;

enum md_t {
    wsinit = 0,
    wstext = 1,
    wsbinf = 2,
    wsinpt = 4,
    wsoutp = 8,
};

static void _pick(const char*, const char*, const char*);
static void _filt(const char*, const char*);
static void _link(const char*, const char*, const char*);

void wpscrpt()
{
    int mode = wsinit;
    if(g_opt.input) mode |= wsinpt;
    if(g_opt.output) mode |= wsoutp;
    if(g_opt.text) mode |= wstext;
    if(g_opt.binf) mode |= wsbinf;

    if(mode == (wsoutp)) {
        _filt(g_opt.output, g_opt.argv[0]);
    } else if(mode == (wsinpt | wstext | wsbinf)) {
        _pick(g_opt.text, g_opt.binf, g_opt.input);
    } else if(mode == (wsoutp | wstext | wsbinf)) {
        _link(g_opt.output, g_opt.text, g_opt.binf);
    } else {
        help(mdwpsc);
    }
}

////////////////////////////////////////////////////////
enum txttp {
    character,
    maintext,
    binarydata,
};

// 占位符
static const char* dummy = "\xAA\xAA\xAA\xAA\xAA\xAA";

// 判断信息模式
static int msgtype(const char* p)
{
    if(p[0] == '%' && p[1] == 'L' && p[2] == 'C')
        return character;
    else if(p[0] == 'c' && p[1] == 'h' && p[2] == 'a' && p[3] == 'r' && p[4] == 0)
        return maintext;
    else
        return binarydata;
}

void _pick(const char* otxt, const char* obin, const char* sour)
{
    char* inbuf;
    long inlen;
    if(!princess::rfile(sour, inbuf, &inlen)) {
        cerr << "*** E: unable to open " << sour << "\n";
        exit(1);
    }
    ostringstream txtoss;
    char* bobuf = new char[inlen * 2];

    char* ps = inbuf;
    char* po = bobuf;

    princess::text tsbk;
    tsbk.setcp(932);
    tsbk.setorder(1);
    string tmp;
    while(ps - inbuf < inlen && po - bobuf < inlen * 2) {
        switch(msgtype(ps)) {
        case binarydata:
            *po++ = *ps++;
            break;
        case character:
            strcpy(po, "%LC");
            po += strlen("%LC");
            ps += strlen("%LC");
            strcpy(po, dummy);
            po += strlen(po);

            tmp = ps;
            tsbk.setch(tmp);

            ps += strlen(ps);
            break;
        case maintext:
            strcpy(po, ps);
            po += strlen(po) + 1;
            strcpy(po, dummy);
            po += strlen(po);
            strcpy(po, "%"); // 连 "%K" 都不满足, 遑论 "%K%P". 这是脚本处理时的一个隐藏 bug
            po += strlen(po);

            ps += strlen(ps) + 1;
            char* flag = strchr(ps, '%');
            if(!flag) {
                cerr << "*** P: internal error\n";
                delete [] inbuf;
                delete [] bobuf;
                exit(1);
            }
            *flag = 0;
            tmp = ps;
            tsbk.setmt(tmp);
            ps += strlen(ps) + 1; // 仅仅绕过第一个百分号
            tsbk++.print(txtoss);
            break;
        }
    }
    delete [] inbuf;
    if(!princess::wfile(obin, bobuf, (long)(po - bobuf), true)) {
        cerr << "*** E: unable to write " << obin << "\n";
        delete [] bobuf;
        exit(1);
    }
    ofstream tofs;
    tofs.open(otxt);
    tofs << txtoss.str() << flush;
    tofs.close();
}

//////////////////////////////////////////////////////////
static const string black_c("\xE2\x97\x8F"); // "●" 简单修改一下可以成为白圈
static bool filtproc(const string& str, ostream& out)
{
    if(str.end() == search(str.begin(), str.end(), black_c.begin(), black_c.end())) {
        return false;
    }
    if(str.begin() + black_c.size() == search(str.begin(), str.begin() + black_c.size(), black_c.begin(), black_c.end())) {
        return false;
    }
    if(str.end() == search(str.end() - black_c.size(), str.end(), black_c.begin(), black_c.end())) {
        return false;
    }

    string tmp(str);
    tmp.erase(tmp.begin(), tmp.begin() + black_c.size());
    tmp.erase(tmp.end() - black_c.size(), tmp.end());
    string::iterator flag;
    if(tmp.end() == (flag = search(tmp.begin(), tmp.end(), black_c.begin(), black_c.end()))) {
        return false;
    }
    tmp.erase(tmp.begin(), flag + black_c.size());
    if(tmp.size() == 0) {
        return true;
    }
    out << tmp << endl;
    return true;
}

/// 主要是看那几个圆圈怎么玩了
void _filt(const char* dest, const char* sour)
{
    ifstream infile;
    infile.open(sour);
    if(!infile) {
        cerr << "*** E: unable to process " << sour << "\n";
        ::exit(-1);
    }
    ofstream ofile;
    ofile.open(dest);
    if(!ofile) {
        cerr << "*** E: unable to process " << dest << "\n";
        ::exit(-1);
    }

    string tmp;
    while(true) {
        getline(infile, tmp);
        if(infile.eof()) break;
        if(filtproc(tmp, ofile)) {
            getline(infile, tmp);
            ofile << tmp << endl;
        }
    }
    infile.close();
    ofile.close();
}

//////////////////////////////////////////////////////////
static bool isdummy(const char* p)
{
    if(!memcmp(p, dummy, strlen(dummy)))
        return true;
    else
        return false;
}

/// 此函数产生的日志向 stdout 输出, 建议重定向
void _link(const char* dest, const char* itxt, const char* ibin)
{
    char* ibuf;
    long ilen;
    if(!princess::rfile(ibin, ibuf, &ilen)) {
        cerr << "*** E: unable to open " << ibin << "\n";
        exit(1);
    }
    ifstream txtf;
    txtf.open(itxt, ios::in);
    if(!txtf) {
        std::cerr << "*** E: unable not open " << itxt << "\n";
        exit(1);
    }

    char* obuf = new char[ilen * 2];

    char* ps = ibuf;
    char* po = obuf;

    char dft = '_'; // 转码默认字符
    int order = 0;  // 记录句子行数
    int lost = 0;   // 记录转码时是否丢失字符
    int flag = 0;   // 根据遇到的eof数量判断文件契合度(粗略)

    string tmp;
    while(ps - ibuf < ilen && po - obuf < ilen * 2) {
        if(!isdummy(ps)) {
            *po++ = *ps++;
        } else {
            ++order;
            getline(txtf, tmp);
            if(txtf.eof()) ++flag;
            princess::cpaa(tmp, 936, tmp, 65001, &dft, &lost);
            if(lost) {
                cout << "log: (codepage conversion) line "
                     << setfill(' ') << setw(4) << order
                     << " in file " << itxt << " lost message" << endl;
            }
            strcpy(po, tmp.c_str());
            ps += strlen(dummy);
            po += tmp.size();
        }
    }
    getline(txtf, tmp);
    if(txtf.eof()) ++flag;
    if(flag != 1) {
        cout << "log: file " << itxt << " may be not suitable" << endl;
    }
    txtf.close();
    delete [] ibuf;

    if(!princess::wfile(dest, obuf, (long)(po - obuf), true)) {
        cerr << "*** E: unable to write " << dest << "\n";
        exit(1);
    }
}
