#ifndef TALK_H
#define TALK_H

#include <fstream>
#include "unicodestring.h"

class Talk {
public:
    Talk(): index(-1), speakerPos(0), talkPos(0), talkContentBytes(0) {}

    int index;
    size_t speakerPos;
    UnicodeString speaker;
    size_t talkPos;
    size_t talkContentBytes;
    UnicodeString message;

	//the string's real offset in *.csx file
	size_t realspeakerOffset;
	size_t realmessageOffset;

    bool hasSpeaker() const { return speaker.length != 0; }
    size_t writeToData(char *data) const;
};

bool isTalkPrefix(const char *contentBegin, size_t contentLength, size_t offset);
Talk parseTalk(const char *contentBegin, size_t contentLength, size_t offset);

#endif // TALK_H
