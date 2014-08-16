#ifndef SECTION_H
#define SECTION_H

#include <fstream>
#include <stdint.h>

class Section {
public:
    Section();
    ~Section();

    char id[8];
    uint32_t size;
    uint32_t unknown;
    char *content;
};

std::ifstream &operator >>(std::ifstream &fin, Section &section);
std::ofstream &operator <<(std::ofstream &fout, const Section &section);

#endif // SECTION_H
