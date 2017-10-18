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

#pragma once

#include <string>
#include <vector>

namespace au {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using s8 = int8_t;
    using s16 = int16_t;
    using s32 = int32_t;
    using s64 = int64_t;

    using f32 = float;
    using f64 = double;

    // off_t is not used here because it may be 32 bit only on Windows.
    // see file_stream.cc for details on handling big files
    using soff_t = s64;
    using uoff_t = u64;

    struct bstr final
    {
        static const size_t npos;

        bstr();
        bstr(const size_t n, u8 fill = 0);
        bstr(const std::string &other);
        bstr(const u8 *str, const size_t size);
        bstr(const char *str, const size_t size);

        bool empty() const;
        size_t size() const;
        size_t capacity() const;
        void resize(const size_t how_much);
        void reserve(const size_t how_much);

        size_t find(const bstr &other) const;
        size_t find(const bstr &other, const size_t start_pos) const;
        bstr substr(const int start) const;
        bstr substr(const int start, const int size) const;
        void replace(const int start, const int size, const bstr &what);

        template<typename T> T *get()
        {
            return reinterpret_cast<T*>(v.data());
        }

        template<typename T> T *end()
        {
            return v.empty() ? nullptr : get<T>() + v.size() / sizeof(T);
        }

        template<typename T> const T *get() const
        {
            return reinterpret_cast<const T*>(v.data());
        }

        template<typename T> const T *end() const
        {
            return v.empty() ? nullptr : get<T>() + v.size() / sizeof(T);
        }

        u8 *begin()
        {
            return get<u8>();
        }

        u8 *end()
        {
            return end<u8>();
        }

        const u8 *begin() const
        {
            return get<const u8>();
        }

        const u8 *end() const
        {
            return end<const u8>();
        }

        const char *c_str() const;
        std::string str(bool trim_to_zero = false) const;

        bstr operator +(const bstr &other) const;
        void operator +=(const bstr &other);
        void operator +=(const char c);
        void operator +=(const u8 c);
        bool operator ==(const bstr &other) const;
        bool operator !=(const bstr &other) const;
        bool operator <=(const bstr &other) const;
        bool operator >=(const bstr &other) const;
        bool operator <(const bstr &other) const;
        bool operator >(const bstr &other) const;
        u8 &operator [](const size_t pos);
        const u8 &operator [](const size_t pos) const;
        u8 &at(const size_t pos);
        const u8 &at(const size_t pos) const;

    private:
        std::vector<u8> v;
    };

    constexpr size_t operator "" _z(unsigned long long int value)
    {
        return value;
    }

    constexpr u8 operator "" _u8(char value)
    {
        return static_cast<u8>(value);
    }

    inline const u8 *operator "" _u8(const char *value, const size_t n)
    {
        return reinterpret_cast<const u8*>(value);
    }

    inline bstr operator "" _b(const char *value, const size_t n)
    {
        return bstr(value, n);
    }

}
