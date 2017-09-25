// exiga.cpp, v1.0 2011/05/20
// coded by asmodean

// contact:
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// Modified by Fuyin
// 2014/09/07

// Modified again by Neroldy
// 2016/06/25
// 2017/09/25

// This tool extracts and rebuild IGA0 (*.iga) archives.
// If the iga file is large, such as bgimage.iga, please compile 64bit version.

#include <iostream>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <algorithm>
#include <vector>
#include <windows.h>
using namespace std;

struct IGAHDR
{
    char signature[4];       // "IGA0"
    unsigned long unknown1;  // it seems like a checksum
    unsigned long unknown2;  // 02 00 00 00
    unsigned long unknown3;  // 02 00 00 00
};

struct IGAENTRY
{
    unsigned long filename_offset;
    unsigned long offset;
    unsigned long length;
};

// 0F4FAEA0    0FB655 00       movzx   edx, byte ptr[ebp]; ebp = src
// 0F4FAEA4    C1E3 07         shl     ebx, 0x7
// 0F4FAEA7    0BDA            or      ebx, edx
// 0F4FAEA9    F6C3 01         test    bl, 0x1
unsigned long get_multibyte_long(unsigned char*& buff)
{
    unsigned long v = 0;

    while (!(v & 1))
    {
        v = (v << 7) | *buff++;
    }

    return v >> 1;
}

vector<unsigned char> cal_multibyte(unsigned long val)
{
    vector<unsigned char> rst;
    unsigned long long v = val;

    if (v == 0)
    {
        rst.push_back(0x01);
        return rst;
    }

    v = (v << 1) + 1;
    unsigned char cur_byte = v & 0xFF;
    while (v & 0xFFFFFFFFFFFFFFFE)
    {
        rst.push_back(cur_byte);
        v >>= 7;
        cur_byte = v & 0xFE;
    }

    reverse(rst.begin(), rst.end());
    return rst;
}

