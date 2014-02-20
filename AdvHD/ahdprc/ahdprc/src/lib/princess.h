#ifndef princess_H_INCLUDED
#define princess_H_INCLUDED

#include "precompile.h"

namespace princess {
// _stroprt.cpp
void cpfnamonly(std::string&, const std::string&);
void cpfnamonly(char*, const char*);
void cpfnamext(std::string&, const std::string&);
void cpfnamext(char*, const char*);
void cpfnamdir(std::string&, const std::string&);
void cpfnamdir(char*, const char*);
std::string cpdiradd(const char*, const char*);

// _memoprt.cpp
void rormem(uint8_t*, const size_t, const int);
void xormem(uint8_t*, const size_t, const int);
void xorset(uint8_t*, const size_t, const uint8_t*, const size_t);

// _cpconv.cpp
void cpaw(wchar_t*&, int*, const char*, const int, uintptr_t);
void cpaw(std::wstring&, const std::string&, uintptr_t);
void cpwa(char*&, int*, uintptr_t, const wchar_t*, const int, const char*, int*);
void cpwa(std::string&, uintptr_t, const std::wstring&, const char*, int*);
void cpaa(char*&, int*, uintptr_t, const char*, const int, uintptr_t, const char*, int*);
void cpaa(std::string&, uintptr_t, const std::string&, uintptr_t, const char*, int*);

// _fileoprt.cpp
bool rfile(const char*, char*&, long* plen = NULL, const long readsiz = -1);
bool wfile(const char*, char*&, const long, const bool isfree = false, const std::ios::openmode mode = std::ios::binary | std::ios::out | std::ios::trunc, const long prew = 0);
bool wfile(const char*, const char*, const long, const std::ios::openmode mode = std::ios::binary | std::ios::out | std::ios::trunc, const long prew = 0);

// _filelist.cpp
class filelist {
public:
    typedef std::vector<std::string>::size_type size_type;

    void list_dir(const char*);
    void list_lstf(const char*);
    inline size_type count() const;
    inline std::string operator [](const size_type) const;
    inline void clear();
    void arrange();
    inline bool good() const;
private:
    bool _good;
    std::vector < std::string > _chain;
};

// _ut_text.cpp
class text {
public:
    inline void setorder(const int);
    inline void setexist(const bool);
    inline void setch(const char*);
    inline void setch(const std::string&);
    inline void setmt(const char*);
    inline void setmt(const std::string&);
    inline void setcp(const uintptr_t);
    void make();
    inline std::string getpr() const;
    inline const char* getpr_c() const;
    inline void print(std::ostream&);
    inline text& operator ++ ();
    inline text operator ++ (int);
private:
    int _order;
    bool _ch_ex;
    std::string _ch_nam;
    std::string _m_text;
    std::string _prdct;
    bool _ismaked;
    uintptr_t _cp;
};

}
// inline functions
#include "princess.inl"

#endif // princess_H_INCLUDED
