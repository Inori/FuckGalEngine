#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <stdint.h>
#include "imagesection.h"
#include "talk.h"

static const char chkFlagOnShortHop[] = { 0x09, 0x00, 0x00, 0x00, 'C', 0x00, 'h', 0x00, 'k', 0x00, 'F', 0x00, 'l', 0x00, 'a', 0x00, 'g', 0x00, 'O', 0x00, 'n', 0x00 };
static const char chkSelectShortHop[] = { 0x09, 0x00, 0x00, 0x00, 'C', 0x00, 'h', 0x00, 'k', 0x00, 'S', 0x00, 'e', 0x00, 'l', 0x00, 'e', 0x00, 'c', 0x00, 't', 0x00 };
static const char onFlagShortHop[] = { 0x06, 0x00, 0x00, 0x00, 'O', 0x00, 'n', 0x00, 'F', 0x00, 'l', 0x00, 'a', 0x00, 'g', 0x00 };
static const char isGameClearShortHop[] = { 0x0b, 0x00, 0x00, 0x00, 'I', 0x00, 's', 0x00, 'G', 0x00, 'a', 0x00, 'm', 0x00, 'e', 0x00, 'C', 0x00, 'l', 0x00, 'e', 0x00, 'a', 0x00, 'r', 0x00 };
static const char isRecollectModeShortHop[] = { 0x0f, 0x00, 0x00, 0x00, 'I', 0x00, 's', 0x00, 'R', 0x00, 'e', 0x00, 'c', 0x00, 'o', 0x00, 'l', 0x00, 'l', 0x00, 'e', 0x00, 'c', 0x00, 't', 0x00, 'M', 0x00, 'o', 0x00, 'd', 0x00, 'e', 0x00 };
static const uint16_t jumpPrefixArg1[] = { 0x0601, 0x0007 };
static const uint16_t jumpPrefixArg2 = 0x0605;
static const char doubleSkipPrefix = 0x06;

void ImageSection::extractStrings() {
    messages.clear();
    choices.clear();

    int messageIndex = 1;
    int choiceIndex = 1;
    for (size_t i = 0; i < size; ++i) {
        if (isTalkPrefix(content, size, i)) {
            Talk talk = parseTalk(content, size, i);

            talk.index = messageIndex;
            ++messageIndex;
            messages.push_back(talk);
        } else {
            Choice choice;
            if (tryParseChoice(&choice, content, size, i)) {
                choice.index = choiceIndex;
                choices.push_back(choice);
                ++choiceIndex;
            }
        }
    }
}

class ImageStringGroup {
public:
    ImageStringGroup(const Talk &talk): message(&talk), choice(0) {}
    ImageStringGroup(const Choice &choice): message(0), choice(&choice) {}

    bool operator <(const ImageStringGroup &rhs) const {
        return pos() < rhs.pos();
    }

    size_t pos() const {
        assert((message == 0) == !(choice == 0));
        if (message != 0) {
            return message->talkPos;
        } else {
            return choice->choicePos;
        }
    }

    const Talk *message;
    const Choice *choice;
};

void ImageSection::substituteStrings(std::deque<Talk> newMessages, std::deque<Choice> newChoices) {
    posComp.reset();

    // creating mapping index -> message/choice
    std::map<int, const Talk *> newMessagesMap;
    for (size_t i = 0; i < newMessages.size(); ++i) {
        if (newMessagesMap.find(newMessages[i].index) != newMessagesMap.end()) {
            throw std::runtime_error("Duplicate message index.");
        }
        newMessagesMap[newMessages[i].index] = &newMessages[i];
    }

    std::map<int, const Choice *> newChoicesMap;
    for (size_t i = 0; i < newChoices.size(); ++i) {
        if (newChoicesMap.find(newChoices[i].index) != newChoicesMap.end()) {
            throw std::runtime_error("Duplicate message index.");
        }
        newChoicesMap[newChoices[i].index] = &newChoices[i];
    }

    // initializing queue of messages and choices in order of appearance in image content
    std::deque<ImageStringGroup> origStringsQueue;
    for (size_t i = 0; i < messages.size(); ++i) {
        origStringsQueue.push_back(ImageStringGroup(messages[i]));
    }
    for (size_t i = 0; i < choices.size(); ++i) {
        origStringsQueue.push_back(ImageStringGroup(choices[i]));
    }
    std::sort(origStringsQueue.begin(), origStringsQueue.end());

    // estimating new size of content
    size_t totalNewStringsSize = 0;
    for (size_t i = 0; i < newMessages.size(); ++i) {
        totalNewStringsSize += newMessages[i].speaker.byteSize() + newMessages[i].message.byteSize();
    }
    for (size_t i = 0; i < newChoices.size(); ++i) {
        totalNewStringsSize += newChoices[i].byteSize();
    }

    // rewriting content with new messages while keeping track of changes in positions
    char *newContent = new char[size * 2 + totalNewStringsSize];
    size_t pos = 0;
    size_t origPos = 0;

    for (size_t i = 0; i < origStringsQueue.size(); ++i) {
        const ImageStringGroup &origImageString = origStringsQueue[i];

        if (origImageString.message != 0) {
            if (newMessagesMap.find(origImageString.message->index) == newMessagesMap.end()) {
                continue;
            }

            const Talk &origMessage = *origImageString.message;
            const Talk &message = *newMessagesMap[origMessage.index];

            if (origMessage.hasSpeaker()) {
                memcpy(newContent + pos, content + origPos, origMessage.speakerPos - origPos);
                pos += origMessage.speakerPos - origPos;
                message.speaker.writeToData(newContent + pos);
                pos += message.speaker.byteSize();
                origPos = origMessage.speakerPos + origMessage.speaker.byteSize();
                posComp.setPos(origPos, pos);
            }

            memcpy(newContent + pos, content + origPos, origMessage.talkPos - origPos);
            pos += origMessage.talkPos - origPos;
            origPos = origMessage.talkPos;
            pos += message.writeToData(newContent + pos);
            origPos += origMessage.talkContentBytes;
            posComp.setPos(origPos, pos);
        } else {
            assert(origImageString.choice != 0);
            if (newChoicesMap.find(origImageString.choice->index) == newChoicesMap.end()) {
                continue;
            }

            const Choice &origChoice = *origImageString.choice;
            const Choice &choice = *newChoicesMap[origChoice.index];

            memcpy(newContent + pos, content + origPos, origChoice.choicePos - origPos);
            pos += origChoice.choicePos - origPos;
            origPos = origChoice.choicePos;
            pos += choice.writeToData(newContent + pos);
            origPos += origChoice.byteSize();
            posComp.setPos(origPos, pos);
        }
    }

    memcpy(newContent + pos, content + origPos, size - origPos);
    pos += size - origPos;
    assert(posComp.position(size) == pos);

    delete[] content;
    content = newContent;
    size = pos;
}

