#ifndef WSCPROC_H_INCLUDED
#define WSCPROC_H_INCLUDED

struct wscele
{
    int order;
    int befoff;

    std::string strA;
    std::string strB;
    int mode;

    int aftoff;
};

class wscproc
{
    wscele tmp;
    std::vector<wscele> chain;
    int genflag; // wscele.mode
    int fillflag;
    int zeroflag; // block 是否为空
    int unfinished;
public:
    wscproc()
    {
        genflag = 0;
        fillflag = 0;
        zeroflag = 0;
        unfinished = 0;
        tmp.order = 0;
        tmp.befoff = 0;
        tmp.mode = 0;
        tmp.aftoff = 0;
    }
    void push(char*);
    wscele get(const int) const;
    int length() const;
    wscele operator [] (const int) const;
};

#endif // WSCPROC_H_INCLUDED
