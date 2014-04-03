// exb.cpp, v1.05 2013/03/03
// coded by asmodean

// contact: 
//   web:   http://plaza.rakuten.co.jp/asmodean
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts individual images from abmp1x (*.b) composite
// images.  These files also contain some animation which is ignored.

// This code sucks.  I should ditch it and use a signature based
// extraction tool instead...

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <direct.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <string>

#pragma pack(1)
struct BHDR {
  char signature[16]; // "abmp11", "abmp12"
};

struct BSECTHDR {
  char signature[16]; // "abdata10", "abdata11", "abimage10"
};

struct BDATASECT {
  unsigned long length;
};

struct BIMAGESECT {
  unsigned char image_count;
};

struct BIMAGEDATSECT1 {
  char           signature[16]; // "abimgdat10", "abimgdat11"
  unsigned short name_length;
};

struct BIMAGEDAT11SECT { 
  unsigned short hash_length; // ??
};

struct BIMAGEDATSECT2 {
  unsigned char unknown1;
  unsigned long data_length;
};

struct BIMAGEDAT13SECT2 {
  unsigned char unknown1;
  unsigned long unknown2[3];
  unsigned long data_length;
};

struct BIMAGEDAT14SECT2 {
  unsigned char unknown1[77];
  unsigned long data_length;
};
#pragma pack()

using std::string;

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "exb v1.05 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.b>\n", argv[0]);
    return -1;
  }

  char*  in_filename = argv[1];
  string prefix(in_filename);
  {
    string::size_type pos = prefix.find_last_of(".");

    if (pos != string::npos) {
      prefix = prefix.substr(0, pos);
    }
  }
  
  int fd = open(in_filename, O_RDONLY | O_BINARY);

  if (fd == -1) {
    fprintf(stderr, "Could not open %s (%s)\n", in_filename, strerror(errno));
    return -1;
  }

  BHDR hdr;

  read(fd, &hdr, sizeof(hdr));

  BSECTHDR secthdr;

  while (read(fd, &secthdr, sizeof(secthdr)) > 0) {
    if (!strcmp(secthdr.signature, "abdata10") ||
        !strcmp(secthdr.signature, "abdata11") ||
        !strcmp(secthdr.signature, "abdata12") ||
        !strcmp(secthdr.signature, "abdata13")) {
      BDATASECT datasect;
      
      read(fd, &datasect, sizeof(datasect));
      lseek(fd, datasect.length, SEEK_CUR);
    } else if (!strcmp(secthdr.signature, "abimage10")) {
      BIMAGESECT imagesect;

      read(fd, &imagesect, sizeof(imagesect));

      for (int i = 0; i < imagesect.image_count; i++) {
        BIMAGEDATSECT1 ids1;        
        read(fd, &ids1, sizeof(ids1));

        char filename[1024] = { 0 };
        read(fd, filename, ids1.name_length);
        // printf("> %s\n", filename);

        if (!memcmp(ids1.signature, "abimgdat11", 10) ||
            !memcmp(ids1.signature, "abimgdat13", 10) ||
            !memcmp(ids1.signature, "abimgdat14", 10))
        {
          BIMAGEDAT11SECT ids11;
          read(fd, &ids11, sizeof(ids11));
          lseek(fd, ids11.hash_length, SEEK_CUR);
        }

        unsigned long len = 0;

        if (!memcmp(ids1.signature, "abimgdat14", 10)) {
          BIMAGEDAT14SECT2 ids2;
          read(fd, &ids2, sizeof(ids2));

          len = ids2.data_length;
        } else if (!memcmp(ids1.signature, "abimgdat13", 10)) {
          BIMAGEDAT13SECT2 ids2;
          read(fd, &ids2, sizeof(ids2));

          len = ids2.data_length;
        } else {
          BIMAGEDATSECT2 ids2;
          read(fd, &ids2, sizeof(ids2));

          len = ids2.data_length;
        }

        if (len == 0) {
          printf("%s: Null skipped.\n", in_filename);
          continue;
        }

        // Fix some junk that at least the English Windows XP doesn't like in filenames
        for (int j = 0; j < ids1.name_length; j++) {
          if (filename[j] == '<' || filename[j] == '>' || filename[j] == '*' || filename[j] == '/') {
            filename[j] = '_';
          }
        }

        char* buff = new char[len];

        read(fd, buff, len);

        string ext;
        if (!memcmp(buff, "\x89PNG", 4)) {
          ext = ".png";
        } else if (!memcmp(buff, "BM", 2)) {
          ext = ".bmp";
        } else if (!memcmp(buff + 6, "JFIF", 4)) {
          ext = ".jpg";
        } else if (!memcmp(buff, "IMOAVI", 6)) {
          ext = ".imoavi";
        }  else if (!memcmp(buff, "abmp", 4)) {
          ext = ".b";
        }

        char out_filename[1024] = { 0 };

        if (ids1.name_length) {
          sprintf(out_filename, "%s+%03d+%s%s", prefix.c_str(), i, filename, ext.c_str());
        } else {
          sprintf(out_filename, "%s+%03d%s", prefix.c_str(), i, ext.c_str());
        }

        int out_fd = open(out_filename, O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);

        if (out_fd == -1) {
          fprintf(stderr, "Could not open %s (%s)\n", out_filename, strerror(errno));
          return -1;
        }

        write(out_fd, buff, len);
        close(out_fd);

        delete [] buff;
      }
    }
  }
  
  close(fd); 

  return 0;
}
