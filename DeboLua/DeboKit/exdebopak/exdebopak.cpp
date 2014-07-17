// exdebopak.cpp, v1.02 2010/02/01
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts PAK (*.pak) archives used by でぼの巣製作所 in 神楽道中記.

// v2.00 2014/7/17
// modified by Fuyin
// now it can rebuild the PAK (*.pak) archives.

#include "as-util.h"
#include "zlib.h"
#include <limits>
//#include <Windows.h>

#define Z_DEFAULT_MEMLEVEL 8


struct PAKHDR {
  unsigned char  signature[4]; // "PAK"
  unsigned short hdr1_length;
  unsigned short unknown1;
  unsigned long  unknown2;
  unsigned long  unknown3;
  unsigned long  hdr2_length;
  unsigned long  unknown5;
  unsigned long  unknown6;
  unsigned long  original_toc_length;
  unsigned long  toc_length;
  unsigned long  unknown7;
};

struct PAKENTRY {
  unsigned long offset;
  unsigned long unknown1;
  unsigned long original_length;
  unsigned long unknown2;
  unsigned long length;
  unsigned long unknown3;
  unsigned long flags;
  unsigned long unknown5;

  unsigned long unknown6;
  unsigned long unknown7;
  unsigned long unknown8;
  unsigned long unknown9;
  unsigned long unknown10;
};

bool rebuild = false;

unsigned long inflate_raw(unsigned char* buff, 
                          unsigned long  len, 
                          unsigned char* out_buff, 
                          unsigned long  out_len)
{
  z_stream stream;

  stream.zalloc    = (alloc_func)0;
  stream.zfree     = (free_func)0;
  stream.opaque    = (voidpf)0;
  stream.next_in   = buff;
  stream.avail_in  = len;
  stream.next_out  = out_buff;
  stream.avail_out = out_len; 
     
  if (inflateInit2(&stream, -MAX_WBITS) >= 0) 
  {  
    inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
  }

  return stream.total_out;
}

unsigned long memBound(unsigned char* buff, unsigned long  len)
{
	z_stream stream;
	unsigned long bound = 0;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	stream.next_in = buff;
	stream.avail_in = len;

	if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, Z_DEFAULT_MEMLEVEL, Z_DEFAULT_STRATEGY) >= 0)
	{
		bound = deflateBound(&stream, len);
	}

	return bound;
}

unsigned long deflate_raw(unsigned char* buff,
	unsigned long  len,
	unsigned char* out_buff,
	unsigned long  out_len)
{
	z_stream stream;

	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	stream.next_in = buff;
	stream.avail_in = len;
	stream.next_out = out_buff;
	stream.avail_out = out_len;
	
	if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, Z_DEFAULT_MEMLEVEL, Z_DEFAULT_STRATEGY) >= 0)
	{
		deflate(&stream, Z_FINISH);
		deflateEnd(&stream);
	}

	return stream.total_out;
}

void process_dir(int fd,
				int fout,
				unsigned long   base,
				unsigned long   newbase,
				unsigned char*& toc,
				unsigned char*  toc_end,
				unsigned long   entry_count,
				const string&   path)
{
	while (toc < toc_end && entry_count--)
	{
		PAKENTRY* entry = (PAKENTRY*)toc;
		toc += sizeof(*entry);

		char* filename = (char*)toc;
		toc += strlen(filename) + 1;

		string full_name = path + filename;

		if (entry->flags & 0xA0)
		{
			if (rebuild)
			{
				string cur_filename = full_name;
				int fcur = as::open_or_die(cur_filename, O_RDONLY | O_BINARY);

				unsigned long len = as::get_file_size(fcur);
				unsigned char *buff = new unsigned char[len];
				read(fcur, buff, len);

				unsigned long complen = memBound(buff, len);
				unsigned char* compbuff = new unsigned char[complen];
				complen = deflate_raw(buff, len, compbuff, complen);

				//修正索引
				entry->original_length = len;
				entry->length = complen;
				entry->offset = tell(fout) - newbase;

				write(fout, compbuff, complen);

				delete[] compbuff;
				delete[] buff;
			}
			else
			{
				unsigned long  len = entry->length;
				unsigned char* buff = new unsigned char[len];
				lseek(fd, base + entry->offset, SEEK_SET);
				read(fd, buff, len);

				unsigned long  out_len = entry->original_length;
				unsigned char* out_buff = new unsigned char[out_len];
				inflate_raw(buff, len, out_buff, out_len);

				as::make_path(full_name);
				as::write_file(full_name, out_buff, out_len);

				delete[] out_buff;
				delete[] buff;
			}

		}
		else
		{
			process_dir(fd, fout, base, newbase, toc, toc_end, entry->original_length, full_name + "\\");
		}
	}
}

int main(int argc, char** argv)
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "exdebopak v2.00, coded by asmodean & modified by Fuyin\n\n");
		fprintf(stderr, "usage: %s <input.pak> [output.pak]\n", argv[0]);
		return -1;
	}

	if (argc == 3)
		rebuild = true;


	string in_filename(argv[1]);
	string out_filename;
	if (rebuild)
		out_filename = argv[2];

	int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);
	int fout = -1;

	PAKHDR hdr;
	read(fd, &hdr, sizeof(hdr));

	if (rebuild)
	{
		fout = as::open_or_die(out_filename, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY);
	}

	unsigned long  toc_len = hdr.toc_length;
	unsigned char* toc_buff = new unsigned char[toc_len];
	read(fd, toc_buff, toc_len);

	unsigned long  out_toc_len = hdr.original_toc_length;
	unsigned char* out_toc_buff = new unsigned char[out_toc_len];
	inflate_raw(toc_buff, toc_len, out_toc_buff, out_toc_len);

	unsigned char* entry = out_toc_buff;
	unsigned long entry_len = out_toc_len;

	//考虑到压缩后的索引区可能比原来大，这里预留足够的空间
	unsigned long newbase = 0;
	if (rebuild)
	{
		newbase = sizeof(hdr) + out_toc_len;
		lseek(fout, newbase, SEEK_SET);
	}

	process_dir(fd,
				fout,
				sizeof(hdr)+toc_len,
				newbase,
				entry,
				entry + entry_len,
				std::numeric_limits<unsigned long>::max(),
				"");


	if (rebuild)
	{
		//直接以解压后的索引大小分配内存
		unsigned char* entry_buff = new unsigned char[out_toc_len];

		//压缩
		unsigned long entry_size = deflate_raw(out_toc_buff, out_toc_len, entry_buff, out_toc_len);

		//多余部分置零
		memset(entry_buff + entry_size, 0, out_toc_len - entry_size);

		//修正文件头
		hdr.toc_length = out_toc_len;

		lseek(fout, 0, SEEK_SET);
		write(fout, &hdr, sizeof(hdr));
		write(fout, entry_buff, out_toc_len);

		delete[] entry_buff;
	}
	

	delete[]out_toc_buff;
	delete[]toc_buff;

	return 0;
}