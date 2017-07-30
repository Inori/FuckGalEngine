// exfp3.cpp, v1.5 2017/7/29
// coded by asmodean
// modified by Fuyin

// contact: 
//   web:   http://asmodean.reverse.net
//   email: asmodean [at] hush.com
//   irc:   asmodean on efnet (irc.efnet.net)

// This tool extracts/de-obfuscates data from FilePackVer3.0 (*.PACK) archives.
// Use exb to extract data from composite images (*.b).

#include "as-util.h"
#include "mt.h"
#include <mmintrin.h>
#include <windows.h>
#include <locale> 
#include <codecvt>
// Uniform obfuscation for all archives
//#define FP3_FLAVOR 1

// Per-archive obfuscation with an initial key file
//#define FP3_FLAVOR 2

// Per-archive obfuscation with an initial key file and (unique?) compiled key data
//#define FP3_FLAVOR 3

//Support "FilePackVer3.1"
#define FP3_FLAVOR 31




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
unsigned __int64 padw(unsigned __int64 a, unsigned __int64 b) {
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

inline __m64 _m_from_i64(unsigned __int64 i)
{
	__m64 m = {0};
	m.m64_u64 = i;;
	return m;
}

unsigned long crc_or_something31(unsigned char* buff, unsigned long  len)
{

	__m64 key = _mm_setzero_si64();
	__m64 result = _mm_setzero_si64();
	unsigned __int64* p = (unsigned __int64*)buff;
	unsigned __int64* end = p + (len / sizeof(__int64));
	
	_m_empty();

	while (p < end)
	{
		key = _m_paddw(key,  _m_from_i64(0xA35793A7A35793A7));
		result = _m_paddw(result, _m_pxor(_m_from_i64(*p++), key));
		result = _m_por( _m_pslld(result, _m_from_int(0x03)), _m_psrld(result, _m_from_int(0x1D)));
	}

	result = _m_pmaddwd(result, _m_psrlq(result , _m_from_int(0x20)));

	return (unsigned long)(result.m64_u64 & 0xFFFFFFFF);
}

void unobfuscate_data(unsigned char* buff, 
                      unsigned long  len, 
                      unsigned long  seed) {
  unsigned __int64  key       = 0xA73C5F9DA73C5F9D;
  unsigned long*    key_words = (unsigned long*) &key;
  unsigned __int64  mutator   = (seed + len) ^ 0xFEC9753E;

  mutator = (mutator << 32) | mutator;

  unsigned __int64* p   = (unsigned __int64*) buff;
  unsigned __int64* end = p + (len / 8);

  while (p < end) {
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

void unobfuscate_filename31(unsigned char* buff,
	unsigned long  len,
	unsigned long  seed)
{
	unsigned long char_len = len / 2; //UTF16
	unsigned long mutator = seed ^ ( (seed >> 0x10) & 0xFFFF);
	unsigned long key = ((char_len * char_len) ^ (char_len ^ 0x3E13) ^ mutator) & 0xFFFF;

	unsigned long cur_key = key;
	wchar_t* uni_name = (wchar_t*)buff;

	for (unsigned int i = 0; i != char_len; ++i)
	{
		cur_key = ((cur_key << 3) + i + key) & 0xFFFF;
		uni_name[i] ^= (wchar_t)cur_key;
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

  for (unsigned long j = 0; j < filename.length(); j++) {
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

  struct val_t {
    union {
      unsigned __int64 as_qword;
      unsigned long    as_dword[2];
      unsigned short   as_word[4];
      unsigned char    as_byte[8];
    };
  };

  static const unsigned long TABLE_SIZE = 21;
  val_t table[TABLE_SIZE] = { 0 };

  for (unsigned long i = 0; i < TABLE_SIZE - 1; i++) {
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

  while (p < end) {
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


void unobfuscate_key_file(const wstring& keyname,
	unsigned char* buff,
	unsigned long len,
	unsigned long seed)
{
	unsigned long key_b = 0x85F532;
	unsigned long key_s = 0x33F641;

	for (unsigned long i = 0; i != keyname.size(); ++i)
	{
		unsigned long wc = keyname[i];
		key_b = (wc << ((i & 7) & 0xFF)) + key_b;
		key_s ^= key_b;
	}

	unsigned long key = 0x8F32DC;

	unsigned long len_key = len & 0x00FFFFFF;
	len_key += len_key;
	len_key += len_key;
	len_key += len_key;
	len_key -= (len & 0x00FFFFFF);

	key = (key ^ len ^ key_b) + key_b + len + len_key;
	key = ((key ^ seed) + key_s) & 0x00FFFFFF;
	key = key + key * 8;

	const unsigned long table_len = 64;
	unsigned long table[table_len] = { 0 };

	union mul_t
	{
		unsigned __int64 result;
		struct
		{
			unsigned long eax;
			unsigned long edx;
		};
	};

	mul_t mutator = { 0 };
	mutator.eax = key;

	for (unsigned long j = 0; j != table_len; ++j)
	{
		mutator.eax ^= 0x8DF21431;
		mutator.result = (unsigned __int64)mutator.eax * (unsigned __int64)0x8DF21431;
		mutator.eax += mutator.edx;
		table[j] = mutator.eax;
	}

	unsigned long key_idx = table[13];
	key_idx &= 0x0F;
	key_idx += key_idx;
	key_idx += key_idx;
	key_idx += key_idx;

	unsigned char* ptable = (unsigned char*)table;
	unsigned __int64* pbuffer = (unsigned __int64*)buff;
	__m64 mm_key7 = _m_from_i64(*(unsigned __int64*)(ptable + 0x18));
	__m64 mm_key6 = { 0 };
	__m64 mm_key1 = { 0 };

	unsigned long step = len / sizeof(unsigned __int64);
	_m_empty();
	for (unsigned long k = 0; k != step; ++k)
	{
		mm_key6 = _m_from_i64(*(unsigned __int64*)(ptable + key_idx));
		mm_key7 = _m_pxor(mm_key7, mm_key6);
		mm_key7 = _m_paddd(mm_key7, mm_key6);
		mm_key1 = _m_from_i64(pbuffer[k]);
		mm_key1 = _m_pxor(mm_key1, mm_key7);
		pbuffer[k] = mm_key1.m64_u64;
		mm_key7 = _m_paddb(mm_key7, mm_key1);
		mm_key7 = _m_pxor(mm_key7, mm_key1);
		mm_key7 = _m_pslld(mm_key7, _m_from_i64(0x1));
		mm_key7 = _m_paddw(mm_key7, mm_key1);
		key_idx = (key_idx + 8) & 0x7F;
	}
}


void unobfuscate_normal_file(const wstring& filename,
	unsigned char* buff,
	unsigned long len,
	unsigned long seed,
	unsigned char* box,
	unsigned long box_len)
{
	unsigned long key_b = 0x86F7E2;
	unsigned long key_s = 0x4437F1;

	for (unsigned long i = 0; i != filename.size(); ++i)
	{
		unsigned long wc = filename[i];
		key_b = (wc << ((i & 7) & 0xFF)) + key_b;
		key_s ^= key_b;
	}

	unsigned long key = 0x56E213;

	union mul_t
	{
		unsigned __int64 result;
		struct
		{
			unsigned long eax;
			unsigned long edx;
		};
	};


	long len_key = len & 0x00FFFFFF;
	len_key *= 0xD;;

	key = (key ^ len ^ key_b) + key_b + len + len_key;
	key = ((key ^ seed) + key_s) & 0x00FFFFFF;
	key *= 0xD;

	const unsigned long table_len = 64;
	unsigned long table[table_len] = { 0 };

	mul_t mutator = { 0 };
	mutator.eax = key;

	for (unsigned long j = 0; j != table_len; ++j)
	{
		mutator.eax ^= 0x8A77F473;
		mutator.result = (unsigned __int64)mutator.eax * (unsigned __int64)0x8A77F473;
		mutator.eax += mutator.edx;
		table[j] = mutator.eax;
	}

	unsigned long key_idx = table[8];
	key_idx &= 0xD;
	key_idx += key_idx;
	key_idx += key_idx;
	key_idx += key_idx;

	unsigned char* ptable = (unsigned char*)table;
	unsigned char* pbox = box;
	unsigned __int64* pbuffer = (unsigned __int64*)buff;

	__m64 mm_key7 = _m_from_i64(*(unsigned __int64*)(ptable + 0x18));
	__m64 mm_key5 = { 0 };
	__m64 mm_key6 = { 0 };
	__m64 mm_key1 = { 0 };

	unsigned long step = len / sizeof(unsigned __int64);
	_m_empty();
	for (unsigned long k = 0; k != step; ++k)
	{
		mm_key6 = _m_from_i64(*(unsigned __int64*)(ptable + (key_idx & 0xF) * 8));
		mm_key5 = _m_from_i64(*(unsigned __int64*)(pbox + (key_idx & 0x7F) * 8));

		mm_key6 = _m_pxor(mm_key6, mm_key5);
		mm_key7 = _m_pxor(mm_key7, mm_key6);
		mm_key7 = _m_paddd(mm_key7, mm_key6);
		mm_key1 = _m_from_i64(pbuffer[k]);
		mm_key1 = _m_pxor(mm_key1, mm_key7);
		pbuffer[k] = mm_key1.m64_u64;
		mm_key7 = _m_paddb(mm_key7, mm_key1);
		mm_key7 = _m_pxor(mm_key7, mm_key1);
		mm_key7 = _m_pslld(mm_key7, _m_from_int(1));
		mm_key7 = _m_paddw(mm_key7, mm_key1);
		key_idx = (key_idx + 1) & 0x7F;
	}
}





struct bpe_hdr_t {
  unsigned char signature[4]; // "1PC\xFF"
  unsigned char flags;
  unsigned char unknown2[3];
  unsigned long original_length;
};

static const unsigned char BPE_FLAG_SHORT_LENGTH = 0x01;

struct bpe_pair_t {
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

  while (buff < end) {   
    // Read/expand literal table
    for (unsigned long i = 0; i < 256;) {
      unsigned long c = *buff++;

      // Range of literals
      if (c > 127) {        
        for (c -= 127; c > 0; c--, i++) {
          table[i].left = (unsigned char) i;
        }
      }

      // Pairs of {left, right} unless left is a literal
      for (c++; c > 0 && i < 256; c--, i++) {
        table[i].left = *buff++;

        if (i != table[i].left) {
          table[i].right = *buff++;
        }
      }
    }

    unsigned long block_len = 0;

    // This optional long length is the only difference from the BPE reference
    // implementation...
    if (hdr->flags & BPE_FLAG_SHORT_LENGTH) {
      block_len = *(unsigned short*) buff;
      buff += sizeof(unsigned short);
    } else {
      block_len = *(unsigned long*) buff;
      buff += sizeof(unsigned long);
    }
    
    // Decompress block
    while (block_len || stack_len) {
      unsigned char c = 0;

      if (stack_len) {
        c = stack[--stack_len];
      } else {
        c = *buff++;
        block_len--;
      }

      if (c == table[c].left) {
        *out_buff++ = c;
      } else {
        stack[stack_len++] = table[c].right;
        stack[stack_len++] = table[c].left;
      }
    }
  }
}

struct DPNGHDR {
  unsigned char signature[4]; // "DPNG"
  unsigned long unknown1;
  unsigned long entry_count;
  unsigned long width;
  unsigned long height;
};

struct DPNGENTRY {
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
  if (len < 4 || memcmp(buff, "DPNG", 4)) {
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
struct ARGBHDR {
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


bool search_key2_buff_from_exe(unsigned char *exe_buff, unsigned long exe_len, unsigned char *key2_buff, unsigned long key2_len)
{
	if (!exe_buff)
		return false;

	for (unsigned long i = exe_len; i != 0; i--)
	{
		if (!strncmp((char*)&exe_buff[i - 1], "\x05TIcon", 6))
		{
			memcpy(key2_buff, &exe_buff[i - 1 + 6], key2_len);
			return true;
		}
	}

	return false;
}

bool load_reskey_from_exe(const string& exe_name, unsigned char** key_buff, unsigned long* key_len)
{
	if (exe_name.empty())
	{
		return false;
	}
	HMODULE hExe = LoadLibraryA(exe_name.c_str());
	if (!hExe)
	{
		return false;
	}

	HRSRC hRes = FindResourceA(hExe, "RESKEY", RT_RCDATA);
	if (!hRes) 
	{
		fprintf(stderr, "Failed to find resource %s:%s\n", "RESKEY", "RCDATA");
		exit(-1);
	}

	HGLOBAL hGlobal = LoadResource(hExe, hRes);
	if (!hGlobal) 
	{
		fprintf(stderr, "Failed to load resource %s:%s\n", "RESKEY", "RCDATA");
		exit(-1);
	}

	unsigned long len = SizeofResource(hExe, hRes);
	unsigned char* buff = new unsigned char[len];

	void* locked_buff = LockResource(hGlobal);
	if (!locked_buff)
	{
		fprintf(stderr, "Failed to lock resource %s:%s\n", "RESKEY", "RCDATA");
		exit(-1);
	}

	memcpy(buff, locked_buff, len);

	*key_buff = buff;
	*key_len = len;

	FreeLibrary(hExe);

	return true;
}

void make_decode_table(unsigned char* table, unsigned long len, unsigned char* key_buff, unsigned long key_len)
{
	unsigned long* ptable = (unsigned long*)table;
	unsigned long step = len / (sizeof(unsigned long));

	if (key_len < 0x80)
	{
		printf("key len too short");
		exit(1);
	}

	for (long i = 0; i != step; ++i)
	{
		__int64 val = (__int64)i;
		unsigned long mod = val % 3;
		if (!mod)
		{
			ptable[i] = (i + 3) * (i + 7);
		}
		else
		{
			ptable[i] = (0 - (i + 3)) * (i + 7);
		}
	}

	unsigned long key = *(key_buff + 0x31) % 0x49 + 0x80;
	unsigned long init_key = *(key_buff + 0x31 + 0x1E) % 7 + 7;
	
	for (unsigned long j = 0; j != len; ++j)
	{
		key += init_key;
		table[j] ^= *(key_buff + (key % key_len)) & 0xFF;
	}
}

char *UnicodeToAnsi(const wchar_t *wstr, int code_page)
{
	static char result[1024];
	int len = WideCharToMultiByte(code_page, 0, wstr, -1, NULL, 0, NULL, 0);
	WideCharToMultiByte(code_page, 0, wstr, -1, result, len, NULL, 0);
	result[len] = '\0';
	return result;
}

string ws2s(const std::wstring& wstr)
{
	string str(UnicodeToAnsi(wstr.c_str(), 936));
	return str;
}

int main(int argc, char** argv) 
{

	const unsigned long key2_len = 256;
	static unsigned char key2_buff[key2_len] = {0};
#if FP3_FLAVOR == 1
  if (argc != 2) 
  {
    fprintf(stderr, "exfp3 v1.46 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack>\n", argv[0]);
    return -1;
  }
#elif FP3_FLAVOR == 2
  if (argc != 3) 
  {
    fprintf(stderr, "exfp3 v1.46 by asmodean\n\n");
    fprintf(stderr, "usage: %s <input.pack> <key.fkey>\n", argv[0]);
    return -1;
  }
#elif FP3_FLAVOR == 3
  if (argc != 4) 
  {
    fprintf(stderr, "exfp3 v1.48 by asmodean & modified by Fuyin\n\n");
    fprintf(stderr, "usage: %s <input.pack> <key.fkey> <game_name.exe>\n\n", argv[0]);
    return -1;
  }

  string exe_name = argv[3];

  int exe_fd = as::open_or_die(exe_name, O_RDONLY | O_BINARY);
  unsigned long exe_size = as::get_file_size(exe_fd);
  unsigned char *exe_buff = new unsigned char[exe_size];

  read(exe_fd, exe_buff, exe_size);
  if (!search_key2_buff_from_exe(exe_buff, exe_size, key2_buff, key2_len))
  {
	  fprintf(stderr, "Can't find key from exe file.\n\n");
	  return -1;
  }
  delete[]exe_buff;

  string key_filename(argv[2]);

  int            key_fd   = as::open_or_die(key_filename, O_RDONLY | O_BINARY);
  unsigned long  key_len  = as::get_file_size(key_fd);
  unsigned char* key_buff = new unsigned char[key_len];  
  read(key_fd, key_buff, key_len);
  close(key_fd);

#else
	if (argc != 3)
	{
		fprintf(stderr, "exfp3 v1.5 by asmodean & modified by Fuyin\n\n");
		fprintf(stderr, "usage: %s <input.pack> <game_name.exe>\n\n", argv[0]);
		return -1;
	}

	string exe_name = argv[2];

	unsigned char* reskey_buff = NULL;
	unsigned long reskey_len = 0;
	if (!load_reskey_from_exe(exe_name, &reskey_buff, &reskey_len))
	{
		fprintf(stderr, "Can't find key from exe file.\n\n");
		return -1;
	}

	

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

#if FP3_FLAVOR == 31
  for (i = 0; i < trl.entry_count; i++)
  {
	  unsigned short filename_len = (*(unsigned short*)p) * sizeof(wchar_t); 
	  p += 2 + filename_len + sizeof(PACKENTRY);
  }
#else

  for (i = 0; i < trl.entry_count; i++) 
  {
    unsigned short filename_len = *(unsigned short*) p;
    p += 2 + filename_len + sizeof(PACKENTRY);
  }

#endif

  // Compute the obfuscation seed from the CRC(?) of some hash data.
  HASHHDR*       hashhdr    = (HASHHDR*) p;
  unsigned char* hash_bytes = NULL;

  if (!strncmp((char*)hashhdr->signature, "HashVer1.4", 10))
	  hash_bytes = p + sizeof(*hashhdr) + hashhdr->length + 0x48;
  else if (!strncmp((char*)hashhdr->signature, "HashVer1.3", 10))
	  hash_bytes = p + sizeof(*hashhdr) + hashhdr->length + 0x24;
  else
  {
	  fprintf(stderr, "Not support for this HashVer.\n\n");
	  return -1;
  }
  
#if FP3_FLAVOR == 31
  unsigned long  seed = crc_or_something31(hash_bytes, 256) & 0x0FFFFFFF;
#else
  unsigned long  seed = crc_or_something(hash_bytes, 256) & 0x0FFFFFFF;
#endif

  const unsigned long decode_table_len = 0x400;
  unsigned char decode_table[decode_table_len] = { 0 };
  make_decode_table(decode_table, decode_table_len, reskey_buff, reskey_len);

  p = toc_buff;

  for (i = 0; i < trl.entry_count; i++) 
  {

#if FP3_FLAVOR == 31
	unsigned short filename_len = (*(unsigned short*)p) * sizeof(wchar_t);
	p += 2;
	unobfuscate_filename31(p, filename_len, seed);

	wstring filename((wchar_t*)p, filename_len / sizeof(wchar_t));
	p += filename_len;

	PACKENTRY* entry = (PACKENTRY*)p;
	p += sizeof(*entry);

	unsigned long  len = entry->length;
	unsigned char* buff = new unsigned char[len];
	_lseeki64(fd, entry->offset, SEEK_SET);
	read(fd, buff, len);

	if (filename.find(L"pack_keyfile") != wstring::npos)
	{
		unobfuscate_key_file(filename, buff, len, seed);
		delete[] buff;
		continue;
	}
	else
	{
		unobfuscate_normal_file(filename, buff, len, seed, decode_table, decode_table_len);
	}

	unsigned long  out_len = entry->original_length;
	unsigned char* out_buff = new unsigned char[out_len];

	if (entry->is_compressed)
	{
		unbpe(buff, len, out_buff, out_len);
	}
	else
	{
		memcpy(out_buff, buff, out_len);
	}

	string outname = ws2s(filename);

	as::make_path(outname);

	if (!proc_dpng(outname, out_buff, out_len) && !proc_argb(outname, out_buff, out_len))
	{
		as::write_file(outname, out_buff, out_len);
	}

	delete[] out_buff;
	delete[] buff;

#else
    unsigned short filename_len = *(unsigned short*) p;
	p += 2;
	unobfuscate_filename(p, filename_len, seed);

	string filename((char*)p, filename_len);


	p += filename_len;

	PACKENTRY* entry = (PACKENTRY*)p;
	p += sizeof(*entry);

	unsigned long  len = entry->length;
	unsigned char* buff = new unsigned char[len];
	_lseeki64(fd, entry->offset, SEEK_SET);
	read(fd, buff, len);

	if (entry->is_obfuscated)
	{
#if FP3_FLAVOR == 1
		unobfuscate_data(buff, len, seed);
#else
		//unobfuscate_flavor2(filename, buff, len, seed, key_buff, key_len, key2_buff, key2_len);
#endif
	}

	unsigned long  out_len = entry->original_length;
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
		delete[] key_buff;

		key_len = out_len;
		key_buff = new unsigned char[key_len];
		memcpy(key_buff, out_buff, key_len);
	}
#endif

	// Tenshi no Tamago has some pointless ARGB prefixed bitmaps
	unsigned char* data_buff = out_buff;
	unsigned long  data_len = out_len;

	as::make_path(filename);

	if (!proc_dpng(filename, data_buff, data_len) && !proc_argb(filename, data_buff, data_len))
	{
		as::write_file(filename, data_buff, data_len);
	}

	delete[] out_buff;
	delete[] buff;
#endif

  }

  delete [] toc_buff;

  close(fd); 

  return 0;
}
