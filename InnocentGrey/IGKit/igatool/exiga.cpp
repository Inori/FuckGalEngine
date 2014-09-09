// exiga.cpp, v1.0 2011/05/20
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// Modified by Fuyin
// 2014/09/07

// This tool extracts and rebuild IGA0 (*.iga) archives.

#include "as-util.h"
#include <algorithm>
#include <vector>
using std::vector;

struct IGAHDR {
  unsigned char signature[4]; // "IGA0"
  unsigned long unknown1;
  unsigned long unknown2;
  unsigned long unknown3;
};

struct IGAENTRY {
  unsigned long filename_offset;
  unsigned long offset;
  unsigned long length;
};


//0F4FAEA0    0FB655 00       movzx   edx, byte ptr[ebp]; ebp = src
//0F4FAEA4    C1E3 07         shl     ebx, 0x7
//0F4FAEA7    0BDA            or      ebx, edx
//0F4FAEA9    F6C3 01         test    bl, 0x1
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

int main(int argc, char** argv)
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "igatool v1.0 by asmodean & modified by Fuyin\n\n");
		fprintf(stderr, "usage: %s <input.iga> [output.iga]\n", argv[0]);
		return -1;
	}

	bool doRebuild = false;

	string in_filename(argv[1]);
	string out_filename;
	if (argc == 3)
	{
		doRebuild = true;
		out_filename = argv[2];
	}


	int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);
	int fout = -1;

	if (doRebuild)
		fout = as::open_or_die(out_filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY);

	IGAHDR hdr;
	read(fd, &hdr, sizeof(hdr));

	if (doRebuild)
		write(fout, &hdr, sizeof(hdr));

	// "big enough"
	unsigned long  toc_len = 1024 * 1024 * 5;
	unsigned char* toc_buff = new unsigned char[toc_len];
	read(fd, toc_buff, toc_len);

	unsigned char* p = toc_buff;
	unsigned long  entries_len = get_multibyte_long(p);
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

	//we don't need to modify the filename block
	unsigned char *filename_block = p;
	unsigned long filenames_len = get_multibyte_long(p);
	unsigned long data_base = as::get_file_size(fd) - (entries.rbegin()->offset + entries.rbegin()->length);

	
	unsigned long filename_block_size = (p - filename_block) + filenames_len;

	vector<unsigned char> index; // entries for output file
	index.reserve(entries_len);  //faster

	unsigned long data_size = 0;
	unsigned char *data = (unsigned char*)malloc(0); //data buffer for output file
	for (auto i = entries.begin(); i != entries.end(); ++i)
	{
		auto next = i + 1;
		unsigned long name_len = (next == entries.end() ? filenames_len : next->filename_offset) - i->filename_offset;

		string filename;

		while (name_len--)
		{
			filename += (char)get_multibyte_long(p);
		}

		if (doRebuild)
		{
			int fcur = as::open_or_die(filename, O_RDONLY | O_BINARY);
			unsigned long len = as::get_file_size(fcur);
			static unsigned long pos = 0;

			data_size += len;
			data = (unsigned char*)realloc(data, data_size);
			read(fcur, data + pos, len);

			for (unsigned long j = 0; j < len; j++)
			{
				(data + pos)[j] ^= (unsigned char)(j + 2);
			}

			vector<unsigned char> vidx = cal_multibyte(i->filename_offset);
			vector<unsigned char> voffset = cal_multibyte(pos);
			vector<unsigned char> vlength = cal_multibyte(len);
			index.insert(index.end(), vidx.begin(), vidx.end());
			index.insert(index.end(), voffset.begin(), voffset.end());
			index.insert(index.end(), vlength.begin(), vlength.end());

			pos += len;
			close(fcur);
		}
		else
		{
			unsigned long  len = i->length;
			unsigned char* buff = new unsigned char[len];
			lseek(fd, data_base + i->offset, SEEK_SET);
			read(fd, buff, len);

			for (unsigned long j = 0; j < len; j++)
			{
				buff[j] ^= (unsigned char)(j + 2);
			}

			as::write_file(filename, buff, len);

			delete[] buff;
		}
	
	}

	if (doRebuild)
	{
		vector<unsigned char> vidx_len = cal_multibyte(index.size());
		write(fout, &vidx_len[0], vidx_len.size());

		write(fout, &index[0], index.size());
		write(fout, filename_block, filename_block_size);
		write(fout, data, data_size);

		close(fout);
	}

	delete[] toc_buff;
	free(data);
	close(fd);

	return 0;
}
