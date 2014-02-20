// exfp3.cpp, v1.46 2012/02/26
// coded by asmodean

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts/de-obfuscates data from FilePackVer3.0 (*.PACK) archives.
// Use exb to extract data from composite images (*.b).

#include "as-util.h"
#include "mt.h"

// Uniform obfuscation for all archives
//#define FP3_FLAVOR 1

// Per-archive obfuscation with an initial key file
//#define FP3_FLAVOR 2

// Per-archive obfuscation with an initial key file and (unique?) compiled key data
#define FP3_FLAVOR 3

#if FP3_FLAVOR >= 3
struct game_info_t {
  string        name;
  unsigned char key2[256];
};

//ԭgame_info

unsigned long GAME_COUNT = sizeof(GAME_INFO) / sizeof(GAME_INFO[0]);
#endif

struct PACKTRL {
  unsigned char signature[16]; // "FilePackVer3.0"
  unsigned long entry_count;
  unsigned long toc_offset;
  unsigned long pad;
};

struct PACKENTRY {
  unsigned long offset;
  unsigned long unknown1;
  unsigned long length;
  unsigned long original_length;
  unsigned long is_compressed;
  unsigned long is_obfuscated;
  unsigned long hash;
};

// It seems this data is actually an obfuscated+compressed copy of the TOC.
struct HASHHDR {
  unsigned char signature[16]; // "HashVer1.3"
  unsigned long unknown1;      // 256 hash_length?
  unsigned long unknown2;
  unsigned long unknown3;
  unsigned long length;        // data length?
};

// Emulate mmx padw instruction
unsigned __int64 padw(unsigned __int64 a, unsigned __int64 b) 
{
  unsigned short* a_words = (unsigned short*) &a;
  unsigned short* b_words = (unsigned short*) &b;

  unsigned __int64 ret     = 0;
  unsigned short*  r_words = (unsigned short*) &ret;

  r_words[0] = a_words[0] + b_words[0];
  r_words[1] = a_words[1] + b_words[1];
  r_words[2] = a_words[2] + b_words[2];
  r_words[3] = a_words[3] + b_words[3];

  return ret;
}

unsigned long crc_or_something(unsigned char* buff, unsigned long  len) 
{
  unsigned __int64  key    = 0;
  unsigned __int64  result = 0;
  unsigned __int64* p      = (unsigned __int64*) buff;
  unsigned __int64* end    = p + (len / 8);

  while (p < end) 
  {
    key    = padw(key, 0x0307030703070307);
    result = padw(result, *p++ ^ key);
  }

  result ^= (result >> 32);

  return (unsigned long) (result & 0xFFFFFFFF);
}

void unobfuscate_data(unsigned char* buff, 
                      unsigned long  len, 
                      unsigned long  seed) 
{
  unsigned __int64  key       = 0xA73C5F9DA73C5F9D;
  unsigned long*    key_words = (unsigned long*) &key;
  unsigned __int64  mutator   = (seed + len) ^ 0xFEC9753E;

  mutator = (mutator << 32) | mutator;

  unsigned __int64* p   = (unsigned __int64*) buff;
  unsigned __int64* end = p + (len / 8);

  while (p < end) 
  {
    // Emulate mmx padd instruction
    key_words[0] += 0xCE24F523;
    key_words[1] += 0xCE24F523;

    key     ^= mutator;
    mutator  = *p++ ^= key;
  }
}

void unobfuscate_filename(unsigned char* buff, 
                          unsigned long  len, 
                          unsigned long  seed)
{
  unsigned long mutator = seed ^ 0x3E;
  unsigned long key     = (mutator + len) & 0xFF;

  for (unsigned int i = 1; i <= len; i++) 
  {
    buff[i - 1] ^= ((i ^ key) & 0xFF) + i;
  }
}

