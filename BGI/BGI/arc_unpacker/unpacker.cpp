
// https://github.com/Inori/FuckGalEngine/blob/master/BGI/BGI/arc_unpacker/unpacker.cpp

#include <Windows.h>
#include <string>
#include <direct.h>

#pragma pack(1)
typedef struct _arc_header
{
    char magic[0xC]; // "BURIKO ARC20"
    DWORD entry_count;
} arc_header;

typedef struct _arc_entry
{
    char filename[0x60];
    DWORD offset;
    DWORD size;
    DWORD unknown1;
    DWORD datasize;
    DWORD unknown2[4];
}arc_entry;

#pragma pack()

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("arc_unpacker by Fuyin\n");
        printf("usage: %s <input.arc>\n", argv[0]);
        return -1;
    }

    std::string in_filename = argv[1];
    FILE *fin = fopen(in_filename.c_str(), "rb");
    if (!fin)
    {
        printf("open input file error!\n");
        return -1;
    }

    DWORD arc_size = 0;
    fseek(fin, 0, SEEK_END);
    arc_size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    if (arc_size < sizeof(arc_header))
    {
        printf("format not match!\n");
        return -1;
    }

    arc_header* phdr = new arc_header;
    fread(phdr, sizeof(arc_header), 1, fin);

    if (strncmp(phdr->magic, "BURIKO ARC20", 0xC))
    {
        printf("format not match!\n");
        return -1;
    }

    printf("File count: %d\n", phdr->entry_count);
    arc_entry* plist = new arc_entry[phdr->entry_count];
    fread(plist, sizeof(arc_entry), phdr->entry_count, fin);

    DWORD data_block_addr = sizeof(arc_header) + sizeof(arc_entry) * phdr->entry_count;

    std::string dirname = in_filename.substr(0, in_filename.find("."));
    _mkdir(dirname.c_str());

    std::string blank(78, 0x20);
    double written_length = 0;

    for (DWORD i = 0; i < phdr->entry_count; ++i)
    {
        std::string out_filename = dirname + "\\" + plist[i].filename;
        
        printf("%6.2lf%%|%s", written_length / arc_size * 100, out_filename.c_str());

        DWORD fsiz = plist[i].size;
        char* data = new char[fsiz];
        fseek(fin, plist[i].offset + data_block_addr, SEEK_SET);
        fread(data, sizeof(char), fsiz, fin);

        FILE* fout = fopen(out_filename.c_str(), "wb");
        if (!fout)
        {
            printf("open output file error!\n");
            return -1;
        }

        fwrite(data, sizeof(char), fsiz, fout);
        written_length += fsiz;

        delete[] data;
        fclose(fout);

        printf("\r%s\r", blank.c_str());
    }

    printf("100.00%|OK\n");

    delete phdr;
    delete[] plist;
    fclose(fin);
    return 0;
}
