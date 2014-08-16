#include <cassert>
#include "positioncomputer.h"

size_t PositionComputer::position(size_t originalPos) const {
    if (offsetsMap.empty()) {
        return originalPos;
    }

    std::map<size_t, size_t>::const_iterator it = offsetsMap.upper_bound(originalPos);

    if (it == offsetsMap.end()) {
        --it;
    } else if (it == offsetsMap.begin()) {
        return originalPos;
    } else {
        --it;
    }

    return originalPos + it->second - it->first;
}

std::map<size_t, size_t>::const_iterator moved(std::map<size_t, size_t>::const_iterator it, int direction) {
    if (direction >= 0) {
        ++it;
    } else {
        --it;
    }

    return it;
}

size_t PositionComputer::reversePosition(size_t pos) const {
    if (offsetsMap.empty() || pos < offsetsMap.begin()->second) {
        return pos;
    }

    std::map<size_t, size_t>::const_iterator it = offsetsMap.upper_bound(pos);

    while (it != offsetsMap.end() && it->second < pos) {
        ++it;
    }

    if (it == offsetsMap.end()) {
        --it;
    }

    while (it->second > pos) {
        --it;
    }

    assert(moved(it, 1) == offsetsMap.end() || moved(it, 1)->second > pos);

    return pos + it->first - it->second;
}

size_t PositionComputer::delta(size_t pos, size_t originalDelta) const {
    assert(position(reversePosition(pos)) == pos);
    return position(reversePosition(pos) + originalDelta) - pos;
}

void PositionComputer::reset() {
    offsetsMap.clear();
}

void PositionComputer::setPos(size_t origPos, size_t pos) {
    offsetsMap[origPos] = pos;
    assert(offsetsMap.find(origPos) == offsetsMap.begin() || moved(offsetsMap.find(origPos), -1)->second < pos);
    assert(moved(offsetsMap.find(origPos), 1) == offsetsMap.end() || moved(offsetsMap.find(origPos), 1)->second > pos);
}
