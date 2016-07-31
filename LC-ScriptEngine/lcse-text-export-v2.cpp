
#include "stdafx.h"

#define METHOD 2

#pragma pack(1)

struct Header_t
{
    int func_entry_cnt;
    int text_block_len; // Important when repacking text.
};

struct FuncEntry_t
{
    int func1; // All unknown.
    int func2;
    int func3;
};

#pragma pack()


int wmain(int argc, wchar_t** argv)
{
    if (argc != 3
#if METHOD == 2
        && argc != 4
#endif
        )
    {
        printf("Usage: {program} <snx_file> <text_file>"
#if METHOD == 2
            " <func_file>"
#endif
            "\n");
        return 0;
    }


    // Read file into memory.
    FILE* fp = _wfopen(argv[1], L"rb");
    fseek(fp, 0, SEEK_END);
    int fsiz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = new char[fsiz];
    fread(buf, fsiz, 1, fp);
    fclose(fp);

    // Find where the text block is.
    Header_t* pHdr = (Header_t*)buf;

#if METHOD == 2
    // Export the func.
    FILE* ff = _wfopen(argv[3], L"w");
    FuncEntry_t* func = (FuncEntry_t*)(pHdr + 1);
    for (int i = 0; i < pHdr->func_entry_cnt; ++i)
    {
        fprintf(ff, "%04X: %08X %08X %08X\n", i, func[i].func1, func[i].func2, func[i].func3);
    }
#endif

    char* pText = (char*)((FuncEntry_t*)(pHdr + 1) + pHdr->func_entry_cnt);
    if (fsiz - (pText - buf) != pHdr->text_block_len)
    {
        printf("Incorrect file format.\n");
        exit(1);
    }

    // Export the text.
    FILE* fo = _wfopen(argv[2], L"w");

    char* anchor = pText;
    char output[1000];
    char temp[6];

    for (int i = 1; anchor < buf + fsiz; ++i)
    {
        int str_length = *(int*)anchor;
        anchor += 4; // 32-bit only.

        output[0] = 0; // Clean.

                       // Filt unreadable chars.
                       // But text can auto wrap in game. So you can ignore \x01 inside the sentence.
        for (int j = 0; j < str_length - 1; ++j)
        {
            if (anchor[j] > 0 && anchor[j] < 0x20)
                sprintf(temp, "<%02x>", anchor[j]);
            else
                sprintf(temp, "%c", anchor[j]);
            strcat(output, temp);
        }

        // Output.
        fprintf(fo, "# %04d @ %06x ^ %03x\n%s\n\n", i, anchor - buf, str_length - 1, output);

        anchor += str_length;
    }

    fclose(fo);

    delete[] buf;
    return 0;
}

