#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <utility.h>
#include <stdio.h>
#include <crass_types.h>
#include <cui_error.h>
	
UTILITY_API int MyCreatePath(const TCHAR *path)
{
	TCHAR *compt;
	TCHAR *tmp_path, *name, *p;
	DWORD len;

	if (PathIsRoot(path))
		return 0;

	len = _tcslen(path);	
	tmp_path = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
	if (!tmp_path) {
		crass_printf(_T("%s: Out of memeory for allocate file path\n"), path);
		return -1;
	}
	tmp_path[len] = 0;
	_tcsncpy(tmp_path, path, len);

	name = PathFindFileName(tmp_path);
	if (name)
		tmp_path[name - tmp_path] = 0;
	
	if (!*tmp_path || PathIsRoot(tmp_path)) {
		free(tmp_path);
		return 0;
	}

	p = tmp_path;
	while (p = _tcsstr(p, _T("/")))
		*p = _T('\\');

	p = tmp_path;
	while (compt = _tcsstr(p, _T("\\"))) {
		TCHAR bak = *(compt + 1);
		*(compt + 1) = 0;

		if (!PathIsRoot(tmp_path)) {
			if (!CreateDirectory(tmp_path, NULL)) {
				if (GetLastError() != ERROR_ALREADY_EXISTS) {
					crass_printf(_T("%s: Can't create directory\n"), tmp_path);
					free(tmp_path);
					return -1;
				}
			}
		}
		*(compt + 1) = bak;
		p = compt + 1;	/* may NULL */
	}
	free(tmp_path);
	
	return 0;
}

