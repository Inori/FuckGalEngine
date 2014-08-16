#include <cstring>
#include "iconv.h"
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include "unicodestring.h"

static const uint32_t shortLength = 64;

UnicodeString::UnicodeString(const char *s, uint32_t l):
    length(l) {
    char *usStr = new char[2 * length];
    memcpy(usStr, s, 2 * length);
    str = usStr;
}

UnicodeString::UnicodeString(const UnicodeString &orig):
    length(orig.length) {
    char *usStr = new char[2 * length];
    memcpy(usStr, orig.str, 2 * length);
    str = usStr;
}

UnicodeString::~UnicodeString() {
    delete[] str;
}

UnicodeString &UnicodeString::operator =(const UnicodeString &rhs) {
    if (this == &rhs) {
        return *this;
    }

    delete[] str;
    length = rhs.length;
    char *usStr = new char[2 * length];
    memcpy(usStr, rhs.str, 2 * length);
    str = usStr;

    return *this;
}

size_t UnicodeString::width() const {
    size_t w = 0;
    for (size_t i = 0; i < length; ++i) {
        if (str[2 * i + 1] == 0) {
            w += 1;
        } else {
            w += 2;
        }
    }

    return w;
}

std::deque<UnicodeString> UnicodeString::split(const char *sep, size_t sepLen) const {
    assert(sepLen > 0);
    std::deque<UnicodeString> result;

    size_t begin = 0;
    for (size_t i = 0; i < length - sepLen + 1; ++i) {
        if (memcmp(str + 2 * i, sep, sepLen * 2) == 0) {
            if (begin < i) {
                UnicodeString us(str + (2 * begin), i - begin);
                result.push_back(us);
            }

            begin = i + sepLen;
        }
    }

    if (begin < length) {
        UnicodeString us(str + (2 * begin), length - begin);
        result.push_back(us);
    }

    return result;
}

std::deque<UnicodeString> UnicodeString::split(size_t len) const {
    std::deque<UnicodeString> result;

    for (size_t i = 0; i < length; i += len) {
        UnicodeString us(str + i * 2, std::min(len, length - i));
        result.push_back(us);
    }

    return result;
}

UnicodeString &UnicodeString::operator +=(const UnicodeString &rhs) {
    char *sum = new char[(length + rhs.length) * 2];
    memcpy(sum, str, length * 2);
    memcpy(sum + (length * 2), rhs.str, rhs.length * 2);

    delete[] str;

    str = sum;
    length = length + rhs.length;
    return *this;
}

bool UnicodeString::operator ==(const UnicodeString &rhs) const {
    return this == &rhs || (length == rhs.length && memcmp(str, rhs.str, length) == 0);
}

int UnicodeString::byteSize() const {
    return length * 2 + 4;
}

void UnicodeString::writeToData(char *data) const {
    memcpy(data, &length, 4);
    memcpy(data + 4, str, length * 2);
}

UnicodeString parseUnicodeString(const char *source, const char *sourceEnd) {
    if (source + 4 >= sourceEnd) {
        throw std::runtime_error("Unicode string out of bounds.");
    }
    UnicodeString res;
    res.length = *((uint32_t *)source);
    if (source + 4 + 2 * res.length > sourceEnd) {
        throw std::runtime_error("Unicode string out of bounds.");
    }
    char *usStr = new char[res.length * 2];
    memcpy(usStr, source + 4, res.length * 2);
    res.str = usStr;
    return res;
}

char* getSourceStrOffset(const char *source, const char *sourceEnd)
{
	
	if (source + 4 >= sourceEnd) {
		throw std::runtime_error("Unicode string out of bounds.");
	}
	UnicodeString res;
	res.length = *((uint32_t *)source);
	if (source + 4 + 2 * res.length > sourceEnd) {
		throw std::runtime_error("Unicode string out of bounds.");
	}
	return (char*)source + 4;
}

bool isShortUnicodeString(const char *source, const char *sourceEnd) {
    if (source + 4 >= sourceEnd) {
        return false;
    }
    uint32_t length = *((uint32_t *)source);
    return length <= shortLength && source + 4 + 2 * length <= sourceEnd;
}

const char *searchBackwardsForShortUnicodeString(const char *contentBegin, size_t startOffset) {
    const char *curr = contentBegin + startOffset - 6;
    const char *minPos = contentBegin - std::min(startOffset, shortLength * 2 + 4);
    while (curr >= minPos) {
        if (curr[5] == 0x00 && curr[6] == 0x00) {
            break;
        }

        uint32_t currAsInt = *((uint32_t *)curr);
        if (currAsInt <= shortLength && curr + (4 + currAsInt * 2) == contentBegin + startOffset) {
            return curr;
        }

        curr -= 2;
    }

    return 0;
}

void unicodeStringToUtf8(char **utf8, size_t *utf8Len, const UnicodeString &us) {
    iconv_t conv = iconv_open("UTF-8", "UTF-16LE");
    if ((int)conv == -1) {
        throw std::runtime_error("Cannot initialize iconv.");
    }

    const size_t startUtf8Bytes = us.length * 3;
    size_t utf8BytesLeft = startUtf8Bytes;
    *utf8 = new char[utf8BytesLeft];
    char *buff = *utf8;
    const char *usStr = us.str;
    size_t usBytesLeft = us.length * 2;
	size_t res = iconv(conv, (const char **)(&usStr), &usBytesLeft, &buff, &utf8BytesLeft);
    //size_t res = iconv(conv, const_cast<char **>(&usStr), &usBytesLeft, &buff, &utf8BytesLeft);
    if (res == (size_t)-1) {
        throw std::runtime_error("iconv conversion fail.");
    }
    *utf8Len = startUtf8Bytes - utf8BytesLeft;

    iconv_close(conv);
}

UnicodeString utf8ToUnicodeString(const char *utf8, size_t utf8Len) {
    iconv_t conv = iconv_open("UTF-16LE", "UTF-8");
    if ((int)conv == -1) {
        throw std::runtime_error("Cannot initialize iconv.");
    }

    UnicodeString us;
    const size_t startUsBytes = utf8Len * 2;
    size_t usBytesLeft = startUsBytes;
    char *buff = new char[usBytesLeft];
    us.str = buff;
    size_t utf8BytesLeft = utf8Len;
    const char *utf8Str = utf8;
	size_t res = iconv(conv, (const char **)(&utf8Str), &utf8BytesLeft, &buff, &usBytesLeft);
    //size_t res = iconv(conv, const_cast<char **>(&utf8Str), &utf8BytesLeft, &buff, &usBytesLeft);
    if (res == (size_t)-1) {
        throw std::runtime_error("iconv conversion fail.");
    }
    us.length = (startUsBytes - usBytesLeft) / 2;

    iconv_close(conv);

    return us;
}

void printUtf8(std::ofstream &fout, const UnicodeString &us) {
    char *utf8;
    size_t len;
    unicodeStringToUtf8(&utf8, &len, us);
    assert(len > 0);
    fout.write(utf8, len);
}
