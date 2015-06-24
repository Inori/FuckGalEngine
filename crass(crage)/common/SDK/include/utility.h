#ifndef UTILITY_H
#define UTILITY_H

#ifdef UTILITY_EXPORTS
#define UTILITY_API __declspec(dllexport)
#else
#define UTILITY_API __declspec(dllimport)
#endif 
	
/*********** utility API **************/

extern UTILITY_API unsigned int warnning_verbose0;
extern UTILITY_API unsigned int warnning_exit;
extern UTILITY_API unsigned int use_gui_wrapper;
extern UTILITY_API unsigned int use_jcrage_wrapper;

#define SOT(x)	(sizeof(x) / sizeof(TCHAR))

#define acp2unicode(ansi, ansi_len, buf, buf_len)	\
	ansi2unicode(CP_ACP, ansi, ansi_len, buf, buf_len)

#define sj2unicode(ansi, ansi_len, buf, buf_len)	\
	ansi2unicode(932, ansi, ansi_len, buf, buf_len)

#define unicode2acp(ansi, ansi_len, buf, buf_len)	\
	unicode2ansi(CP_ACP, ansi, ansi_len, buf, buf_len)

#define unicode2sj(ansi, ansi_len, buf, buf_len)	\
	unicode2ansi(932, ansi, ansi_len, buf, buf_len)

UTILITY_API int locale_printf(DWORD id, ...);
UTILITY_API int locale_app_printf(DWORD cid, DWORD id, ...);
UTILITY_API int crass_printf(const TCHAR *msg, ...);
UTILITY_API DWORD wscanf(TCHAR *fmt, SIZE_T fmt_size);

UTILITY_API int ansi2unicode(int loc, const char *ansi, int ansi_len, 
						TCHAR *buf, int buf_len);
UTILITY_API int unicode2ansi(int loc, char *ansi, int ansi_len, 
						const TCHAR *unicode, int unicode_len);

UTILITY_API int parse_options(const TCHAR *option);
UTILITY_API const char *get_options(const char *key);
UTILITY_API const TCHAR *get_options2(const TCHAR *key);

UTILITY_API void jcrage_exit(void);

/*********** my API **************/

#if 1
UTILITY_API int MyCreatePath(const TCHAR *p);
UTILITY_API HANDLE MyOpenFile(const TCHAR *name);
UTILITY_API HANDLE MyCreateFile(const TCHAR *name);
UTILITY_API int MyReadFile(HANDLE file, void *buf, DWORD len);
UTILITY_API int MyActReadFile(HANDLE file, void *buf, DWORD *ret_len);
//int MyActReadFile(HANDLE file, void *buf, DWORD len);
UTILITY_API int MyWriteFile(HANDLE file, void *buf, DWORD len);
UTILITY_API void MyCloseFile(HANDLE file);
UTILITY_API int MyGetFileSize(HANDLE file, DWORD *size);
UTILITY_API int MyGetFilePos(HANDLE file, DWORD *offset);
UTILITY_API int MyGetFilePosition(HANDLE file, PLONG offset_lo, 
					  PLONG offset_hi);
UTILITY_API int MySetFilePos(HANDLE file, DWORD offset, DWORD method);
UTILITY_API int MySetFilePosition(HANDLE file, LONG offset_lo, PLONG offset_hi,
					  DWORD method);
UTILITY_API int MySaveFile(TCHAR *name, void *buf, DWORD len);
#endif

#define PALETTE_NEED_ALIGN	2

UTILITY_API int MySaveBMPFile(TCHAR *name, BYTE *buf, DWORD len, 
			BYTE *palette, DWORD palette_size,
			DWORD width, DWORD height, DWORD bits_count,
			DWORD need_align);

/* 废弃！ */
UTILITY_API int MyBuildBMPFile(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length,
			DWORD width, DWORD height, DWORD bits_count,
			BYTE **ret, DWORD *ret_length, void *(*alloc)(DWORD));

/* 废弃！创建16 bit BMP文件 */
#define RGB555		1
#define RGB565		2
UTILITY_API int MyBuildBMP16File(BYTE *dib, DWORD dib_length,
			DWORD width, DWORD height, BYTE **ret, DWORD *ret_length, 
			unsigned long flags, DWORD *mask, void *(*alloc)(DWORD));

UTILITY_API int MySaveAsWAVE(void *wave, DWORD wave_length, 
						   WORD FormatTag, WORD Channels, 
						   DWORD SamplesPerSec, WORD BitsPerSample, 
						   void *cb, WORD cbSize, BYTE **ret_wav, 
						   DWORD *ret_wav_length, void *(*alloc)(DWORD));

UTILITY_API int MySaveAsBMP(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length,
			DWORD width, DWORD height, DWORD bits_count, DWORD colors,
			BYTE **ret, DWORD *ret_length, void *(*alloc)(DWORD));

UTILITY_API alpha_blending(BYTE *dib, DWORD width, DWORD height, DWORD bpp);
UTILITY_API alpha_blending_reverse(BYTE *dib, DWORD width, DWORD height, DWORD bpp);

/*********** bits API **************/

struct bits {
	unsigned long curbits;
	unsigned long curbyte;
	unsigned char cache;
	unsigned char *stream;
	unsigned long stream_length;
};

UTILITY_API void bits_init(struct bits *bits, unsigned char *stream, unsigned long stream_length);

UTILITY_API int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval);
UTILITY_API int bit_get_high(struct bits *bits, void *retval);

UTILITY_API int bits_put_high(struct bits *bits, unsigned int req_bits, void *setval);
UTILITY_API int bit_put_high(struct bits *bits, unsigned char setval);
UTILITY_API void bits_flush(struct bits *bits);

#endif	/* UTILITY_H */