#if 1
UTILITY_API HANDLE MyOpenFile(const TCHAR *name)
{	
	return CreateFile(name, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, /*FILE_ATTRIBUTE_READONLY*/FILE_ATTRIBUTE_NORMAL, NULL);
}
#else
UTILITY_API HANDLE MyOpenFile(TCHAR *name)
{
	HANDLE file;
	HINSTANCE hDLL;
	typedef HANDLE (__stdcall *pOpen)(TCHAR *, DWORD, 
		DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, 
		HANDLE);
	pOpen Open;

	hDLL = LoadLibraryA("kernel32");
	if (!hDLL)
		return INVALID_HANDLE_VALUE;
#ifdef UNICODE
	Open = (pOpen)GetProcAddress(hDLL, "CreateFileW");
#else
	Open = (pOpen)GetProcAddress(hDLL, "CreateFileA");
#endif
	GlobalFree(Library(hDLL);
	if (!Open)
		return INVALID_HANDLE_VALUE;
	
	file = Open(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_READONLY, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		_stprintf(fmt_buf, _T("%s: Can't open file.\n"), name);
		wcprintf_error(fmt_buf);
	}

	return file;
}
#endif

#if 1
UTILITY_API HANDLE MyCreateFile(const TCHAR *name)
{
	HANDLE file;

	MyCreatePath(name);
	DeleteFile(name);

	file = CreateFile(name, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		crass_printf(_T("%s: Can't create file\n"), name);

	return file;
}
#else
UTILITY_API HANDLE MyCreateFile(const TCHAR *name)
{
	HANDLE file;
	HINSTANCE hDLL;
	typedef HANDLE (__stdcall *pCreate)(TCHAR *, DWORD, 
		DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, 
		HANDLE);
	pCreate Create;

	MyCreatePath(name);
	DeleteFile(name);
	hDLL = LoadLibraryA("kernel32");
	if (!hDLL)
		return INVALID_HANDLE_VALUE;
#ifdef UNICODE
	Create = (pCreate)GetProcAddress(hDLL, "CreateFileW");
#else
	Create = (pCreate)GetProcAddress(hDLL, "CreateFileA");
#endif
	FreeLibrary(hDLL);
	if (!Create)
		return INVALID_HANDLE_VALUE;
	
	file = Create(name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
					FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		_stprintf(fmt_buf, _T("%s: Can't create file\n"), name);
		wcprintf_error(fmt_buf);
	}

	return file;
}
#endif

UTILITY_API DWORD MyGetFilePos(HANDLE file)
{
	DWORD offset;

	offset = SetFilePointer(file, 0, NULL, FILE_CURRENT); 
	if (offset == -1)
		crass_printf(_T("Can't get file pos.\n"));

	return offset;
}

UTILITY_API int MyGetFilePosition(HANDLE file, PLONG offset_lo, 
					  PLONG offset_hi)
{
	DWORD offset;
	int ret = 0;
	LONG hi = 0;
	
	/* 如果不提供dummy，会返回失败 */
	offset = SetFilePointer(file, 0, &hi, FILE_CURRENT);
	if (offset == 0xffffffff && GetLastError() != NO_ERROR)
			ret = -1;
	else {
		*offset_lo = offset;
		if (offset_hi)
			*offset_hi = hi;
	}

	return ret;
}

UTILITY_API int MySetFilePosition(HANDLE file, LONG offset_lo, 
					  PLONG offset_hi, DWORD method)
{
	int ret = 0;

	if (SetFilePointer(file, offset_lo, offset_hi, method) == 0xffffffff) {
		DWORD err = GetLastError();
		if (err != NO_ERROR)
			ret = -1;
	}

	return ret;
}

UTILITY_API int MyReadFile(HANDLE file, void *buf, DWORD len)
{
	DWORD actlen;

	if (!ReadFile(file, buf, len, &actlen, 0))
		return -1;
	if (actlen != len)
		return -1;		

	return 0;
}

UTILITY_API int MyActReadFile(HANDLE file, void *buf, DWORD *ret_len)
{
	DWORD len = *ret_len;
	
	if (!ReadFile(file, buf, len, ret_len, 0))
		return -1;

	return 0;
}

UTILITY_API int MyWriteFile(HANDLE file, void *buf, DWORD len)
{
	DWORD actlen;

	if (!WriteFile(file, buf, len, &actlen, 0)) {
		crass_printf(_T("Can't write file.\n"));
   		return -1;
	}
	if (len != actlen) {
		crass_printf(_T("Short of write file.\n"));
    	return -1;
	}

	return 0;
}

UTILITY_API void MyCloseFile(HANDLE file)
{
	HINSTANCE hDLL;
	typedef int (__stdcall *pClose)(HANDLE);
	pClose Close;
	
	hDLL = LoadLibraryA("ntdll");
	if (!hDLL)
		return;

	Close = (pClose)GetProcAddress(hDLL, "ZwClose");
	FreeLibrary(hDLL);
	if (!Close)
		return;
	Close(file);
}

UTILITY_API int MyGetFileSize(HANDLE file, DWORD *size)
{
	DWORD len;

	len = GetFileSize(file, NULL);
	if (len == INVALID_FILE_SIZE) {
		crass_printf(_T("Can't get file size.\n"));
		return -1;
	}

	*size = len;
	
	return 0;
}

UTILITY_API int MyGetFilePos(HANDLE file, DWORD *offset)
{
	DWORD val;

	val = SetFilePointer(file, 0, NULL, FILE_CURRENT);
	if (val == 0xFFFFFFFF) {
		crass_printf(_T("Can't seek file.\n"));
		return -1;
	}
	*offset = val;

	return 0;
}

UTILITY_API int MySaveFile(TCHAR *name, void *buf, DWORD len)
{
	HANDLE file;

	DeleteFile(name);
	file = MyCreateFile(name);
	if (file == INVALID_HANDLE_VALUE) 
		return -1;

	if (MyWriteFile(file, buf, len)) {
		MyCloseFile(file);
		DeleteFile(name);
		return -1;
	}
	MyCloseFile(file);

	return 0;
}

UTILITY_API int MySaveBMPFile(TCHAR *name, BYTE *buf, DWORD len, 
			BYTE *palette, DWORD palette_size, 
			DWORD width, DWORD height, DWORD bits_count,
			DWORD need_align)
{
	HANDLE file;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmiHeader;
	DWORD line_len, image_size, tmp_height, raw_line_len;
	TCHAR fname[256];	
	BYTE *dib_buf = NULL;
	unsigned int use_gray_palette = 0;
	unsigned int colors;
	
	raw_line_len = width * bits_count / 8;	
	line_len = (width * bits_count / 8 + 3) & ~3;	
	colors = 1 << bits_count;

	tmp_height = height;
	if (tmp_height & 0x80000000)
		tmp_height = 0 - height;
	image_size = line_len * tmp_height;

	crass_printf(_T("%s: BMP width %d, height %d, %d bits\n"), 
		name, width, tmp_height, bits_count);

	if ((raw_line_len != line_len) && (need_align & PALETTE_NEED_ALIGN)) {
		dib_buf = (BYTE *)GlobalAlloc(GMEM_FIXED, image_size);
		if (!dib_buf)
			return -1;

		for (unsigned int y = 0; y < tmp_height; y++) {
			for (unsigned int x = 0; x < width; x++) {
					for (unsigned int p = 0; p < bits_count / 8; p++)
						dib_buf[y * line_len + x * bits_count / 8 + p] = buf[y * raw_line_len + x * bits_count / 8 + p];
			}
		}
	}

	if (bits_count <= 8) {
		if (!palette && !palette_size) {
			palette_size = (1 << bits_count) * 4;
			palette = (BYTE *)GlobalAlloc(GMEM_FIXED, palette_size);
			if (!palette) {
				if (dib_buf)
					GlobalFree(dib_buf);
				return -1;
			}
			
			for (unsigned int p = 0; p < colors; p++) {
				palette[p * 4 + 0] = p;
				palette[p * 4 + 1] = p;
				palette[p * 4 + 2] = p;
				palette[p * 4 + 3] = 0;
			}

			use_gray_palette = 1;
		} else if (need_align & PALETTE_NEED_ALIGN) {
			BYTE *tmp;
			
			tmp = (BYTE *)GlobalAlloc(GMEM_FIXED, (1 << bits_count) * 4);
			if (!tmp) {
				if (dib_buf)
					GlobalFree(dib_buf);
				return -1;
			}
			
			for (unsigned int p = 0; p < colors; p++) {
				tmp[p * 4 + 0] = palette[p * 3 + 0];
				tmp[p * 4 + 1] = palette[p * 3 + 1];
				tmp[p * 4 + 2] = palette[p * 3 + 2];
				tmp[p * 4 + 3] = 0;
			}
			palette	= tmp;
			palette_size = colors * 4;		
		}
	} else {
		palette_size = 0;
		palette = NULL;
	}
	
	bmfh.bfType = 0x4D42;
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) 
		+ palette_size + image_size;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) 
		+ palette_size;

	bmiHeader.biSize = sizeof(bmiHeader);
	bmiHeader.biWidth = width;		
	bmiHeader.biHeight = height;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = (WORD)bits_count;
	bmiHeader.biCompression = BI_RGB;		
	bmiHeader.biSizeImage = image_size;
	bmiHeader.biXPelsPerMeter = 0;
	bmiHeader.biYPelsPerMeter = 0;
	bmiHeader.biClrUsed = 0;		
	bmiHeader.biClrImportant = 0;
			
	if (!_tcsstr(name, _T(".bmp")) && !_tcsstr(name, _T(".BMP"))) {
		if (bits_count <= 8)
			_stprintf(fname, _T("%s.MSK.BMP"), name);
		else
			_stprintf(fname, _T("%s.BMP"), name);
	} else
		_stprintf(fname, _T("%s"), name);
			
	DeleteFile(fname);
	file = MyCreateFile(fname);
	if (file == INVALID_HANDLE_VALUE) {
		if (use_gray_palette)
			GlobalFree(palette);
		if (dib_buf)
			GlobalFree(dib_buf);
		return -1;
	}
	
	if (MyWriteFile(file, &bmfh, sizeof(bmfh))) {
		MyCloseFile(file);
		DeleteFile(fname);
		if (use_gray_palette)
			GlobalFree(palette);
		if (dib_buf)
			GlobalFree(dib_buf);
		return -1;
	}

	if (MyWriteFile(file, &bmiHeader, sizeof(bmiHeader))) {
		MyCloseFile(file);
		DeleteFile(fname);
		if (use_gray_palette)
			GlobalFree(palette);
		if (dib_buf)
			GlobalFree(dib_buf);
		return -1;
	}

	if (palette_size && palette) {
		if (MyWriteFile(file, palette, palette_size)) {
			MyCloseFile(file);
			DeleteFile(fname);
			if (use_gray_palette)
				GlobalFree(palette);
			if (dib_buf)
				GlobalFree(dib_buf);
			return -1;
		}
		if (use_gray_palette)
			GlobalFree(palette);
	}

	if (dib_buf) {
		if (MyWriteFile(file, dib_buf, image_size)) {
			MyCloseFile(file);
			DeleteFile(fname);
			GlobalFree(dib_buf);
			return -1;
		}
		GlobalFree(dib_buf);
	} else {
		if (MyWriteFile(file, buf, image_size)) {
			MyCloseFile(file);
			DeleteFile(fname);
			return -1;
		}
	}
	MyCloseFile(file);

	return 0;
}