void ImageSection::updateShortHops() {
    updatedJumpPos.clear();

    for (size_t i = 0; i < size; ++i) {
        Jump jump = detectJump(i);
        switch (jump.type) {
        case Jump::Jump1Arg:
            updateJump(jump.arg1Pos);
            break;

        case Jump::Jump2Arg:
            updateJump(jump.arg1Pos);
            updateJump(jump.arg2Pos);
            break;

        default:
            break;
        }
    }
}

ImageSection::Jump ImageSection::detectJump(size_t pos) {
    Jump jump;
    jump.type = Jump::NoJump;

    size_t currPos = pos;
    bool try1Arg = false;
    bool try2Arg = false;

    if (currPos + 4 >= size || content[currPos + 1] != 0 || content[currPos + 2] != 0 || content[currPos + 3] != 0) {
        // small optimization
        return jump;
    } else if (currPos + sizeof(chkFlagOnShortHop) + 6 <= size && memcmp(content + currPos, chkFlagOnShortHop, sizeof(chkFlagOnShortHop)) == 0) {
        currPos += sizeof(chkFlagOnShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(chkSelectShortHop) + 6 <= size && memcmp(content + currPos, chkSelectShortHop, sizeof(chkSelectShortHop)) == 0) {
        currPos += sizeof(chkSelectShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(onFlagShortHop) + 6 <= size && memcmp(content + currPos, onFlagShortHop, sizeof(onFlagShortHop)) == 0) {
        currPos += sizeof(onFlagShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(isGameClearShortHop) + 6 <= size && memcmp(content + currPos, isGameClearShortHop, sizeof(isGameClearShortHop)) == 0) {
        currPos += sizeof(isGameClearShortHop);
        try1Arg = true;
    } else if (currPos + sizeof(isRecollectModeShortHop) + 12 <= size && memcmp(content + currPos, isRecollectModeShortHop, sizeof(isRecollectModeShortHop)) == 0) {
        currPos += sizeof(isRecollectModeShortHop);
        try1Arg = true;
        try2Arg = true;
    }

    if (try1Arg) {
        uint16_t prefix = *((uint16_t *)(content + currPos));
        currPos += 2;
        for (size_t i = 0; i < sizeof(jumpPrefixArg1); ++i) {
            if (prefix == jumpPrefixArg1[i]) {
                jump.type = Jump::Jump1Arg;
                jump.arg1Pos = currPos;
                currPos += 4;
                break;
            }
        }

        if (try2Arg && jump.type == Jump::Jump1Arg) {
            uint16_t prefix = *((uint16_t *)(content + currPos));
            currPos += 2;
            if (prefix == jumpPrefixArg2) {
                jump.type = Jump::Jump2Arg;
                jump.arg2Pos = currPos;
            }
        }
    }

    return jump;
}

void ImageSection::updateJump(size_t pos) {
    if (updatedJumpPos.find(pos) != updatedJumpPos.end()) {
        return;
    }

    updatedJumpPos.insert(pos);

    uint32_t distance = *((uint32_t *)(content + pos));
    uint32_t delta = posComp.delta(pos + 4, distance);
    *((uint32_t *)(content + pos)) = delta;
    if (pos + delta + 5 > size) {
        throw std::runtime_error("Short jump distance out of bounds.");
    }

    //std::cerr << "updated jump on " << std::hex << (pos + 80) << std::dec << " " << distance << " -> " << delta << "\n";

    if (content[pos + delta + 4] == doubleSkipPrefix) {
        //std::cerr << "double-skip\n";
        updateJump(pos + delta + 5);
    } else {
        assert((content[pos + delta + 4] == 2 && content[pos + delta + 5] == 0 && content[pos + delta + 6] == 6) ||
                (content[pos + delta + 4] == 2 && content[pos + delta + 5] == 0 && content[pos + delta + 6] == 4) ||
                (content[pos + delta + 4] == 8 && content[pos + delta + 5] == 5 && content[pos + delta + 6] == 0) ||
                (content[pos + delta + 4] == 9 && content[pos + delta + 5] == 0 && content[pos + delta + 6] == 2) ||
                (content[pos + delta + 4] == 4 && content[pos + delta + 5] == 0 && content[pos + delta + 6] == 0));
    }
}
