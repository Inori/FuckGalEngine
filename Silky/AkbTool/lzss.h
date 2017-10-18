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
#include "types.h"

namespace au {
namespace algo {
namespace pack {

//    struct BitwiseLzssSettings final
//    {
//        size_t position_bits;
//        size_t size_bits;
//        size_t min_match_size;
//        size_t initial_dictionary_pos;
//    };

    struct BytewiseLzssSettings final
    {
        BytewiseLzssSettings();

        size_t initial_dictionary_pos;
    };

//    class BaseLzssWriter
//    {
//    public:
//        virtual ~BaseLzssWriter() {}
//        virtual void write_literal(const u8 literal) = 0;
//        virtual void write_repetition(
//            const size_t position_bits,
//            const size_t position,
//            const size_t size_bits,
//            const size_t size) = 0;
//        virtual bstr retrieve() = 0;
//    };

//    bstr lzss_decompress(
//        const bstr &input,
//        const size_t output_size,
//        const BitwiseLzssSettings &settings);

//    bstr lzss_decompress(
//        io::BaseBitStream &input_stream,
//        const size_t output_size,
//        const BitwiseLzssSettings &settings);

    bstr lzss_decompress(
        const bstr &input,
        const size_t output_size,
        const BytewiseLzssSettings &settings = BytewiseLzssSettings());

//    bstr lzss_compress(
//        const bstr &input, const algo::pack::BitwiseLzssSettings &settings);
//
//    bstr lzss_compress(
//        io::BaseByteStream &input_stream,
//        const BitwiseLzssSettings &settings);
//
    bstr lzss_compress(
        const bstr &input,
        const BytewiseLzssSettings &settings = BytewiseLzssSettings());
//
//    bstr lzss_compress(
//        io::BaseByteStream &input_stream,
//        const BytewiseLzssSettings &settings);
//
//    bstr lzss_compress(
//        io::BaseByteStream &input_stream,
//        const algo::pack::BitwiseLzssSettings &settings,
//        BaseLzssWriter &writer);

} } }