UTILITY_API int MyBuildBMPFile(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length,
			DWORD width, DWORD height, DWORD bits_count,
			BYTE **ret, DWORD *ret_length, void *(*alloc)(DWORD))
{
	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
	DWORD act_height, aligned_line_length, raw_line_length;
	DWORD act_dib_length, act_palette_length, output_length;
	BYTE *pdib, *pal, *output;
	unsigned int colors, pixel_bytes;

	if (alloc == (void *(*)(DWORD))malloc)
		return -1;
	
	raw_line_length = width * bits_count / 8;	
	aligned_line_length = (width * bits_count / 8 + 3) & ~3;
	if (warnning_verbose0 && (raw_line_length != aligned_line_length)) {
		crass_printf(_T("WARNNING: dib line lenght not aligned\n"));	
		if (warnning_exit)
			exit(-1);
	}

	if (bits_count <= 8) {
		if (palette_length) {
			if (!palette || palette_length == 1024)
				colors = 256;
			else 
				colors = palette_length / 3;
		} else 
			colors = 1 << bits_count;
	} else
		colors = 0;
	act_palette_length = colors * 4;

	pixel_bytes = bits_count / 8;

	if (!(height & 0x80000000))
		act_height = height;
	else
		act_height = 0 - height;
	act_dib_length = aligned_line_length * act_height;
		
	output_length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length + act_dib_length;

	output = (BYTE *)alloc(output_length);
	if (!output)
		return -1;
		
	bmfh = (BITMAPFILEHEADER *)output;	
	bmfh->bfType = 0x4D42;
	bmfh->bfSize = output_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length;
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);	
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width;		
	bmiHeader->biHeight = act_height;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = (WORD)bits_count;
	bmiHeader->biCompression = BI_RGB;		
	bmiHeader->biSizeImage = act_dib_length;
	bmiHeader->biXPelsPerMeter = 0;
	bmiHeader->biYPelsPerMeter = 0;
	bmiHeader->biClrUsed = bits_count <= 8 ? colors : 0;		
	bmiHeader->biClrImportant = 0;
	
	pal = (BYTE *)(bmiHeader + 1);
	if (bits_count <= 8) {
		unsigned int p;

		if (!palette || !palette_length) {
			for (p = 0; p < colors; p++) {
				pal[p * 4 + 0] = p;
				pal[p * 4 + 1] = p;
				pal[p * 4 + 2] = p;
				pal[p * 4 + 3] = 0;
			}
		} else if (palette_length != act_palette_length) {
			for (p = 0; p < colors; p++) {
				pal[p * 4 + 0] = palette[p * 3 + 0];
				pal[p * 4 + 1] = palette[p * 3 + 1];
				pal[p * 4 + 2] = palette[p * 3 + 2];
				pal[p * 4 + 3] = 0;
			}
		} else
			memcpy(pal, palette, palette_length);
	}

	pdib = pal + act_palette_length;
	/* 有些系统的bmp结尾会多出2字节 */
	if (dib_length > act_dib_length)
		dib_length = act_dib_length;

	if (dib_length == act_dib_length)
		raw_line_length = aligned_line_length;

	if (act_height == height) {
		for (unsigned int y = 0; y < act_height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				for (unsigned int p = 0; p < pixel_bytes; ++p)
					pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[y * raw_line_length + x * pixel_bytes + p];
			}
		}
	} else {
		for (unsigned int y = 0; y < act_height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				for (unsigned int p = 0; p < pixel_bytes; ++p)
					pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[(act_height - y - 1) * raw_line_length + x * pixel_bytes + p];
			}
		}
	}

