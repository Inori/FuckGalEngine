#ifndef UNICODESTRING_H
#define UNICODESTRING_H

#include <fstream>
#include <deque>
#include <stdint.h>

class UnicodeString {
public:
    const char *str;
    uint32_t length;

    UnicodeString(): str(0), length(0) {}
    UnicodeString(const char *s, uint32_t l);
    UnicodeString(const UnicodeString &orig);
    ~UnicodeString();
    UnicodeString &operator =(const UnicodeString &rhs);

    // width of text - ascii has width of 1, other characters of 2
    size_t width() const;

    // split UnicodeString with supplied separator wchar, separator is dropped
    std::deque<UnicodeString> split(const char *sep, size_t sepLen) const;
    // split UnicodeString into parts of maximum of len size
    std::deque<UnicodeString> split(size_t len) const;

    UnicodeString &operator +=(const UnicodeString &rhs);
    bool operator ==(const UnicodeString &rhs) const;
    int byteSize() const;
    void writeToData(char *data) const;
};

UnicodeString parseUnicodeString(const char *source);
UnicodeString parseUnicodeString(const char *source, const char *sourceEnd);

//计算message在内存中地址
char* getSourceStrOffset(const char *source, const char *sourceEnd);


bool isShortUnicodeString(const char *source, const char *sourceEnd);

// search for unicode string backwards
// it must be at most 255 wchars long, not contain any 0x0000 (wchar nulls) and
// end exactly at str - 1
const char *searchBackwardsForShortUnicodeString(const char *content, size_t startOffset);

void unicodeStringToUtf8(char **utf8, size_t *utf8Len, const UnicodeString &utf16le);
UnicodeString utf8ToUnicodeString(const char *utf8, size_t utf8Len);

void printUtf8(std::ofstream &fout, const UnicodeString &us);


#endif // UNICODESTRING_H