// get folder files list in pack mode
void get_foler_files(string path, vector<string>& files)
{
    // please use long long to prevent memory crash in 64bit
    long long hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                {
                    files.push_back(p.assign(path).append("\\").append(fileinfo.name));
                    get_foler_files(p.assign(path).append("\\").append(fileinfo.name), files);
                }
            }
            else
            {
                files.push_back(fileinfo.name);
            }

        } while (_findnext(hFile, &fileinfo) == 0);

        _findclose(hFile);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "igatool v1.2 by asmodean & modified by Fuyin & modified again by Neroldy\n");
        fprintf(stderr, "usage(unpack mode): %s <input.iga> [-x]\n", argv[0]);
        fprintf(stderr, "example: igatool.exe fgimage.iga\n");
        fprintf(stderr, "usage(pack mode): %s <inputfolder> <output.iga> [-x]\n", argv[0]);
        fprintf(stderr, "example: igatool.exe script_folder script.iga -x\n");
        fprintf(stderr, "option [-x] is for script xor encrypt or decrypt\n");
        return -1;
    }
    // xor_flag for script encrypt or decrypt
    bool xor_flag = false;
    string in_filename(argv[1]);
    string in_foldername;
    string out_filename;
    int fd = -1;
    int fout = -1;
    vector<string> in_fileslist;
    if (argc == 2 || (argc == 3 && strcmp(argv[2], "-x") == 0))  // read
    {
        if (argc == 3) xor_flag = true;
        fd = _open(in_filename.c_str(), O_RDONLY | O_BINARY);
        IGAHDR hdr;
        _read(fd, &hdr, sizeof(hdr));
        // "big enough" and read the iga file entry block
        unsigned long toc_len = 1024 * 1024 * 5;
        unsigned char* toc_buff = new unsigned char[toc_len];
        _read(fd, toc_buff, toc_len);

        unsigned char* p = toc_buff;
        unsigned long entries_len = get_multibyte_long(p);
        unsigned char* end = p + entries_len;
        typedef vector<IGAENTRY> entries_t;
        entries_t entries;

        while (p < end)
        {
            IGAENTRY entry;

            entry.filename_offset = get_multibyte_long(p);
            entry.offset = get_multibyte_long(p);
            entry.length = get_multibyte_long(p);

            entries.push_back(entry);
        }

        unsigned long filenames_len = get_multibyte_long(p);
        unsigned long data_base = _filelength(fd) - (entries.rbegin()->offset + entries.rbegin()->length);

        // create new folder which has the same name as the iga file and put the file in it
        string out_folder = in_filename.substr(0, in_filename.length() - 4);
        CreateDirectoryA(out_folder.c_str(), NULL);

        for (auto i = entries.begin(); i != entries.end(); ++i)
        {
            auto next = i + 1;
            unsigned long name_len =
                (next == entries.end() ? filenames_len : next->filename_offset) - i->filename_offset;

            string filename;

            while (name_len--)
            {
                filename += (char)get_multibyte_long(p);
            }

            unsigned long len = i->length;
            unsigned char* buff = new unsigned char[len];
            _lseek(fd, data_base + i->offset, SEEK_SET);
            _read(fd, buff, len);

            for (unsigned long j = 0; j < len; j++)
            {
                buff[j] ^= (unsigned char)(j + 2);
                if (xor_flag) buff[j] ^= 0xFF;
            }
            string outputfile = out_folder + "/" + filename;
            int file = _open(outputfile.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY);
            _write(file, buff, len);
            _close(file);

            delete[] buff;
        }
        delete[] toc_buff;
        _close(fd);
    }
    else  // write
    {
        if (argc == 4 && strcmp(argv[3], "-x") == 0) xor_flag = true;
        in_foldername = argv[1];
        out_filename = argv[2];
        get_foler_files(in_foldername, in_fileslist);
        // no input file in the folder
        if (in_fileslist.size() == 0)
        {
            fprintf(stderr, "folder is empty!\n");
            return -1;
        }
        fout = _open(out_filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY);
        // construct IGA header
        IGAHDR hdr;
        strcpy(hdr.signature, "IGA0");
        hdr.unknown1 = 0x00;
        hdr.unknown2 = 0x02;
        hdr.unknown3 = 0x02;
        _write(fout, &hdr, sizeof(hdr));
        vector<unsigned char> index;  // entries for output file
        unsigned long long data_size = 0;
        vector<unsigned char> data;  // data buffer for output file
        vector<unsigned char> filename_block;
        for (auto i = in_fileslist.begin(); i != in_fileslist.end(); ++i)
        {
            string filename = *i;
            string in_file = in_foldername + "/" + filename;
            int fcur = _open(in_file.c_str(), O_RDONLY | O_BINARY);
            unsigned long len = _filelength(fcur);
            unsigned char* temp = new unsigned char[len];
            static unsigned long long pos = 0;
            static unsigned long long filename_offset = 0;

            data_size += len;
            _read(fcur, temp, len);

            for (unsigned long j = 0; j < len; j++)
            {
                if (xor_flag) temp[j] ^= 0xFF;
                temp[j] ^= (unsigned char)(j + 2);
                data.push_back(temp[j]);
            }
            delete[] temp;

            unsigned int length = filename.length();
            for (unsigned int j = 0; j < length; ++j)
            {
                vector<unsigned char> vfilename = cal_multibyte(filename[j]);
                filename_block.insert(filename_block.end(), vfilename.begin(), vfilename.end());
            }

            vector<unsigned char> vidx = cal_multibyte(filename_offset);
            vector<unsigned char> voffset = cal_multibyte(pos);
            vector<unsigned char> vlength = cal_multibyte(len);
            index.insert(index.end(), vidx.begin(), vidx.end());
            index.insert(index.end(), voffset.begin(), voffset.end());
            index.insert(index.end(), vlength.begin(), vlength.end());

            pos += len;
            filename_offset += filename.length();
            _close(fcur);
        }

        vector<unsigned char> vidx_len = cal_multibyte(index.size());
        _write(fout, &vidx_len[0], vidx_len.size());
        _write(fout, &index[0], index.size());

        vector<unsigned char> filename_block_len = cal_multibyte(filename_block.size());
        _write(fout, &filename_block_len[0], filename_block_len.size());
        _write(fout, &filename_block[0], filename_block.size());
        _write(fout, data.data(), data_size);

        _close(fout);
    }

    return 0;
}
