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

#include <stdexcept>

namespace au {
namespace err {

    struct GeneralError : public std::runtime_error
    {
        virtual ~GeneralError() {}
        GeneralError(const std::string &description);
    };

    struct UsageError final : public GeneralError
    {
        UsageError(const std::string &description);
    };

    struct DataError : public GeneralError
    {
        virtual ~DataError() {}
    protected:
        DataError(const std::string &description);
    };

    struct RecognitionError final : public DataError
    {
        RecognitionError();
        RecognitionError(const std::string &description);
    };

    struct CorruptDataError final : public DataError
    {
        CorruptDataError(const std::string &description);
    };

    struct BadDataSizeError final : public DataError
    {
        BadDataSizeError();
    };

    struct BadDataOffsetError final : public DataError
    {
        BadDataOffsetError();
    };

    struct IoError : public GeneralError
    {
        virtual ~IoError() {}
        IoError(const std::string &description);
    };

    struct EofError final : public IoError
    {
        EofError();
    };

    struct FileNotFoundError final : public IoError
    {
        FileNotFoundError(const std::string &description);
    };

    struct NotSupportedError : public GeneralError
    {
        virtual ~NotSupportedError() {}
        NotSupportedError(const std::string &description);
    };

    struct UnsupportedBitDepthError final : public NotSupportedError
    {
        UnsupportedBitDepthError(size_t bit_depth);
    };

    struct UnsupportedChannelCountError final : public NotSupportedError
    {
        UnsupportedChannelCountError(size_t channel_count);
    };

    struct UnsupportedVersionError final : public NotSupportedError
    {
        UnsupportedVersionError();
        UnsupportedVersionError(int version);
    };

} }