void unobfuscate_flavor2(const string&  filename,
                         unsigned char* buff, 
                         unsigned long  len, 
                         unsigned long  seed,
                         unsigned char* key_buff, 
                         unsigned long  key_len,
                         unsigned char* key2_buff,
                         unsigned long  key2_len)
{
  unsigned long mt_mutator = 0x85F532;
  unsigned long mt_seed    = 0x33F641;

  for (unsigned long j = 0; j < filename.length(); j++) 
  {
    mt_mutator += (unsigned char)j * (unsigned char)filename[j];
    mt_seed    ^= mt_mutator;
  }

  mt_seed += seed ^ (7 * (len & 0xFFFFFF) + len + mt_mutator + (mt_mutator ^ len ^ 0x8F32DC));
  mt_seed = 9 * (mt_seed & 0xFFFFFF);

#if FP3_FLAVOR >= 3
  mt_seed ^= 0x453A;
#endif

  init_genrand(mt_seed);
  mt_xor_state(key_buff, key_len);
  mt_xor_state(key2_buff, key2_len);

  struct val_t 
  {
    union 
    {
      unsigned __int64 as_qword;
      unsigned long    as_dword[2];
      unsigned short   as_word[4];
      unsigned char    as_byte[8];
    };
  };

  static const unsigned long TABLE_SIZE = 21;
  val_t table[TABLE_SIZE] = { 0 };

  for (unsigned long i = 0; i < TABLE_SIZE - 1; i++) 
  {
    table[i].as_dword[0] = genrand_int32();
    table[i].as_dword[1] = genrand_int32();
  }

  table[TABLE_SIZE - 1].as_dword[0] = genrand_int32();
  table[TABLE_SIZE - 1].as_dword[1] = 0;

  val_t mutator;
  mutator.as_dword[0] = genrand_int32();
  mutator.as_dword[1] = genrand_int32();

  unsigned long table_index  = genrand_int32() & 0xF;

  val_t* p   = (val_t*) buff;
  val_t* end = p + (len / 8);

  while (p < end) 
  {
    mutator.as_qword ^= table[table_index].as_qword;
    mutator.as_dword[0] += table[table_index].as_dword[0];
    mutator.as_dword[1] += table[table_index].as_dword[1];

    p->as_qword ^= mutator.as_qword;

    for (unsigned long i = 0; i < 8; i++) mutator.as_byte[i] += p->as_byte[i];
    mutator.as_qword     ^= p->as_qword;
    mutator.as_dword[0] <<= 1;
    mutator.as_dword[1] <<= 1;
    for (unsigned long i = 0; i < 4; i++) mutator.as_word[i] += p->as_word[i];

    table_index++;
    table_index &= 0xF;

    p++;
  }
}

struct bpe_hdr_t 
{
  unsigned char signature[4]; // "1PC\xFF"
  unsigned char flags;
  unsigned char unknown2[3];
  unsigned long original_length;
};

static const unsigned char BPE_FLAG_SHORT_LENGTH = 0x01;

struct bpe_pair_t 
{
  unsigned char left;
  unsigned char right;
};

// byte pair encoding algorithm
void unbpe(unsigned char* buff,
           unsigned long  len,
           unsigned char* out_buff,
           unsigned long  out_len)
{
  unsigned char* end = buff + len; 

  bpe_hdr_t* hdr = (bpe_hdr_t*) buff;
  buff += sizeof(*hdr);

  bpe_pair_t    table[256];
  //unsigned char stack[4096];
  unsigned char stack[4096];
  unsigned long stack_len = 0;

  while (buff < end) 
  {   
    // Read/expand literal table
    for (unsigned long i = 0; i < 256;) 
    {
      unsigned long c = *buff++;

      // Range of literals
      if (c > 127) 
      {        
        for (c -= 127; c > 0; c--, i++) 
        {
          table[i].left = (unsigned char) i;
        }
      }

      // Pairs of {left, right} unless left is a literal
      for (c++; c > 0 && i < 256; c--, i++) 
      {
        table[i].left = *buff++;

        if (i != table[i].left) 
        {
          table[i].right = *buff++;
        }
      }
    }

    unsigned long block_len = 0;

    // This optional long length is the only difference from the BPE reference
    // implementation...
    if (hdr->flags & BPE_FLAG_SHORT_LENGTH) 
    {
      block_len = *(unsigned short*) buff;
      buff += sizeof(unsigned short);
    } 
    else 
    {
      block_len = *(unsigned long*) buff;
      buff += sizeof(unsigned long);
    }
    
    // Decompress block
    while (block_len || stack_len) 
    {
      unsigned char c = 0;

      if (stack_len) 
      {
        c = stack[--stack_len];
      } 
      else 
      {
        c = *buff++;
        block_len--;
      }

      if (c == table[c].left) 
      {
        *out_buff++ = c;
      } else 
      {
        stack[stack_len++] = table[c].right;
        stack[stack_len++] = table[c].left;
      }
    }
  }
}

