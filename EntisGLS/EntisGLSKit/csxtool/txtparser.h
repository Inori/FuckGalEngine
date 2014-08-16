#ifndef TXTPARSER_H
#define TXTPARSER_H

#include <deque>
#include <string>
#include <ostream>
#include "talk.h"
#include "choice.h"

class TxtParser {
public:
    std::deque<Talk> messages;
    std::deque<Choice> choices;

    struct Stats {
        size_t translatedMessages;
        size_t totalMessages;
        size_t translatedChoices;
        size_t totalChoices;

        Stats(): translatedMessages(0), totalMessages(0), translatedChoices(0), totalChoices(0) {}
        Stats &operator +=(const Stats &other);
        double messagesPercent() const;
        double choicesPercent() const;
    };

    void writeTxt(std::string utfFilename) const;
    Stats parseTxt(std::string utfFilename);
};

std::ostream &operator <<(std::ostream &os, const TxtParser::Stats &stats);

#endif // TXTPARSER_H
