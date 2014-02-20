
#include "stdafx.h"
#include "CQ_Text.h"

void write_data(const wchar_t* fn, const void* buf, const size_t len, const std::wstring& dir_n)
{
    std::wstring fout_name;
    FILE* fout;
    fout_name = dir_n;
    fout_name += fn;
    _wfopen_s(&fout, fout_name.c_str(), L"wb+");
    if(!fout) throw 1;
    fwrite(buf, 1, len, fout);
    fclose(fout);
}

void process(unsigned char* buf, const long len, const std::wstring& dir_n)
{
    HEADER* hdr = (HEADER*) buf;
    ENTRY_NAME* entry_name = (ENTRY_NAME*)(buf + hdr->index_offset + 4); // 跳过 4 个 0
    const int entry_count = hdr->name_table_size / sizeof(ENTRY_NAME);
    ENTRY_INFO* entry_info = (ENTRY_INFO*)(entry_name + entry_count);
    ++entry_info; // 跳过空信息

    write_data(L"__header", buf, sizeof(HEADER), dir_n);
    write_data(L"__index", buf + hdr->index_offset, hdr->index_length_ex, dir_n);

    wchar_t fout_name[256];
    unsigned char* praw_zero = buf + hdr->header_lenth;
    unsigned char* praw;
    unsigned char* pout;

    for(int i = 0; i < entry_count; ++i, ++entry_name, ++entry_info)
    {
        praw = praw_zero + entry_info->file_nohdr_off;
        MultiByteToWideChar(
            hdr->charset,
            0,
            entry_name->lower_name,
            -1,
            fout_name,
            MultiByteToWideChar(hdr->charset, 0, entry_name->lower_name, -1, 0, 0)
            );
        if(entry_info->file_inpack_sz == -1)
        {
            write_data(fout_name, praw, entry_info->file_uncprs_sz, dir_n);
        }
        else
        {
            pout = new unsigned char[MEMPRTCT(entry_info->file_uncprs_sz)];
            uncompress(pout, praw);
            write_data(fout_name, pout, entry_info->file_uncprs_sz, dir_n);
            delete [] pout;
        }
    }
}