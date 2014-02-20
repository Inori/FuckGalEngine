// exoozoarc.cpp, v1.0 2012/05/25
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts *.arc archives used by Ç±ÇÃëÂãÛÇ…ÅAóÉÇÇ–ÇÎÇ∞Çƒ.

#include "as-util.h"

struct ARCHDR {
  unsigned long entry_count;
  unsigned long toc_length;
};

struct ARCENTRY {
  unsigned long length;
  unsigned long offset;
  //wchar_t       filename[1];
};

struct PNAPHDR {
  unsigned char signature[4]; // "PNAP"
  unsigned long unknown1;
  unsigned long width;
  unsigned long height;
  unsigned long entry_count;
};

struct PNAPENTRY {
  unsigned long unknown1;
  unsigned long index;
  unsigned long offset_x;
  unsigned long offset_y;
  unsigned long width;
  unsigned long height;
  unsigned long unknown2;
  unsigned long unknown3;
  unsigned long unknown4;
  unsigned long length;
};

bool process_pnap(const string&  filename,
                  unsigned char* buff, 
                  unsigned long len)
{
  if (len < 4 || memcmp(buff, "PNAP", 4)) {
    return false;
  }

  string prefix = as::get_file_prefix(filename) + "+PNAP";

  FILE* fh = as::open_or_die_file(prefix + "+layers.txt", "w");

  PNAPHDR*       hdr     = (PNAPHDR*) buff;
  PNAPENTRY*     entries = (PNAPENTRY*) (hdr + 1);
  unsigned char* p       = (unsigned char*) (entries + hdr->entry_count);

  fprintf(fh, "%d %d\n", hdr->width, hdr->height);

  for (unsigned long i = 0; i < hdr->entry_count; i++) {
    if (!entries[i].length) {
      continue;
    }

    string out_filename = prefix + as::stringf("+%03d", i) + as::guess_file_extension(p, entries[i].length);    

    fprintf(fh, 
            "%d \t%d \t%d \t%d \t%d \t%s\n", 
            entries[i].index,
            entries[i].offset_x,
            entries[i].offset_y,
            entries[i].width,
            entries[i].height,
            out_filename.c_str());

    as::write_file(out_filename, p, entries[i].length);

    p += entries[i].length;
  }

  fclose(fh);

  return true;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "exoozoarc v1.0 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.arc>\n", argv[0]);

    return -1;
  }

  string in_filename(argv[1]);

  int fd = as::open_or_die(in_filename, O_RDONLY | O_BINARY);

  ARCHDR hdr;
  read(fd, &hdr, sizeof(hdr));

  unsigned long  toc_len  = hdr.toc_length;
  unsigned char* toc_buff = new unsigned char[toc_len];
  read(fd, toc_buff, toc_len);

  unsigned long  data_base = sizeof(hdr) + toc_len;
  unsigned char* p         = toc_buff;

  for (unsigned long i = 0; i < hdr.entry_count; i++) {
    ARCENTRY* entry = (ARCENTRY*) p;
    p += sizeof(*entry);

    wchar_t* filename_wc = (wchar_t*)p;
    string   filename    = as::convert_wchar(filename_wc);
    p += (wcslen(filename_wc) + 1) * 2;

    unsigned long  len  = entry->length;
    unsigned char* buff = new unsigned char[len];
    _lseeki64(fd, data_base + entry->offset, SEEK_SET);
    read(fd, buff, len);

    if (!process_pnap(filename, buff, len)) {
      if (as::stringtol(filename).find(".mos") != string::npos) {
        filename += as::guess_file_extension(buff, len);
      }

      as::write_file(filename, buff, len);
    }

    delete [] buff;
  }

  delete [] toc_buff;

  close(fd);

  return 0;
}
