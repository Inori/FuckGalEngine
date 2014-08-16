#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include "txtparser.h"
#include "header.h"

static const char *choiceTlPrefix = "[EN|CHOICE";
static const char *tlPrefix = "[EN";
static const char openingBracket[] = { '[', 0x00 };
static const char endingBracket[] = { ']', 0x00 };
static const char space[] = { ' ', 0x00 };
static const char utf8Bom[] = { 0xef, 0xbb, 0xbf };
static const char scenarioNewLine[] = { 0x0F, 0xFF };

TxtParser::Stats &TxtParser::Stats::operator +=(const TxtParser::Stats &rhs) {
    translatedMessages += rhs.translatedMessages;
    totalMessages += rhs.totalMessages;
    translatedChoices += rhs.translatedChoices;
    totalChoices += rhs.totalChoices;
    return *this;
}

double TxtParser::Stats::messagesPercent() const {
    if (totalMessages == 0) {
        return 100;
    } else {
        return 100 * (double)translatedMessages / (double)totalMessages;
    }
}

double TxtParser::Stats::choicesPercent() const {
    if (totalChoices == 0) {
        return 100;
    } else {
        return 100 * (double)translatedChoices / (double)totalChoices;
    }
}

void TxtParser::writeTxt(std::string utfFilename) const {
    std::ofstream fout(utfFilename.c_str(), std::ios_base::out | std::ios_base::binary);
    fout.write(utf8Bom, 3); // UTF-8 BOM

    for (size_t i = 0; i < choices.size(); ++i) {
        const Choice &choice = choices[i];

        fout << "[JP|CHOICE" << std::setw(2) << std::setfill('0') << choice.index << "] ";
        printUtf8(fout, choice.choice1);
        fout << "\r\n";

        fout << "[JP|CHOICE" << std::setw(2) << std::setfill('0') << choice.index << "] ";
        printUtf8(fout, choice.choice2);
        fout << "\r\n";

        fout << choiceTlPrefix << std::setw(2) << std::setfill('0') << choice.index << "] \r\n";
        fout << choiceTlPrefix << std::setw(2) << std::setfill('0') << choice.index << "] \r\n";

        fout << "\r\n\r\n";
    }

    for (size_t i = 0; i < messages.size(); ++i) {
        const Talk &talk = messages[i];
        std::deque<UnicodeString> lines = talk.message.split(scenarioNewLine, sizeof(scenarioNewLine) / 2);

        for (size_t j = 0; j < lines.size(); ++j) {
            fout << "[JP" << std::setw(5) << std::setfill('0') << talk.index << "] ";

            if (j == 0 && talk.hasSpeaker()) {
                fout << "[";
                printUtf8(fout, talk.speaker);
                fout << "] ";
            }

            printUtf8(fout, lines[j]);
            fout << "\r\n";
        }

        for (size_t j = 0; j < lines.size(); ++j) {
            fout << tlPrefix << std::setw(5) << std::setfill('0') << talk.index << "] ";

            if (j == 0 && talk.hasSpeaker()) {
                fout << "[";
                printUtf8(fout, talk.speaker);
                fout << "] ";
            }

            // exporting message id and position in original csx for debug
			fout << std::hex << (talk.speakerPos + sizeof(Header)+16) + 4 << " " << std::hex << (talk.talkPos + sizeof(Header)+16) + 0x1A << std::dec;

            fout << "\r\n";
        }

        fout << "\r\n\r\n";
    }

    fout.close();
}

static int parseNumber(const char *numStr, size_t length) {
    int res = 0;
    for (size_t i = 0; i < length; ++i) {
        res *= 10;
        if (numStr[i] < '0' || numStr[i] > '9') {
            throw std::runtime_error("Invalid number.");
        }
        res += numStr[i] - '0';
    }
    return res;
}

static std::deque<std::pair<const char *, size_t> > splitMem(const char *str, size_t strLen, const char *sep, size_t sepLen) {
    assert(sepLen > 0);
    std::deque<std::pair<const char *, size_t> > result;
    size_t begin = 0;

    for (size_t i = 0; i + sepLen - 1 < strLen; ++i) {
        if (begin < i && memcmp(str + i, sep, sepLen) == 0) {
            result.push_back(std::make_pair(str + begin, i - begin));
            begin = i + sepLen;
        }
    }

    if (begin < strLen) {
        result.push_back(std::make_pair(str + begin, strLen - begin));
    }

    return result;
}

static size_t numOfSpaces(const char *str, size_t length) {
    size_t spaces = 0;
    while (spaces < length && memcmp(str + spaces, space, sizeof(space)) == 0) {
        ++spaces;
    }
    return spaces;
}

