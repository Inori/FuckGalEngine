#pragma comment(linker,"/ENTRY:main")
#pragma comment(linker,"/SECTION:.text,ERW /MERGE:.rdata=.text")
#pragma comment(linker,"/SECTION:.Amano,ERW /MERGE:.text=.Amano")

#include <Windows.h>
#include "krkr2.h"
#include "Mem.cpp"

INIT_CONSOLE_HANDLER

ForceInline Void main2(Int argc, WCHAR **argv)
{
    if (argc == 1)
        return;

    CKrkr2 rs;
    while (--argc)
        rs.Auto(*++argv);
}

void __cdecl main(Int argc, WCHAR **argv)
{
    getargsW(&argc, &argv);
    main2(argc, argv);
    exit(0);
}