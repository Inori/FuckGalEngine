#include "header.h"

std::ifstream &operator >>(std::ifstream &fin, Header &header) {
    fin.read((char *)&header, sizeof(Header));
    return fin;
}

std::ofstream &operator <<(std::ofstream &fout, const Header &header) {
    fout.write((const char *)&header, sizeof(Header));
    return fout;
}