static size_t findSubstring(const char *str, size_t strLen, const char *substr, size_t substrLen) {
    for (size_t i = 0; i < strLen - substrLen + 1; ++i) {
        if (memcmp(str + i * 2, substr, substrLen) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

TxtParser::Stats TxtParser::parseTxt(std::string utfFilename) {
    size_t initMessagesSize = messages.size();
    size_t initChoicesSize = choices.size();
    Stats stats;

    std::ifstream fin(utfFilename.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!fin) {
        throw std::runtime_error("Couldn't open file " + utfFilename + ".");
    }

    int buffLen = 4 * 1024 * 1024;
    int len = 0;
    char *buff = new char[buffLen];

    while (fin) {
        fin.read(buff + len, buffLen - len);
        len += fin.gcount();
        char *temp = new char[buffLen + 1024 * 1024];
        memcpy(temp, buff, buffLen);
        delete[] buff;
        buff = temp;
        buffLen += 1024 * 1024;
    }

    if (len < 128) {
        throw std::runtime_error("File too short.");
    }

    if (memcmp(buff, utf8Bom, sizeof(utf8Bom)) != 0) {
        throw std::runtime_error("File is not a UTF-8 file with BOM.");
    }

    const size_t tlPrefixLen = strlen(tlPrefix);
    const size_t choiceTlPrefixLen = strlen(choiceTlPrefix);
    bool expectsSecondChoice = false;

    std::deque<std::pair<const char *, size_t> > lines = splitMem(buff + 3, len - 3, "\r\n", 2);
    for (size_t i = 0; i < lines.size(); ++i) {
        const char *lineStr = lines[i].first;
        if (choiceTlPrefixLen + 3 < lines[i].second && memcmp(lineStr, choiceTlPrefix, choiceTlPrefixLen) == 0) {
            if (expectsSecondChoice) {
                ++stats.totalChoices;
            }

            int index = parseNumber(lineStr + choiceTlPrefixLen, 2);
            if (lineStr[choiceTlPrefixLen + 2] != ']') {
                throw std::runtime_error("Invalid choice.");
            }

            UnicodeString line = utf8ToUnicodeString(lineStr + choiceTlPrefixLen + 3, lines[i].second - choiceTlPrefixLen - 3);
            size_t offset = 2 * numOfSpaces(line.str, line.length);

            UnicodeString choiceString = UnicodeString(line.str + offset, line.length - offset / 2);

            if (choiceString.length > 0) {
                if (expectsSecondChoice) {
                    if (!choices.empty() && choices.back().index != index) {
                        throw std::runtime_error("Invalid choice index.");
                    }
                    choices.back().choice2 = choiceString;
                } else {
                    Choice choice;
                    choice.index = index;
                    choice.choice1 = choiceString;
                    choices.push_back(choice);
                }
            }

            expectsSecondChoice = !expectsSecondChoice;
        } else if (tlPrefixLen + 6 < lines[i].second && memcmp(lineStr, tlPrefix, tlPrefixLen) == 0) {
            Talk talk;
            talk.index = parseNumber(lineStr + tlPrefixLen, 5);
            if (lineStr[tlPrefixLen + 5] != ']') {
                throw std::runtime_error("Invalid message.");
            }

            if (messages.size() == 0 || messages.back().index != talk.index) {
                ++stats.totalMessages;
            }

            UnicodeString line = utf8ToUnicodeString(lineStr + tlPrefixLen + 6, lines[i].second - tlPrefixLen - 6);
            size_t offset = 2 * numOfSpaces(line.str, line.length);

            if (offset + sizeof(openingBracket) < line.length * 2 && memcmp(line.str + offset, openingBracket, sizeof(openingBracket)) == 0) {
                offset += sizeof(openingBracket);
                const char *usStr = line.str + offset;
                size_t usStrLen = findSubstring(line.str + offset, line.length - offset / 2, endingBracket, sizeof(endingBracket));
                talk.speaker = UnicodeString(usStr, usStrLen);
                if (talk.speaker.length == (size_t)-1) {
                    throw std::runtime_error("Invalid speaker name.");
                }
                offset += 2 * talk.speaker.length + sizeof(endingBracket);
                offset += 2 * numOfSpaces(line.str + offset, line.length - offset / 2);
            }

            talk.message = UnicodeString(line.str + offset, line.length - offset / 2);
            if (talk.message.length > 0) {
                if (messages.size() == 0 || messages.back().index != talk.index) {
                    messages.push_back(talk);
                } else {
                    if (talk.hasSpeaker()) {
                        throw std::runtime_error("Speaker name not in first line.");
                    }
                    messages.back().message += UnicodeString(scenarioNewLine, sizeof(scenarioNewLine) / 2);
                    messages.back().message += talk.message;
                }
            }
        }
    }

    if (expectsSecondChoice) {
        throw std::runtime_error("Both choices from one question or neither of them must be translated.");
    }

    delete[] buff;

    stats.translatedMessages = messages.size() - initMessagesSize;
    stats.translatedChoices = choices.size() - initChoicesSize;

    return stats;
}

std::ostream &operator <<(std::ostream &os, const TxtParser::Stats &stats) {
    os << stats.translatedMessages << "/" << stats.totalMessages << " (" << std::setprecision(2) << std::setiosflags(std::ios_base::fixed) << stats.messagesPercent() << "%) lines, ";
    os << stats.translatedChoices << "/" << stats.totalChoices << " (" << stats.choicesPercent() << "%) choices";
    return os;
}