struct DPNGHDR 
{
  unsigned char signature[4]; // "DPNG"
  unsigned long unknown1;
  unsigned long entry_count;
  unsigned long width;
  unsigned long height;
};

struct DPNGENTRY 
{
  unsigned long offset_x;
  unsigned long offset_y;
  unsigned long width;
  unsigned long height;
  unsigned long length;
  unsigned long unknown1;
  unsigned long unknown2;
};

bool proc_dpng(const string&  filename,
               unsigned char* buff, 
               unsigned long len)
{
  if (len < 4 || memcmp(buff, "DPNG", 4)) 
  {
    return false;
  }

  string   prefix = as::get_file_prefix(filename);
  DPNGHDR* hdr    = (DPNGHDR*) buff;
  buff += sizeof(*hdr);

  for (unsigned long i = 0; i < hdr->entry_count; i++) 
  {
    DPNGENTRY* entry = (DPNGENTRY*) buff;
    buff += sizeof(*entry);

    if (entry->length) 
    {
      string out_filename = prefix 
                              + as::stringf("+DPNG%03d+x%dy%d", i, entry->offset_x, entry->offset_y)
                              + as::guess_file_extension(buff, entry->length);

      as::write_file(out_filename, buff, entry->length);
    }

    buff += entry->length;
  }

  return true;
}

#pragma pack(1)
struct ARGBHDR 
{
  unsigned char signature[4];   // "ARGB"
  unsigned char signature2[12]; // "SaveData1"?
  unsigned char unknown1;       // type?
  unsigned long rgb_length;
  unsigned long msk_length;
};
#pragma pack()

bool proc_argb(const string&  filename,
               unsigned char* buff, 
               unsigned long len)
{
  if (len < 4 || memcmp(buff, "ARGB", 4)) 
  {
    return false;
  }

  ARGBHDR*       hdr      = (ARGBHDR*) buff;
  unsigned char* rgb_buff = (unsigned char*) (hdr + 1);
  unsigned char* msk_buff = rgb_buff + hdr->rgb_length;

  string prefix = as::get_file_prefix(filename);

  as::write_file(prefix + "+rgb" + as::guess_file_extension(rgb_buff, hdr->rgb_length), rgb_buff, hdr->rgb_length);
  as::write_file(prefix + "+msk" + as::guess_file_extension(msk_buff, hdr->msk_length), msk_buff, hdr->msk_length);

  return true;
}

