#include <stdexcept>
#include <cstring>
#include "choice.h"

const char choiceSequencePrefix[] = { 0x02, 0x00, 0x06 };
const char choicePostfix[] = { 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x08, 0x05, 0x02, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 'A', 0x00, 'd', 0x00, 'd', 0x00, 'S', 0x00, 'e', 0x00, 'l', 0x00, 'e', 0x00, 'c', 0x00, 't', 0x00 };
const char choiceContPostfix[] = { 0x01, 0x02, 0x00, 0x06 };
const char choiceFinalPostfix[] = { 0x01, 0x02, 0x03, 0x04 };

size_t Choice::byteSize() const {
    return sizeof(choiceSequencePrefix) + 2 * sizeof(choicePostfix) + sizeof(choiceContPostfix) + sizeof(choiceFinalPostfix) + choice1.byteSize() + choice2.byteSize();
}

size_t Choice::writeToData(char *data) const {
    size_t len = 0;

    memcpy(data, choiceSequencePrefix, sizeof(choiceSequencePrefix));
    len += sizeof(choiceSequencePrefix);

    choice1.writeToData(data + len);
    len += choice1.byteSize();

    memcpy(data + len, choicePostfix, sizeof(choicePostfix));
    len += sizeof(choicePostfix);
    memcpy(data + len, choiceContPostfix, sizeof(choiceContPostfix));
    len += sizeof(choiceContPostfix);

    choice2.writeToData(data + len);
    len += choice2.byteSize();

    memcpy(data + len, choicePostfix, sizeof(choicePostfix));
    len += sizeof(choicePostfix);
    memcpy(data + len, choiceFinalPostfix, sizeof(choiceFinalPostfix));
    len += sizeof(choiceFinalPostfix);

    return len;
}

bool tryParseChoice(Choice *choice, const char *contentBegin, size_t contentLength, size_t offset) {
    const char *contentEnd = contentBegin + contentLength;
    size_t curr = offset;

    choice->choicePos = offset;

    if (curr + sizeof(choiceSequencePrefix) > contentLength || memcmp(contentBegin + curr, choiceSequencePrefix, sizeof(choiceSequencePrefix)) != 0) {
        return false;
    }
    curr += sizeof(choiceSequencePrefix);

    if (isShortUnicodeString(contentBegin + curr, contentEnd)) {
        choice->choice1 = parseUnicodeString(contentBegin + curr, contentEnd);
        curr += choice->choice1.byteSize();
    } else {
        return false;
    }

    if (curr + sizeof(choicePostfix) > contentLength || memcmp(contentBegin + curr, choicePostfix, sizeof(choicePostfix)) != 0) {
        return false;
    }
    curr += sizeof(choicePostfix);

    if (curr + sizeof(choiceContPostfix) > contentLength || memcmp(contentBegin + curr, choiceContPostfix, sizeof(choiceContPostfix)) != 0) {
        return false;
    }
    curr += sizeof(choiceContPostfix);

    if (isShortUnicodeString(contentBegin + curr, contentEnd)) {
        choice->choice2 = parseUnicodeString(contentBegin + curr, contentEnd);
        curr += choice->choice2.byteSize();
    } else {
        return false;
    }

    if (curr + sizeof(choicePostfix) > contentLength || memcmp(contentBegin + curr, choicePostfix, sizeof(choicePostfix)) != 0) {
        return false;
    }
    curr += sizeof(choicePostfix);

    if (curr + sizeof(choiceFinalPostfix) > contentLength || memcmp(contentBegin + curr, choiceFinalPostfix, sizeof(choiceFinalPostfix)) != 0) {
        return false;
    }

    return true;
}