#if 0
	if (pixel_bytes == 4) {
		BYTE *rgba = pdib;
		for (unsigned int y = 0; y < act_height; y++) {
			for (unsigned int x = 0; x < width; x++) {
				BYTE alpha = rgba[3];
				
				rgba[0] = (rgba[0] * alpha + 0xff * ~alpha) / 255;
				rgba[1] = (rgba[1] * alpha + 0xff * ~alpha) / 255;
				rgba[2] = (rgba[2] * alpha + 0xff * ~alpha) / 255;	
				rgba += 4;
			}
		}
	}
#endif	
	*ret = output;
	*ret_length = output_length;	
	return 0;
}

UTILITY_API int MyBuildBMP16File(BYTE *dib, DWORD dib_length,
			DWORD width, DWORD height, BYTE **ret, DWORD *ret_length, 
			unsigned long flags, DWORD *mask, void *(*alloc)(DWORD))
{
	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
	DWORD act_height, aligned_line_length, raw_line_length;
	DWORD act_dib_length, act_mask_length = 0, output_length;
	BYTE *pdib, *pmask, *output;
	DWORD rgb565_mask[4] = { 0xF800, 0x07E0, 0x001F, 0 };
//	DWORD rgb565_mask[3] = { 0xF800, 0x07E0, 0x001F };
	WORD biCompression;

	if ((alloc == (void *(*)(DWORD))malloc) || !alloc)
		return -1;
	
	raw_line_length = width * 2;	
	aligned_line_length = (width * 2 + 3) & ~3;
	if (warnning_verbose0 && (raw_line_length != aligned_line_length)) {
		crass_printf(_T("WARNNING: dib line lenght not aligned\n"));	
		if (warnning_exit)
			exit(-1);
	}

	act_height = height;
	if (act_height & 0x80000000)
		act_height = 0 - height;
	act_dib_length = aligned_line_length * act_height;

	if (flags & RGB555) {
		biCompression = BI_RGB;
		mask = NULL;
	} else if (flags & RGB565) {
		biCompression = BI_BITFIELDS;
		mask = rgb565_mask;
		act_mask_length = sizeof(rgb565_mask);
	} else {
		if (!mask)
			return -1;

		biCompression = BI_BITFIELDS;
		act_mask_length = sizeof(rgb565_mask);
	}
		
	output_length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_mask_length + act_dib_length;
	output = (BYTE *)alloc(output_length);
	if (!output)
		return -1;
	
	bmfh = (BITMAPFILEHEADER *)output;	
	bmfh->bfType = 0x4D42;
	bmfh->bfSize = output_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_mask_length;
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);	
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER) + act_mask_length;
	bmiHeader->biWidth = width;		
	bmiHeader->biHeight = height;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 16;
	bmiHeader->biCompression = biCompression;		
	bmiHeader->biSizeImage = act_dib_length;
	bmiHeader->biXPelsPerMeter = 0;
	bmiHeader->biYPelsPerMeter = 0;
	bmiHeader->biClrUsed = 0;		
	bmiHeader->biClrImportant = 0;
	
	pmask = (BYTE *)(bmiHeader + 1);
	if (mask)
		memcpy(pmask, mask, act_mask_length);

	pdib = pmask + act_mask_length;	
	if (act_dib_length != dib_length) {
		for (unsigned int y = 0; y < act_height; y++) {
			for (unsigned int x = 0; x < width; x++) {
				pdib[y * aligned_line_length + x * 2 + 0] = dib[y * raw_line_length + x * 2 + 0];
				pdib[y * aligned_line_length + x * 2 + 1] = dib[y * raw_line_length + x * 2 + 1];
			}
		}
	} else
		memcpy(pdib, dib, dib_length);
	
	*ret = output;
	*ret_length = output_length;	
	return 0;
}

