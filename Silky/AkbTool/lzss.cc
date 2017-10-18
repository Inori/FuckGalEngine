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

#include "lzss.h"
#include <array>
#include "ptr.h"
#include "range.h"

using namespace au;

algo::pack::BytewiseLzssSettings::BytewiseLzssSettings()
    : initial_dictionary_pos(0xFEE)
{
}

bstr algo::pack::lzss_decompress(
    const bstr &input,
    const size_t output_size,
    const BytewiseLzssSettings &settings)
{
    std::array<u8, 0x1000> dict = {0};
    auto dict_ptr
        = algo::make_cyclic_ptr(dict.data(), dict.size())
        + settings.initial_dictionary_pos;

    bstr output(output_size);
    auto output_ptr = algo::make_ptr(output);
    auto input_ptr = algo::make_ptr(input);

    u16 control = 0;
    while (output_ptr.left())
    {
        control >>= 1;
        if (!(control & 0x100))
        {
            if (!input_ptr.left()) break;
            control = *input_ptr++ | 0xFF00;
        }
        if (control & 1)
        {
            if (!input_ptr.left()) break;
            const auto b = *input_ptr++;
            *output_ptr++ = b;
            *dict_ptr++ = b;
        }
        else
        {
            if (input_ptr.left() < 2) break;
            const auto lo = *input_ptr++;
            const auto hi = *input_ptr++;
            const auto look_behind_pos = lo | ((hi & 0xF0) << 4);
            auto repetitions = (hi & 0xF) + 3;
            auto source_ptr
                = algo::make_cyclic_ptr(dict.data(), dict.size())
                + look_behind_pos;
            while (repetitions-- && output_ptr.left())
            {
                const auto b = *source_ptr++;
                *output_ptr++ = b;
                *dict_ptr++ = b;
            }
        }
    }
    return output;
}

// fake compress
bstr algo::pack::lzss_compress(
    const bstr &input, const algo::pack::BytewiseLzssSettings &settings)
{
    const auto control_size = input.size() / 8;
    const u8 dirty_size = input.size() % 8;
    const auto output_size = control_size + (dirty_size ? 1 : 0) + input.size();
    bstr output(output_size);
    auto i = 0;
    auto j = 0;
    for (auto control : range(control_size))
    {
        output[i++] = 0xFF;
        for (auto k : range(8))
        {
            output[i++] = input[j++];
        }
    }
    if (dirty_size)
    {
        const u8 dirty_control = (1 << dirty_size) - 1;
        output[i++] = dirty_control;
        for (auto k : range(dirty_size))
        {
            output[i++] = input[j++];
        }
        
    }
    return output;
}
