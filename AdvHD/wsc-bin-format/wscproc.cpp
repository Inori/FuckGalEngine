
#include "pch.h"
#include "usr.h"
#include "wscproc.h"

wscele wscproc::get(const int i) const
{
    return chain[i];
}

wscele wscproc::operator [] (const int i) const
{
    return chain[i];
}

int wscproc::length() const
{
    return chain.size();
}

void wscproc::push(char* s)
{
    if(strlen(s) == 0 || s[0] == 0xd || s[0] == 0xa)
    {
        return;
    }

    if(s[0] == '<' && strlen(s) >= 3)
    {
        if(s[1] == '!') { return; }

        if(fillflag == 1)
        {
            chain.push_back(tmp);
            fillflag = 0;
        }
        if(zeroflag == 1 || unfinished != 0)
        {
            chain.push_back(tmp);
            fprintf(stderr, "!warning<2>:   lacking text: %04d\n", tmp.order);
        }


        if(s[1] == '@')
        {
            int total = 0;
            sscanf(s, "<@%d", &total);
            if(total != (int)chain.size())
            {
                fputs("! error <0>: lacking lines\n", stderr);
            }
        }

        else
        {
            zeroflag = 1;
            char m[20];
            sscanf(s, "<%d, %x, %x, %s", &(tmp.order), &(tmp.befoff), &(tmp.aftoff), m);
            if(memcmp(m, "single", 6) == 0) tmp.mode = 1;
            if(memcmp(m, "double", 6) == 0) tmp.mode = 2;

            tmp.strA = "";
            tmp.strB = "";
            //printf("%d %x %x %d\n", tmp.order, tmp.befoff, tmp.aftoff, tmp.mode);
        }
    }
    else
    {
        zeroflag = 0;
        if(tmp.mode == 1 && genflag == 0)
        {
            tmp.strB = s;
            ++genflag;
        }
        else if(tmp.mode == 2 && genflag == 0)
        {
            tmp.strA = s;
            unfinished = 1;
            ++genflag;
        }
        else if(tmp.mode == 2 && genflag == 1)
        {
            tmp.strB = s;
            ++genflag;
        }
        else
        {
            fprintf(stderr, "! error <1>: redundant text: %04d\n", tmp.order);
            return;
        }

        if(tmp.mode == genflag && tmp.mode != 0)
        {
            fillflag = 1;
            unfinished = genflag = 0;
        }
    }
}