UTILITY_API int MySaveAsWAVE(void *wave, DWORD wave_length, 
						   WORD FormatTag, WORD Channels, 
						   DWORD SamplesPerSec, WORD BitsPerSample, 
						   void *cb, WORD cbSize, BYTE **ret_wav, 
						   DWORD *ret_wav_length, void *(*alloc)(DWORD))
{
	WAVEFORMATEX wav_header;
	DWORD wav_length, riff_chunk_len, fmt_chunk_len, data_chunk_len;
	BYTE *wav;

	if (!alloc)
		return -1;

	wav_header.wFormatTag = FormatTag;
	wav_header.nChannels = Channels;
	wav_header.wBitsPerSample = BitsPerSample;
	wav_header.nSamplesPerSec = SamplesPerSec;
	wav_header.cbSize = cbSize;
	/* 通道数×每样本的数据位值／8 */
	wav_header.nBlockAlign = Channels * BitsPerSample / 8;
	/* 波形音频数据传送速率，其值为通道数×采样率×每样本的数据位数／8 */
	wav_header.nAvgBytesPerSec = wav_header.nBlockAlign * SamplesPerSec;
	wav_header.cbSize = cbSize;

	fmt_chunk_len = sizeof(WAVEFORMATEX) + cbSize;
	data_chunk_len = wave_length;
	riff_chunk_len = (4) + (8 + fmt_chunk_len) + (8 + data_chunk_len);
	wav_length = 8 + riff_chunk_len;
	wav = (BYTE *)(*alloc)(wav_length);
	if (!wav)
		return -CUI_EMEM;

	memcpy(wav, "RIFF", 4);
	*(u32 *)&wav[4] = riff_chunk_len;
	memcpy(&wav[8], "WAVE", 4);
	memcpy(&wav[12], "fmt ", 4);
	*(u32 *)&wav[16] = fmt_chunk_len;
	memcpy(&wav[20], &wav_header, sizeof(wav_header));
	memcpy(&wav[38], cb, cbSize);
	memcpy(&wav[38 + cbSize], "data", 4);
	*(u32 *)&wav[38 + cbSize + 4] = data_chunk_len;
	memcpy(&wav[38 + cbSize + 8], wave, data_chunk_len);
	
	*ret_wav = wav;
	*ret_wav_length = wav_length;
	return 0;	
}

