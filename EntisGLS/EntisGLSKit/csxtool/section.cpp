#include "section.h"

Section::Section():
    content(0) {
}

Section::~Section() {
    delete[] content;
}

std::ifstream &operator >>(std::ifstream &fin, Section &section) {
    fin.read((char *)&section, 16);
    section.content = new char[section.size];
    fin.read(section.content, section.size);
    return fin;
}

std::ofstream &operator <<(std::ofstream &fout, const Section &section) {
    fout.write((const char *)&section, 16);
    fout.write(section.content, section.size);
    return fout;
}
