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

#include <algorithm>

namespace au {
namespace algo {

    struct Range final
    {
        struct Iterator final
            : std::iterator<std::random_access_iterator_tag, int, int>
        {
            int i, stride;

            Iterator(int i, int stride) : i(i), stride(stride)
            {
            }

            Iterator(Iterator it, int stride) : i(*it), stride(stride)
            {
            }

            int operator *() const
            {
                return i;
            }

            Iterator operator ++()
            {
                i += stride;
                return *this;
            }

            bool operator !=(Iterator other) const
            {
                return stride < 0 ? i > *other : i < *other;
            }
        };

        Range(int b, int e, int stride = 1)
            : stride(stride), b(b, stride), e(e, stride)
        {
        }

        Iterator begin() const
        {
            return b;
        }

        Iterator end() const
        {
            return e;
        }

        int stride;
        Iterator b, e;
    };

    inline Range range(int b, int e, int stride=1)
    {
        return Range(b, e, stride);
    }

    inline Range range(int e)
    {
        return Range(0, e);
    }

} }
