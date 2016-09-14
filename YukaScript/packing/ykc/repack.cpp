
#include "stdafx.h"
#include "funcs.h"

namespace fs = boost::filesystem;
namespace algo = boost::algorithm;

struct FileEntry
{
    std::string path;
    char* buff = 0;
    int length;
};
std::vector<FileEntry> filelist;
fs::path g_root;

int read_file(const fs::path& path, char** buff)
{
    FILE* fp = 0;
    _wfopen_s(&fp, path.c_str(), L"rb");
    if (fp == 0)
    {
        wprintf_s(L"cannot open and read: %s\n", path.c_str());
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    *buff = new char[length];
    fseek(fp, 0, SEEK_SET);
    fread(*buff, 1, length, fp);
    fclose(fp);


    return length;
}

void make_pic(const fs::path& path)
{
    char* pngbuff = 0;
    int pnglen = 0;
    char* mskbuff = 0;
    int msklen = 0;
    char* lyrbuff = 0;
    int lyrlen = 0;

    auto curname = path.filename().replace_extension();
    auto rawname = path;
    rawname.append(curname.c_str());
    auto pngname = rawname;
    pngname += ".png";
    auto mskname = rawname;
    mskname += ".msk";
    auto lyrname = rawname;
    lyrname += ".lyr";

    if (fs::exists(pngname))
    {
        pnglen = read_file(pngname, &pngbuff);

        unsigned long* sig = (unsigned long*)(pngbuff);
        if (*sig == 0x474E5089) // PNG\x89
        {
            *sig = 0x504E4789; // GNP\x89
        }
    }
    if (fs::exists(mskname))
    {
        msklen = read_file(mskname, &mskbuff);

        unsigned long* sig = (unsigned long*)(mskbuff);
        if (*sig == 0x474E5089) // PNG\x89
        {
            *sig = 0x504E4789; // GNP\x89
        }
    }
    if (fs::exists(lyrname))
    {
        lyrlen = read_file(lyrname, &lyrbuff);
        
        unsigned long* sig = (unsigned long*)(lyrbuff);
        if (*sig == 0x474E5089) // PNG\x89
        {
            *sig = 0x504E4789; // GNP\x89
        }
    }

    FileEntry ent;
    ent.path = algo::ireplace_first_copy(path.string(), g_root.string(), L"");
    ent.length = sizeof(YKGHDR) + pnglen + msklen + lyrlen;
    ent.buff = new char[ent.length];

    YKGHDR ghdr = { 0 };
    memcpy_s(ghdr.signature, 8, "YKG000", 6);
    ghdr.hdr_length = sizeof(YKGHDR);
    if (pnglen)
    {
        ghdr.rgb_offset = ghdr.hdr_length;
        ghdr.rgb_length = pnglen;
        memcpy_s(ent.buff + ghdr.rgb_offset, ent.length - ghdr.rgb_offset, pngbuff, pnglen);
    }
    if (msklen)
    {
        ghdr.msk_offset = ghdr.hdr_length + ghdr.rgb_length;
        ghdr.msk_length = msklen;
        memcpy_s(ent.buff + ghdr.msk_offset, ent.length - ghdr.msk_offset, mskbuff, msklen);
    }
    if (lyrlen)
    {
        ghdr.__lyr_off = ghdr.hdr_length + ghdr.rgb_length + ghdr.msk_length;
        ghdr.__lyr_len = lyrlen;
        memcpy_s(ent.buff + ghdr.__lyr_off, ent.length - ghdr.__lyr_off, lyrbuff, lyrlen);
    }
    memcpy_s(ent.buff, ent.length, &ghdr, sizeof(YKGHDR));

    filelist.push_back(ent);
}

void make_single(const fs::path& path)
{
    FileEntry ent;
    ent.path = algo::ireplace_first_copy(path.string(), g_root.string(), L"");
    ent.length = read_file(path, &(ent.buff));

    if (ent.length != -1)
    {
        filelist.push_back(ent);
    }
}


void repack_rec(const fs::path& root)
{
    fs::directory_iterator end_iter;
    for (fs::directory_iterator dir_iter(root); dir_iter != end_iter; ++dir_iter)
    {
        if (fs::is_directory(dir_iter->status()) && !fs::is_empty(*dir_iter))
        {
            if (_wcsicmp(dir_iter->path().extension().c_str(), L".ykg") == 0)
            {
                make_pic(*dir_iter);
            }
            else
            {
                repack_rec(*dir_iter);
            }
        }
        else if (fs::is_regular_file(dir_iter->status()))
        {
            make_single(*dir_iter);
        }
    }
}

int repack(const wchar_t* path)
{
    g_root = path;
    g_root += "\\";
    repack_rec(g_root);

    const std::vector<FileEntry>& const_list = filelist;
    g_root = path;
    g_root += ".ykc";

    FILE* fp = 0;
    _wfopen_s(&fp, g_root.c_str(), L"wb+");

    YKCHDR hdr = { 0 };
    memcpy_s(hdr.signature, 8, "YKC001", 6);
    hdr.__product = 0x18;

    fwrite(&hdr, sizeof(YKCHDR), 1, fp);
    int data_beg = ftell(fp);

    for (const auto& ent : const_list)
    {
        fwrite(ent.buff, 1, ent.length, fp);
    }
    int filename_beg = ftell(fp);
    for (const auto& ent : const_list)
    {
        fwrite(ent.path.c_str(), 1, ent.path.length(), fp);
        fputc(0, fp);
    }
    int table_beg = ftell(fp);

    YKCENTRY* table = new YKCENTRY[const_list.size()];
    memset(table, 0, sizeof(YKCENTRY) * const_list.size());

    table->data_offset = data_beg;
    table->data_length = const_list[0].length;
    for (int i = 1; i < (int)const_list.size(); ++i)
    {
        table[i].data_length = const_list[i].length;
        table[i].data_offset = table[i - 1].data_offset + table[i - 1].data_length;
    }
    table->filename_offset = filename_beg;
    table->filename_length = const_list[0].path.length() + 1;
    for (int i = 1; i < (int)const_list.size(); ++i)
    {
        table[i].filename_length = const_list[i].path.length() + 1;
        table[i].filename_offset = table[i - 1].filename_offset + table[i - 1].filename_length;
    }

    fwrite(table, sizeof(YKCENTRY), const_list.size(), fp);

    fseek(fp, 0, SEEK_SET);

    hdr.toc_length = sizeof(YKCENTRY) * const_list.size();
    hdr.toc_offset = table_beg;
    fwrite(&hdr, sizeof(YKCHDR), 1, fp);

    fclose(fp);

    for (auto&& ent : filelist)
    {
        delete[] ent.buff;
    }

    return 0;
}
