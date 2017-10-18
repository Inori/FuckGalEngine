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

#include "format.h"
#include <cstdio>
#include <memory>

using namespace au;

std::string algo::format(const std::string fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    const auto ret = format(fmt, args);
    va_end(args);
    return ret;
}

std::string algo::format(const std::string fmt, std::va_list args)
{
    size_t size;
    std::va_list args_copy;
    va_copy(args_copy, args);

    {
        const size_t buffer_size = 256;
        char buffer[buffer_size];
        size = vsnprintf(buffer, buffer_size, fmt.c_str(), args);
        if (size < buffer_size)
        {
            va_end(args_copy);
            return std::string(buffer, size);
        }
    }

    size++;
    auto buf = std::make_unique<char[]>(size);
    vsnprintf(buf.get(), size, fmt.c_str(), args_copy);
    va_end(args_copy);
    return std::string(buf.get(), buf.get() + size - 1);
}
