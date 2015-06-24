#include <tchar.h>
#include <windows.h>
#include <utility.h>

UTILITY_API void bits_init(struct bits *bits, unsigned char *stream, unsigned long stream_length)
{
	memset(bits, 0, sizeof(*bits));
	bits->stream = stream;
	bits->stream_length = stream_length;
}

#if 1

#if 1
UTILITY_API int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval)
{
	unsigned int bits_value = 0;

	while (req_bits > 0) {
		unsigned int req;

		if (!bits->curbits) {
			if (bits->curbyte >= bits->stream_length)
				return -1;
			bits->cache = bits->stream[bits->curbyte++];
			bits->curbits = 8;
		}

		if (bits->curbits < req_bits)
			req = bits->curbits;
		else
			req = req_bits;

		bits_value <<= req;
		bits_value |= bits->cache >> (bits->curbits - req);
		bits->cache &= (1 << (bits->curbits - req)) - 1;
		req_bits -= req;
		bits->curbits -= req;
	}
	*retval = bits_value;
	return 0;
}
#else
static BYTE table0[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

static BYTE table1[8] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

UTILITY_API int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval)
{
	unsigned int bits_value = 0;

	for (int i = req_bits - 1; i >= 0; i--) {
		if (!bits->curbits) {
			if (bits->curbyte >= bits->stream_length)
				return -1;

			bits->cache = bits->stream[bits->curbyte++];
		}

		if (bits->cache & table0[bits->curbits++])
			bits_value |= table1[i & 7];

		bits->curbits &= 7;
		if (!(i & 7) && i)
			bits_value <<= 8;
	}
	*retval = bits_value;

	return 0;
}
#endif

UTILITY_API int bit_get_high(struct bits *bits, void *retval)
{
	return bits_get_high(bits, 1, (unsigned int *)retval);
}	

#else

/* 把距离最高位最近的地方的值返回给retval */
UTILITY_API int bit_get_high(struct bits *bits, void *retval)
{
	if (!bits->curbits) {
		if (bits->curbyte >= bits->stream_length)
			return -1;

		bits->cache = bits->stream[bits->curbyte++];
		bits->curbits = 8;
	}
	bits->curbits--;
	*(unsigned char *)retval = bits->cache >> bits->curbits;
	bits->cache &= (1 << bits->curbits) - 1;
	return 0;
}

/* FIXME：实现错误（应该是把距离最高位最近的地方的值依次返回给retval的从高到低字节） */
#if 1
UTILITY_API int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval)
{
	for (unsigned int i = 0; i < req_bits; i++) {
		unsigned char bitval;
		
		if (bit_get_high(bits, &bitval))
			return -1;
		
		*retval <<= 1;
		*retval |= bitval & 1;
	}
	*retval &= (1 << req_bits) - 1;
	
	return 0;
}
#else
UTILITY_API int bits_get_high(struct bits *bits, unsigned int req_bits, unsigned int *retval)
{
	unsigned char byteval = 0;
	
	for (unsigned int i = 0; i < req_bits; i++) {
		unsigned char bitval;
		
		if (bit_get_high(bits, &bitval))
			return -1;
		
		byteval <<= 1;
		byteval |= bitval & 1;

		if (!((i + 1) & 7)) {
			((unsigned char *)retval)[i / 8] = byteval;
			byteval = 0;			
		}		
	}
	if (i & 7)
		((unsigned char *)retval)[(i - 1) / 8] = byteval;
	
	return 0;
}
#endif

#endif

/* 把setval的最低位设置到离最高位最近的地方开始 */
UTILITY_API int bit_put_high(struct bits *bits, unsigned char setval)
{
	bits->curbits++;
	bits->cache |= (setval & 1) << (8 - bits->curbits);
	if (bits->curbits == 8) {		
		if (bits->curbyte >= bits->stream_length)
			return -1;
		bits->stream[bits->curbyte++] = bits->cache;
		bits->curbits = 0;
		bits->cache = 0;
	}
	return 0;
}

/* 按照从高字节到低字节的顺序把setval中的值设置到离最高位最近的地方开始 */
UTILITY_API int bits_put_high(struct bits *bits, unsigned int req_bits, void *setval)
{
	unsigned int this_bits;
	unsigned int this_byte;
	unsigned int i;
	
	this_byte = req_bits / 8;	
	this_bits = req_bits & 7;
	for (int k = (int)this_bits - 1; k >= 0; k--) {
		unsigned char bitval;
		
		bitval = !!(((unsigned char *)setval)[this_byte] & (1 << k));		
		if (bit_put_high(bits, bitval))
			return -1;		
	}	
	this_bits = req_bits & ~7;
	this_byte--;
	for (i = 0; i < this_bits; i++) {
		unsigned char bitval;
		
		bitval = !!(((unsigned char *)setval)[this_byte - i / 8] & (1 << (7 - (i & 7))));		
		if (bit_put_high(bits, bitval))
			return -1;		
	}

	return 0;	
}

UTILITY_API void bits_flush(struct bits *bits)
{
	bits->stream[bits->curbyte] = bits->cache;	
}

#if 0
int bit_get_low(struct bits *bits, void *retval)
{
	int this_bits;
	unsigned char ret = 0;

	if (!bits->curbits) {
		if (bits->curbyte >= bits->stream_length)
			return -1;

		bits->cache = bits->stream[bits->curbyte++];
		bits->curbits = 8;
	}
	this_bits = req_bits <= bits->curbits ? req_bits : bits->curbits;
	bits->curbits -= this_bits;
	((unsigned char *)retval)[getbits / 8] = (bits->cache & ((1 << this_bits) - 1)) << (getbits & 7);
	bits->cache >>= this_bits;
	req_bits -= this_bits;
	getbits += this_bits;
	return 0;
}
#endif
