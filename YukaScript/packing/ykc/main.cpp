
#include "stdafx.h"
#include "funcs.h"

namespace fs = boost::filesystem;

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
    {
        wprintf_s(L"ykc <input>\n    pack if input is directory;\n    unpack if input is file.\n");
        return 1;
    }
    if (fs::is_directory(argv[1]))
    {
        return repack(argv[1]);
    }
    else
    {
        return unpack(argv[1]);
    }
}
