
// LC-ScriptEngine Unpacker
// Redistributed from https://github.com/polaris-/vn_translation_tools/blob/master/LC-ScriptEngine/lcse-unpack.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

// Struct of the list file ".lst"
#pragma pack(1)

struct Entry_t
{
    long offset;
    long length;
    char fname[0x40];
    long type;
};

struct Header_t
{
    long entry_count;
    char key[4]; // Embedded with "offset" of first entry
};

#pragma pack()

void export_file(FILE* fdata, Entry_t* entry, char key)
{
    // Copy content
    char* buff = new char[entry->length];
    fseek(fdata, entry->offset, SEEK_SET);
    fread(buff, 1, entry->length, fdata);

    char outname[50];
    strcpy(outname, entry->fname);

    // Fit the files
    switch (entry->type)
    {
    case 1:
        strcat(outname, ".snx");
        for (int i = 0; i < entry->length; ++i) buff[i] ^= key; // Decrypt
        break;
    case 2:
        strcat(outname, ".bmp");
        break;
    case 3:
        strcat(outname, ".png");
        break;
    case 4:
        strcat(outname, ".wav");
        break;
    case 5:
        strcat(outname, ".ogg");
        break;
    default:
        strcat(outname, ".dat");
        break;
    }

    FILE* fo = fopen(outname, "wb+");
    if (fo == nullptr) throw nullptr;
    fwrite(buff, 1, entry->length, fo);
    fclose(fo);

    delete[] buff;
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        if (argc < 2)
        {
            printf("usage: %s <infile> [outdir]\n", argv[0]);
            return 0;
        }

        /// Prepare the file list
        wchar_t* lstfn = new wchar_t[wcslen(argv[1]) + 5];
        wcscpy(lstfn, argv[1]);
        wcscat(lstfn, L".lst");

        FILE* flst = _wfopen(lstfn, L"rb");
        if (flst == nullptr) throw nullptr;

        delete[] lstfn;

        fseek(flst, 0, SEEK_END);
        int lstsiz = ftell(flst);
        if (lstsiz < 4) throw nullptr; // File has a wrong length

        fseek(flst, 0, SEEK_SET);
        char* list = new char[lstsiz];
        fread(list, 1, lstsiz, flst);
        fclose(flst);

        Header_t header;
        memcpy(&header, list, sizeof(Header_t)); // Should not modify the "list"

        memset(header.key, header.key[3], 4); // Get the XOR value automatically
        //memset(header.key, 0x01, 4); // Set the XOR value manually

        header.entry_count ^= *(long*)header.key;

        /// Now export the files
        FILE* fdata = _wfopen(argv[1], L"rb");
        if (fdata == nullptr) throw nullptr;

        if (argc == 3)
        {
            _wmkdir(argv[2]);
            _wchdir(argv[2]);
        }

        Entry_t* entry = (Entry_t*)(list + 4);

        for (int i = 0; i < header.entry_count; ++i)
        {
            int x = 0;
            while (entry[i].fname[x]) entry[i].fname[x++] ^= header.key[0];
            entry[i].length ^= *(long*)header.key;
            entry[i].offset ^= *(long*)header.key;

            export_file(fdata, entry + i, header.key[0] + 1);
            // export_file(fdata, entry, 0x02); // Set the XOR value manually
        }

        fclose(fdata);
        delete[] list;
        return 0;
    }
    catch (...)
    {
        printf("error.\n");
        return 1;
    }
}
