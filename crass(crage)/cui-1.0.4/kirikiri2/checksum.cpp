#include <windows.h>
#include <zlib.h>

unsigned int CheckSum(BYTE *buf, unsigned int len)
{
	// compute file checksum via adler32.
	// src's file pointer is to be reset to zero.
	return adler32(adler32(0L, NULL, 0), buf, len);
}


