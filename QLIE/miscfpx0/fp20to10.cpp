// fp20to10.cpp, v1.0 2006/11/25
// coded by asmodean

// contact: 
//   web:   http://plaza.rakuten.co.jp/asmodean
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)
//   icq:   55244079

// This tool converts FilePackVer2.0 (*.PACK) archives to FilePack1.0.
// Yes, all it does is change one byte. ;)

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdio>

struct PACKTRL {
  unsigned char signature[16]; // "FilePackVer1.0", "FilePackVer2.0"
  unsigned long entry_count;
  unsigned long toc_offset;
  unsigned long pad;
};

static const char FP10SIG[] = "FilePackVer1.0";
static const char FP20SIG[] = "FilePackVer2.0";

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "fp20to10 v1.0 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack>\n", argv[0]);
    return -1;
  }

  char* in_filename = argv[1];
  
  int fd = open(in_filename, O_RDWR | O_BINARY);

  if (fd == -1) {
    fprintf(stderr, "Could not open %s (%s)\n", in_filename, strerror(errno));
    return -1;
  }

  PACKTRL trl;

  lseek(fd, -(int)sizeof(trl), SEEK_END);
  read(fd, &trl, sizeof(trl));

  if (memcmp(trl.signature, FP20SIG, sizeof(FP20SIG) - 1)) {
    fprintf(stderr, "%s: doesn't seem to be a 'FilePackVer2.0' archive\n", in_filename);
    return -1;
  }

  memcpy(trl.signature, FP10SIG, sizeof(FP10SIG) - 1);

  lseek(fd, -(int)sizeof(trl), SEEK_END);
  write(fd, &trl, sizeof(trl));
  close(fd); 

  return 0;
}