static int MyBuildBMPFile2(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length,
			DWORD width, DWORD height, DWORD bits_count, DWORD colors,
			BYTE **ret, DWORD *ret_length, void *(*alloc)(DWORD))
{
	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
	DWORD act_height, aligned_line_length, raw_line_length;
	DWORD act_dib_length, act_palette_length, output_length;
	BYTE *pdib, *pal, *output;
	unsigned int pixel_bytes;

	if (alloc == (void *(*)(DWORD))malloc)
		return -1;
	
	raw_line_length = (width * bits_count + 7) / 8;			
	aligned_line_length = (raw_line_length + 3) & ~3;
	if (warnning_verbose0 && (raw_line_length != aligned_line_length)) {
		crass_printf(_T("WARNNING: dib line lenght not aligned\n"));	
		if (warnning_exit)
			exit(-1);
	}

	if (bits_count <= 8) {
		if (!colors) {
			if (warnning_verbose0)
				crass_printf(_T("WARNNING: shoule specify colors\n"));
			if (warnning_exit)
				exit(-1);
			colors = 1 << bits_count;
		}
	} else
		colors = 0;
	act_palette_length = colors * 4;

	pixel_bytes = bits_count / 8;

	if (!(height & 0x80000000))
		act_height = height;
	else
		act_height = 0 - height;
	act_dib_length = aligned_line_length * act_height;
		
	output_length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length + act_dib_length;
	output = (BYTE *)alloc(output_length);
	if (!output)
		return -1;
	
	bmfh = (BITMAPFILEHEADER *)output;	
	bmfh->bfType = 0x4D42;
	bmfh->bfSize = output_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length;
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);	
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width;		
	bmiHeader->biHeight = act_height;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = (WORD)bits_count;
	bmiHeader->biCompression = BI_RGB;		
	bmiHeader->biSizeImage = act_dib_length;
	bmiHeader->biXPelsPerMeter = 0;
	bmiHeader->biYPelsPerMeter = 0;
	bmiHeader->biClrUsed = colors;		
	bmiHeader->biClrImportant = 0;
	
	pal = (BYTE *)(bmiHeader + 1);
	if (bits_count <= 8) {
		unsigned int p;

		if (!palette || !palette_length) {
			if (colors == 2) {
				pal[0 * 4 + 0] = 0;
				pal[0 * 4 + 1] = 0;
				pal[0 * 4 + 2] = 0;
				pal[0 * 4 + 3] = 0;
				pal[1 * 4 + 0] = 0xff;
				pal[1 * 4 + 1] = 0xff;
				pal[1 * 4 + 2] = 0xff;
				pal[1 * 4 + 3] = 0;
			} else {
				for (p = 0; p < colors; p++) {
					pal[p * 4 + 0] = p;
					pal[p * 4 + 1] = p;
					pal[p * 4 + 2] = p;
					pal[p * 4 + 3] = 0;
				}
			}
		} else if (palette_length != act_palette_length) {
			for (p = 0; p < colors; p++) {
				pal[p * 4 + 0] = palette[p * 3 + 0];
				pal[p * 4 + 1] = palette[p * 3 + 1];
				pal[p * 4 + 2] = palette[p * 3 + 2];
				pal[p * 4 + 3] = 0;
			}
		} else
			memcpy(pal, palette, palette_length);
	}
	
	pdib = pal + act_palette_length;
	/* 有些系统的bmp结尾会多出2字节 */
	if (dib_length > act_dib_length)
		dib_length = act_dib_length;

	if (dib_length == act_dib_length)
		raw_line_length = aligned_line_length;

	if (bits_count >= 8) {
		if (act_height == height) {
			for (unsigned int y = 0; y < act_height; ++y) {
				for (unsigned int x = 0; x < width; ++x) {
					for (unsigned int p = 0; p < pixel_bytes; ++p)
						pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[y * raw_line_length + x * pixel_bytes + p];
				}
			}
		} else {
			for (unsigned int y = 0; y < act_height; ++y) {
				for (unsigned int x = 0; x < width; ++x) {
					for (unsigned int p = 0; p < pixel_bytes; ++p)
						pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[(act_height - y - 1) * raw_line_length + x * pixel_bytes + p];
				}
			}
		}
	} else {
		if (act_height == height) {
			for (unsigned int y = 0; y < act_height; ++y) {
				for (unsigned int x = 0; x < raw_line_length; ++x)
						pdib[y * aligned_line_length + x] = dib[y * raw_line_length + x];
			}
		} else {
			for (unsigned int y = 0; y < act_height; ++y) {
				for (unsigned int x = 0; x < raw_line_length; ++x)
						pdib[y * aligned_line_length + x] = dib[(act_height - y - 1) * raw_line_length + x];
			}
		}
	}

	*ret = output;
	*ret_length = output_length;	
	return 0;
}

UTILITY_API int MySaveAsBMP(BYTE *dib, DWORD dib_length, 
			BYTE *palette, DWORD palette_length,
			DWORD width, DWORD height, DWORD bits_count, DWORD colors,
			BYTE **ret, DWORD *ret_length, void *(*alloc)(DWORD))
{
	if (bits_count != 16)
		return MyBuildBMPFile2(dib, dib_length, palette, palette_length, width, height,
			bits_count, colors, ret, ret_length, alloc);
	else
		return MyBuildBMP16File(dib, dib_length, width, height,
			ret, ret_length, 0, NULL, alloc);
}
