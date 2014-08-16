#ifndef HEADER_H
#define HEADER_H

#include <fstream>
#include <stdint.h>

struct Header {
    char fileType[8];
    uint32_t unknown1;
    uint32_t zero;
    char imageType[40];
    uint32_t size;
    uint32_t unknown2;
};

std::ifstream &operator >>(std::ifstream &fin, Header &header);
std::ofstream &operator <<(std::ofstream &fout, const Header &header);

#endif // HEADER_H
