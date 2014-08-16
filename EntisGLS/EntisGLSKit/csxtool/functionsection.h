#ifndef FUNCTIONSECTION_H
#define FUNCTIONSECTION_H

#include <deque>
#include "section.h"
#include "positioncomputer.h"

class FunctionSection : public Section {
public:
    void updateOffsets(const PositionComputer &posCom);
};

#endif // FUNCTIONSECTION_H
