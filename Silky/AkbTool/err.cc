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

#include "err.h"
#include "format.h"

using namespace au;
using namespace au::err;

GeneralError::GeneralError(const std::string &desc) : std::runtime_error(desc)
{
}

UsageError::UsageError(const std::string &desc) : GeneralError(desc)
{
}

DataError::DataError(const std::string &desc) : GeneralError(desc)
{
}

RecognitionError::RecognitionError() : DataError("Data not recognized")
{
}

RecognitionError::RecognitionError(const std::string &desc) : DataError(desc)
{
}

CorruptDataError::CorruptDataError(const std::string &desc) : DataError(desc)
{
}

BadDataSizeError::BadDataSizeError() : DataError("Bad data size")
{
}

BadDataOffsetError::BadDataOffsetError() : DataError("Bad data offset")
{
}

IoError::IoError(const std::string &desc) : GeneralError(desc)
{
}

EofError::EofError() : IoError("Premature end of file")
{
}

FileNotFoundError::FileNotFoundError(const std::string &desc) : IoError(desc)
{
}

NotSupportedError::NotSupportedError(const std::string &desc)
    : GeneralError(desc)
{
}

UnsupportedBitDepthError::UnsupportedBitDepthError(size_t bit_depth)
    : NotSupportedError(algo::format("Unsupported bit depth: %d", bit_depth))
{
}

UnsupportedChannelCountError::UnsupportedChannelCountError(size_t channel_count)
    : NotSupportedError(algo::format(
        "Unsupported channel count: %d", channel_count))
{
}

UnsupportedVersionError::UnsupportedVersionError(int version)
    : NotSupportedError(algo::format("Unsupported version: %d", version))
{
}

UnsupportedVersionError::UnsupportedVersionError()
    : NotSupportedError("Unsupported version")
{
}
