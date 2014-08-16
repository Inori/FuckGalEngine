#ifndef CHOICE_H
#define CHOICE_H

#include <fstream>
#include "unicodestring.h"

class Choice {
public:
    Choice(): index(-1), choicePos(0) {}

    int index;
    size_t choicePos;
    UnicodeString choice1;
    UnicodeString choice2;

    size_t byteSize() const;
    size_t writeToData(char *data) const;
};

bool tryParseChoice(Choice *choice, const char *contentBegin, size_t contentLength, size_t offset);

#endif // CHOICE_H
