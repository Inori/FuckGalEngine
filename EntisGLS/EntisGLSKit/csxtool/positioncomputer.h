#ifndef POSITIONCOMPUTER_H
#define POSITIONCOMPUTER_H

#include <map>
#include <cstddef>

class PositionComputer {
public:
    size_t position(size_t originalPos) const;
    size_t reversePosition(size_t pos) const;
    size_t delta(size_t pos, size_t originalDelta) const;

    void reset();
    void setPos(size_t origPos, size_t pos);

private:
    std::map<size_t, size_t> offsetsMap;
};

#endif // POSITIONCOMPUTER_H
