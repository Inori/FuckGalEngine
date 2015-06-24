#ifndef BITS_H
#define BITS_H

#include <my_common.h>

struct bits {
	unsigned long curbits;
	unsigned long curbyte;
	unsigned char cache;
	unsigned char *stream;
	unsigned long stream_length;
};

MY_API void bits_init(struct bits *bits, unsigned char *stream, unsigned long stream_length);

MY_API int bits_get_high(struct bits *bits, unsigned int req_bits, void *retval);
MY_API int bit_get_high(struct bits *bits, void *retval);

MY_API int bits_put_high(struct bits *bits, unsigned int req_bits, void *setval);
MY_API int bit_put_high(struct bits *bits, unsigned char setval);
MY_API void bits_flush(struct bits *bits);

#endif	/* BITS_H */