int main(int argc, char** argv) 
{
  unsigned char* key2_buff = NULL;
  unsigned long  key2_len  = 0;

#if FP3_FLAVOR == 1
  if (argc != 2) 
  {
    fprintf(stderr, "exfp3 v1.46 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack>\n", argv[0]);
    return -1;
  }
#else
#if FP3_FLAVOR == 2
  if (argc != 3) 
  {
    fprintf(stderr, "exfp3 v1.46 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack> <key.fkey>\n", argv[0]);
    return -1;
  }
#else
  if (argc != 4) 
  {
    fprintf(stderr, "exfp3 v1.46 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack> <key.fkey> <game index>\n\n", argv[0]);

    for (unsigned long i = 0; i < GAME_COUNT; i++) 
    {
      fprintf(stderr, "\tgame index = %d -> %s\n", i, GAME_INFO[i].name.c_str());
    }
    return -1;
  }

  unsigned long game_index = atol(argv[3]);
  if (game_index >= GAME_COUNT) 
  {
    fprintf(stderr, "Unknown game index: %d\n", game_index);
    return -1;
  }

  game_info_t game_info = GAME_INFO[game_index];

  key2_len  = sizeof(game_info.key2);
  key2_buff = game_info.key2;
#endif

  string key_filename(argv[2]);

  int            key_fd   = as::open_or_die(key_filename, O_RDONLY | O_BINARY);
  unsigned long  key_len  = as::get_file_size(key_fd);
  unsigned char* key_buff = new unsigned char[key_len];  
  read(key_fd, key_buff, key_len);
  close(key_fd);
#endif 

  string in_filename(argv[1]);
  
  int           fd       = as::open_or_die(in_filename, O_RDONLY | O_BINARY);
  unsigned long file_len = as::get_file_size(fd);

  PACKTRL trl;
  _lseeki64(fd, -(int)sizeof(trl), SEEK_END);
  read(fd, &trl, sizeof(trl));

  unsigned long  toc_len  = file_len - sizeof(trl) - trl.toc_offset;
  unsigned char* toc_buff = new unsigned char[toc_len];
  _lseeki64(fd, trl.toc_offset, SEEK_SET);
  read(fd, toc_buff, toc_len);

  unsigned long  i = 0;
  unsigned char* p = toc_buff;

  // Figure out how much data is TOC entries..
  for (i = 0; i < trl.entry_count; i++) 
  {
    unsigned short filename_len = *(unsigned short*) p;
    p += 2 + filename_len + sizeof(PACKENTRY);
  }

  // Compute the obfuscation seed from the CRC(?) of some hash data.
  HASHHDR*       hashhdr    = (HASHHDR*) p;
  unsigned char* hash_bytes = p + sizeof(*hashhdr) + hashhdr->length + 36;
  unsigned long  seed       = crc_or_something(hash_bytes, 256) & 0x0FFFFFFF;

  p = toc_buff;

  for (i = 0; i < trl.entry_count; i++) 
  {
    unsigned short filename_len = *(unsigned short*) p;
    p += 2;

    unobfuscate_filename(p, filename_len, seed);

    string filename((char*)p, filename_len);
    p += filename_len;

    PACKENTRY* entry = (PACKENTRY*) p;
    p += sizeof(*entry);

    unsigned long  len  = entry->length;
    unsigned char* buff = new unsigned char[len];
    _lseeki64(fd, entry->offset, SEEK_SET);
    read(fd, buff, len);

    if (entry->is_obfuscated) 
    {
#if FP3_FLAVOR == 1
      unobfuscate_data(buff, len, seed);
#else
      unobfuscate_flavor2(filename, buff, len, seed, key_buff, key_len, key2_buff, key2_len);
#endif
    }

    unsigned long  out_len  = entry->original_length;
    unsigned char* out_buff = new unsigned char[out_len];

    if (entry->is_compressed) 
    {
      unbpe(buff, len, out_buff, out_len);
    } 
    else 
    {
      memcpy(out_buff, buff, out_len);      
    }

#if FP3_FLAVOR != 1
    if (filename.find("pack_keyfile") != string::npos) 
    {
      delete [] key_buff;

      key_len  = out_len;
      key_buff = new unsigned char[key_len];
      memcpy(key_buff, out_buff, key_len);
    }
#endif

    // Tenshi no Tamago has some pointless ARGB prefixed bitmaps
    unsigned char* data_buff = out_buff;
    unsigned long  data_len  = out_len;

    as::make_path(filename);

    if (!proc_dpng(filename, data_buff, data_len) && !proc_argb(filename, data_buff, data_len)) 
    {       
      as::write_file(filename, data_buff, data_len);
    }

    delete [] out_buff;
    delete [] buff;
  }

  delete [] toc_buff;

  close(fd); 

  return 0;
}
