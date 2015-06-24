#ifndef MY_H
#define MY_H

#include <my_common.h>

typedef char				s8;
typedef short				s16;
typedef int					s32;
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;

extern MY_API TCHAR fmt_buf[1024];
//extern MY_API TCHAR packed_filename[MAX_PATH];
//extern MY_API TCHAR resource_filename[MAX_PATH];
//extern MY_API TCHAR resource_directory[MAX_PATH];
//extern MY_API TCHAR idx_filename[MAX_PATH];
//extern MY_API TCHAR output_directory[MAX_PATH];
//extern MY_API TCHAR save_filename[MAX_PATH];
//extern MY_API unsigned int need_package;
//extern MY_API unsigned int output_index;
//extern MY_API unsigned int output_raw;
//extern MY_API unsigned int output_null;

#define SOT(x)	(sizeof(x) / sizeof(TCHAR))

#define acp2unicode(ansi, ansi_len, buf, buf_len)	\
	ansi2unicode(CP_ACP, ansi, ansi_len, buf, buf_len)

#define sj2unicode(ansi, ansi_len,buf, buf_len)	\
	ansi2unicode(932, ansi, ansi_len, buf, buf_len)

#define unicode2acp(ansi, ansi_len, buf, buf_len)	\
	unicode2ansi(CP_ACP, ansi, ansi_len, buf, buf_len)

#define unicode2sj(ansi, ansi_len,buf, buf_len)	\
	unicode2ansi(932, ansi, ansi_len, buf, buf_len)

MY_API DWORD wcprintf(const TCHAR *fmt);
MY_API DWORD wcprintf_error(const TCHAR *fmt);

//MY_API int replace_extension(TCHAR *path, TCHAR *rep_ext, TCHAR *output_path);

MY_API int ansi2unicode(int loc, char *ansi, int ansi_len, 
						TCHAR *buf, int buf_len);
MY_API int unicode2ansi(int loc, char *ansi, int ansi_len, 
						TCHAR *unicode, int unicode_len);

//char *sj2gbk(char *sjs);
MY_API int MyCreatePath(TCHAR *p);
MY_API HANDLE MyOpenFile(const TCHAR *name);
MY_API HANDLE MyCreateFile(TCHAR *name);
MY_API int MyReadFile(HANDLE file, void *buf, DWORD len);
MY_API int MyActReadFile(HANDLE file, void *buf, DWORD *ret_len);
//int MyActReadFile(HANDLE file, void *buf, DWORD len);
MY_API int MyWriteFile(HANDLE file, void *buf, DWORD len);
MY_API void MyCloseFile(HANDLE file);
MY_API int MyGetFileSize(HANDLE file, DWORD *size);
MY_API int MyGetFilePos(HANDLE file, DWORD *offset);
MY_API int MyGetFilePosition(HANDLE file, PLONG offset_lo, 
					  PLONG offset_hi);
MY_API int MySetFilePos(HANDLE file, DWORD offset, DWORD method);
MY_API int MySetFilePosition(HANDLE file, LONG offset_lo, LONG offset_hi,
					  DWORD method);
MY_API int MySaveFile(TCHAR *name, void *buf, DWORD len);
#define DIB_NEED_ALIGN			(1 << 0)
#define PALETTE_NEED_ALIGN		(1 << 1)
MY_API int MySaveBMPFile(TCHAR *name, BYTE *buf, DWORD len, 
			BYTE *palette, DWORD palette_size,
			DWORD width, DWORD height, DWORD bits_count,
			DWORD need_align);

MY_API int MyBuildBMPFile(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length, 
			DWORD width, DWORD height, DWORD bits_count,
			BYTE **ret, DWORD *ret_length);

#if 0
struct my_buffer_manager {
	BYTE *current;
	BYTE *start;
	DWORD length;
};

struct my_buffer_manager *MyCreateBuffer(void *buffer, DWORD len);
void MyCloseBuffer(struct my_buffer_manager *mbm);
int MyReadBufferByte(struct my_buffer_manager *mbm, BYTE *val);
int MyReadBufferWord(struct my_buffer_manager *mbm, WORD *val);
int MyReadBufferDword(struct my_buffer_manager *mbm, DWORD *val);
int MyReadBuffer(struct my_buffer_manager *mbm, void *buffer, DWORD len);
int MyGetBufferPos(struct my_buffer_manager *mbm);
int MySetBufferPos(struct my_buffer_manager *mbm, DWORD len, int method);

#define CUR_POS		1
#define BEGIN_POS	2
#define END_POS		3
#endif

#endif

