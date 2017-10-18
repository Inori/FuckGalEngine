// Copyright (C) 2016 by rr-
//
// This file is part of arc_unpacker.
//
// arc_unpacker is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// arc_unpacker is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with arc_unpacker. If not, see <http://www.gnu.org/licenses/>.

#include "types.h"
#include <algorithm>

using namespace au;

const size_t bstr::npos = static_cast<size_t>(-1);

bstr::bstr()
{
}

bstr::bstr(const size_t n, u8 fill) : v(n, fill)
{
}

bstr::bstr(const u8 *str, const size_t size) : v(str, str + size)
{
}

bstr::bstr(const char *str, const size_t size) : v(str, str + size)
{
}

bstr::bstr(const std::string &other) : v(other.begin(), other.end())
{
}

const char *bstr::c_str() const
{
    return get<const char>();
}

std::string bstr::str(bool trim_to_zero) const
{
    if (trim_to_zero)
    {
        const auto pos = std::find(v.begin(), v.end(), '\0');
        return std::string(c_str(), pos != v.end() ? pos - v.begin() : size());
    }
    return std::string(c_str(), size());
}

bool bstr::empty() const
{
    return v.size() == 0;
}

size_t bstr::size() const
{
    return v.size();
}

size_t bstr::capacity() const
{
    return v.capacity();
}

size_t bstr::find(const bstr &other) const
{
    const auto pos = std::search(
        v.begin(), v.end(),
        other.get<u8>(), other.get<u8>() + other.size());
    if (pos == v.end())
        return bstr::npos;
    return pos - v.begin();
}

size_t bstr::find(const bstr &other, const size_t start_pos) const
{
    const auto pos = std::search(
        v.begin() + start_pos, v.end(),
        other.get<u8>(), other.get<u8>() + other.size());
    if (pos == v.end())
        return bstr::npos;
    return pos - v.begin();
}

bstr bstr::substr(int start) const
{
    if (start > static_cast<int>(size()))
        return ""_b;
    while (start < 0)
        start += v.size();
    return bstr(get<const u8>() + start, size() - start);
}

bstr bstr::substr(int start, int size) const
{
    if (start > static_cast<int>(v.size()))
        return ""_b;
    while (size < 0)
        size += v.size();
    while (start < 0)
        start += v.size();
    if (start > static_cast<int>(v.size()))
        return ""_b;
    if (start + size > static_cast<int>(v.size()))
        return substr(start, v.size() - start);
    return bstr(get<const u8>() + start, size);
}

void bstr::replace(int start, int size, const bstr &what)
{
    while (size < 0)
        size += v.size();
    while (start < 0)
        start += v.size();
    if (start > static_cast<int>(v.size()))
    {
        v.insert(v.end(), what.begin(), what.end());
        return;
    }
    if (start + size > static_cast<int>(v.size()))
    {
        replace(start, v.size() - start, what);
        return;
    }
    if (size > 0)
        v.erase(v.begin() + start, v.begin() + start + size);
    v.insert(v.begin() + start, what.begin(), what.end());
}

void bstr::resize(const size_t how_much)
{
    v.resize(how_much);
}

void bstr::reserve(const size_t how_much)
{
    v.reserve(how_much);
}

bstr bstr::operator +(const bstr &other) const
{
    bstr ret(v.data(), size());
    ret += other;
    return ret;
}

void bstr::operator +=(const bstr &other)
{
    v.insert(v.end(), other.get<u8>(), other.get<u8>() + other.size());
}

void bstr::operator +=(const char c)
{
    v.push_back(c);
}

void bstr::operator +=(const u8 c)
{
    v.push_back(c);
}

bool bstr::operator ==(const bstr &other) const
{
    return v == other.v;
}

bool bstr::operator !=(const bstr &other) const
{
    return v != other.v;
}

bool bstr::operator <=(const bstr &other) const
{
    return v <= other.v;
}

bool bstr::operator >=(const bstr &other) const
{
    return v >= other.v;
}

bool bstr::operator <(const bstr &other) const
{
    return v < other.v;
}

bool bstr::operator >(const bstr &other) const
{
    return v > other.v;
}

u8 &bstr::operator [](const size_t pos)
{
    return v[pos];
}

const u8 &bstr::operator [](const size_t pos) const
{
    return v[pos];
}

u8 &bstr::at(const size_t pos)
{
    return v.at(pos);
}

const u8 &bstr::at(const size_t pos) const
{
    return v.at(pos);
}
