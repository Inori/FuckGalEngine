
#ifndef PRINCESS_INLINE
#define PRINCESS_INLINE

namespace princess {
// _filelist.cpp
inline filelist::size_type filelist::count() const
{
    return _chain.size();
}

inline std::string filelist::operator [](const filelist::size_type s) const
{
    return _chain[s];
}

inline void filelist::clear()
{
    _good = false;
    _chain.clear();
}

inline bool filelist::good() const
{
    return _good;
}

// _ut_text.cpp
inline void text::setorder(const int s)
{
    _ismaked = false;
    _ch_ex = false;
    _order = s;
}

inline void text::setexist(const bool s)
{
    _ismaked = false;
    _ch_ex = s;
}

inline void text::setch(const char* s)
{
    _ismaked = false;
    _ch_nam = s;
    _ch_ex = true;
}

inline void text::setch(const std::string& s)
{
    _ismaked = false;
    _ch_nam = s;
    _ch_ex = true;
}

inline void text::setmt(const char* s)
{
    _ismaked = false;
    _m_text = s;
}

inline void text::setmt(const std::string& s)
{
    _ismaked = false;
    _m_text = s;
}

inline void text::setcp(const uintptr_t s)
{
    _ismaked = false;
    _cp = s;
}

inline std::string text::getpr() const
{
    return _prdct;
}

inline const char* text::getpr_c() const
{
    return _prdct.c_str();
}

inline void text::print(std::ostream& out)
{
    make();
    out << _prdct;
}

inline text& text::operator++()
{
    _ismaked = false;
    _ch_ex = false;
    ++_order;
    return *this;
}

inline text text::operator++(int)
{
    text ret(*this);
    ++*this;
    return ret;
}

}
#endif // PRINCESS_INLINE
