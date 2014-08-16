// exafa.cpp, v1.02 2013/02/16
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts AFAH (*.afa) archives.

//#error AFF support not finished

#include "as-util.h"
#include "zlib.h" 

//#define AFA_VERSION 1

// Since パステルチャイム３ (2012/02/15)
#define AFA_VERSION 2

struct AFAHDR1 {
  unsigned char signature[4];  // "AFAH"
  unsigned long length;
  unsigned char signature2[8]; // "AliceArch"
  unsigned long unknown2;      // version?
  unsigned long unknown3;
  unsigned long data_base;
};

struct AFAHDR2 {
  unsigned char signature[4]; // "INFO"
  unsigned long toc_length;
  unsigned long original_toc_length;
  unsigned long entry_count;
};

struct AFAENTRY1 {
  unsigned long filename_length;
  unsigned long filename_length_padded;  
};

struct AFAENTRY2 {
  unsigned long unknown1;
  unsigned long unknown2;
#if AFA_VERSION == 1
  unsigned long unknown3;
#endif
  unsigned long offset;
  unsigned long length;
};

struct AFFHDR {
  unsigned char signature[4]; // "AFF"
  unsigned long unknown1;
  unsigned long length;
  unsigned long unknown2; // 1234
};

struct CWSHDR {
  unsigned char signature[4]; // "CWS\x0A"
  unsigned long original_length;
};

void proc_aff(const string&  filename,
              unsigned char* buff,
              unsigned long  len)
{
  static const unsigned char KEY[] = { 0xC8, 0xBB, 0x8F, 0xB7, 0xED, 0x43, 0x99, 0x4A, 
                                       0xA2, 0x7E, 0x5B, 0xB0, 0x68, 0x18, 0xF8, 0x88 };

  AFFHDR* hdr = (AFFHDR*) buff;

  unsigned long  data_len  = len - sizeof(*hdr);
  unsigned char* data_buff = (unsigned char*) (hdr + 1);

  unsigned long unobfuscate_len = std::min(data_len, 64UL);

  for (unsigned long i = 0; i < unobfuscate_len; i++) {
    data_buff[i] ^= KEY[i % sizeof(KEY)];
  }

  string out_filename = filename;
  string ext = "bin";//as::guess_file_extension(data_buff, data_len);

  if (!ext.empty()) {
    out_filename = as::get_file_prefix(filename) + ext;
  }

  as::write_file(out_filename, data_buff, data_len);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "exafa v1.02, coded by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.afa>\n", argv[0]);
    return -1;
  }

  string in_filename(argv[1]);

  int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);

  AFAHDR1 hdr1;
  read(fd, &hdr1, sizeof(hdr1));

  AFAHDR2 hdr2;
  read(fd, &hdr2, sizeof(hdr2));

  unsigned long  toc_len  = hdr2.toc_length;
  unsigned char* toc_buff = new unsigned char[toc_len];
  read(fd, toc_buff, toc_len);

  unsigned long  out_toc_len  = hdr2.original_toc_length;
  unsigned char* out_toc_buff = new unsigned char[out_toc_len];
  uncompress(out_toc_buff, &out_toc_len, toc_buff, toc_len);

  unsigned char* p = out_toc_buff;

  for (unsigned long i = 0; i < hdr2.entry_count; i++) {
    AFAENTRY1* entry1 = (AFAENTRY1*) p;
    p += sizeof(*entry1);

    string filename((char*)p, entry1->filename_length);
    p += entry1->filename_length_padded;

    AFAENTRY2* entry2 = (AFAENTRY2*) p;
    p += sizeof(*entry2);

    unsigned long  len  = entry2->length;
    unsigned char* buff = new unsigned char[len];
    lseek(fd, hdr1.data_base + entry2->offset, SEEK_SET);
    read(fd, buff, len);

    as::make_path(filename);

    if (!memcmp(buff, "AFF", 3)) {
      proc_aff(filename, buff, len);
    } else {
      as::write_file(filename, buff, len);
    }

    delete [] buff;
  }

  delete [] out_toc_buff;
  delete [] toc_buff;

  close(fd);

  return 0;
}