
#include "usr.h"

using namespace std;

static void _ror(const char*, const char*, const int);

void rorfile()
{
    if(!g_opt.opr) help(mdrorf);
    int opr = strtol(g_opt.opr, NULL, 0);
    if(g_opt.output && g_opt.argv[0]) {
        _ror(g_opt.output, g_opt.argv[0], opr);
    } else {
        for(int i = 0; i < g_opt.argc; ++i) {
            _ror(g_opt.argv[i], g_opt.argv[i], opr);
        }
    }
}

void _ror(const char* dest, const char* sour, const int opr)
{
    char* buf;
    long buflen;
    if(!princess::rfile(sour, buf, &buflen)) {
        cerr << "*** E: cannot read " << sour << "\n";
        exit(1);
    }
    princess::rormem((uint8_t*)buf, buflen, opr);
    if(!princess::wfile(dest, buf, buflen, true)) {
        cerr << "*** E: cannot write " << dest << "\n";
        delete [] buf;
        exit(1);
    }
}
