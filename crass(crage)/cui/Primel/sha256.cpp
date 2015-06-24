#include <windows.h>

struct sha256_ctx {
	unsigned int state[8];
	unsigned int Nl, Nh;
	unsigned char buffer[64];
	unsigned int num;
};

#define SWAP32(v)		((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))
#define ROTATE(x, s)	(((x) >> (s)) | ((x) << (32 - (s))))

#define Sigma0(x)	(ROTATE((x),30) ^ ROTATE((x),19) ^ ROTATE((x),10))
#define Sigma1(x)	(ROTATE((x),26) ^ ROTATE((x),21) ^ ROTATE((x),7))
#define sigma0(x)	(ROTATE((x),25) ^ ROTATE((x),14) ^ ((x)>>3))
#define sigma1(x)	(ROTATE((x),15) ^ ROTATE((x),13) ^ ((x)>>10))
#define Ch(x,y,z)	(((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SHA_LBLOCK	16
#define SHA256_CBLOCK	(SHA_LBLOCK*4)

#define HOST_c2l(c,l)	(l =(((unsigned long)(*((c)++)))<<24),		\
			 l|=(((unsigned long)(*((c)++)))<<16),		\
			 l|=(((unsigned long)(*((c)++)))<< 8),		\
			 l|=(((unsigned long)(*((c)++)))    ),		\
			 l)

static const unsigned int K256[64] = {
	0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,
	0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
	0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,
	0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
	0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,
	0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
	0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,
	0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
	0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,
	0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
	0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,
	0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
	0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,
	0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
	0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,
	0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL
};

static void sha256_init(struct sha256_ctx *ctx)
{
	ctx->Nl = ctx->Nh = 0;
	ctx->num = 0;
	ctx->state[0] = 0x6A09E667;
	ctx->state[1] = 0xBB67AE85;
	ctx->state[2] = 0x3C6EF372;
	ctx->state[3] = 0xA54FF53A;
	ctx->state[4] = 0x510E527F;
	ctx->state[5] = 0x9B05688C;
	ctx->state[6] = 0x1F83D9AB;
	ctx->state[7] = 0x5BE0CD19;
}

#if 1
static void sha256_block(struct sha256_ctx *ctx, const void *in)
{
	unsigned int a,b,c,d,e,f,g,h,s0,s1,T1,T2;
	unsigned int X[16];
	int i;
	const unsigned char *data = (unsigned char *)in;

	a = ctx->state[0];	b = ctx->state[1];	c = ctx->state[2];	d = ctx->state[3];
	e = ctx->state[4];	f = ctx->state[5];	g = ctx->state[6];	h = ctx->state[7];

	unsigned int l;

	for (i = 0; i < 16; ++i) {
		HOST_c2l(data,l); T1 = X[i] = l;
		T1 += h + Sigma1(e) + Ch(e,f,g) + K256[i];
		T2 = Sigma0(a) + Maj(a,b,c);
		h = g;	g = f;	f = e;	e = d + T1;
		d = c;	c = b;	b = a;	a = T1 + T2;
	}

	for (; i < 64; i++) {
		s0 = X[(i+1)&0x0f];	
		s0 = sigma0(s0);
		s1 = X[(i+14)&0x0f];
		s1 = sigma1(s1);

		T1 = X[i&0xf] += s0 + s1 + X[(i+9)&0xf];
		T1 += h + Sigma1(e) + Ch(e,f,g) + K256[i];
		T2 = Sigma0(a) + Maj(a,b,c);
		h = g;	g = f;	f = e;	e = d + T1;
		d = c;	c = b;	b = a;	a = T1 + T2;
	}

	ctx->state[0] += a;	ctx->state[1] += b;	ctx->state[2] += c;	ctx->state[3] += d;
	ctx->state[4] += e;	ctx->state[5] += f;	ctx->state[6] += g;	ctx->state[7] += h;
}
#else
static void sha256_block(struct sha256_ctx *ctx)
{
	unsigned int h[64];
	unsigned int x[8];

	for (int i = 0; i < 8; ++i)
		x[i] = ctx->state[i];

	for (i = 0; i < 16; ++i)
		h[i] = SWAP32(((unsigned int *)ctx->buffer)[i]);

	int k = 0;
	for (i = 0; i < 12; ++i) {
		h[k + 16] = h[k + 0] + sigma0(h[k + 1]) + sigma1(h[k + 14]) + h[k + 9];		
		h[k + 17] = h[k + 1] + sigma0(h[k + 2]) + sigma1(h[k + 15]) + h[k + 10];
		h[k + 18] = h[k + 2] + sigma0(h[k + 3]) + sigma1(h[k + 16]) + h[k + 11];
		h[k + 19] = h[k + 3] + sigma0(h[k + 4]) + sigma1(h[k + 17]) + h[k + 12];
		k += 4;
	}

#if 0
	for (i = 0; i < 256; ++i) {
		#define Sigma0(x)	(ROTATE((x),30) ^ ROTATE((x),19) ^ ROTATE((x),10))
		#define Sigma1(x)	(ROTATE((x),26) ^ ROTATE((x),21) ^ ROTATE((x),7))

		v38 = *(x_5 - 1);
		v39 = *(x_5 + 1);
		v40 = __ROL__(v38, 11);
		v41 = __ROR__(v38, 7);
		v42 = v41 ^ v40;
		v43 = __ROL__(v38, 6);
		v44 = v43 ^ v42;
		v45 = v38 & (*x_5 ^ v39);
		v46 = *(x_5 - 5);
		v47 = *(int *)((char *)&dword_4ABD38 + v5) + (v39 ^ v45) + v44;
		v48 = *(x_5 - 4);
		v49 = *(x_5 + 2) + *(int *)((char *)&h + v5) + v47;
		v50 = __ROL__(v46, 13);
		v51 = __ROR__(v46, 10);
		v52 = v51 ^ v50;
		v53 = __ROL__(v46, 2);
		v54 = v49 + (v53 ^ v52) + (v48 & *(x_5 - 3) | v46 & (v48 | *(x_5 - 3)));
		v55 = v49 + *(x_5 - 2);
		v56 = __ROL__(v55, 11);
		v57 = __ROR__(v55, 7);
		v58 = v57 ^ v56;
		*(x_5 - 6) = v54;
		v59 = __ROL__(v55, 6);
		v60 = *(int *)((char *)&dword_4ABD3C + v5) + (*x_5 ^ v55 & (*x_5 ^ *(x_5 - 1))) + (v59 ^ v58);
		*(x_5 - 2) = v55;
		v61 = *(x_5 - 6);
		v62 = *(x_5 + 1) + *(int *)((char *)&h_1 + v5) + v60;
		v63 = __ROL__(v61, 13);
		v64 = __ROR__(v61, 10);
		v65 = v64 ^ v63;
		v66 = __ROL__(v61, 2);
		v67 = v62 + (v66 ^ v65) + (v48 & *(x_5 - 5) | v61 & (v48 | *(x_5 - 5)));
		v68 = *(x_5 - 2);
		v69 = v62 + *(x_5 - 3);
		*(x_5 - 7) = v67;
		*(x_5 - 3) = v69;
		v70 = __ROL__(v69, 11);
		v71 = __ROR__(v69, 7);
		v72 = v71 ^ v70;
		v73 = __ROL__(v69, 6);
		v74 = *(int *)((char *)&h_2 + v100)
			+ *(int *)((char *)&dword_4ABD40 + v100)
			+ (*(x_5 - 1) ^ v69 & (v68 ^ *(x_5 - 1)))
			+ (v73 ^ v72);
		v75 = *(x_5 - 7);
		v76 = *x_5 + v74;
		v77 = __ROL__(v75, 13);
		v78 = __ROR__(v75, 10);
		v79 = v78 ^ v77;
		v80 = __ROL__(v75, 2);
		v81 = *(x_5 - 5) & v61 | v75 & (*(x_5 - 5) | v61);
		v82 = v76 + *(x_5 - 4);
		v83 = v76 + (v80 ^ v79) + v81;
		v84 = __ROL__(v82, 11);
		v85 = __ROR__(v82, 7);
		v86 = v85 ^ v84;
		v87 = __ROL__(v82, 6);
		v88 = (*(x_5 - 2) ^ v82 & (*(x_5 - 2) ^ *(x_5 - 3))) + (v87 ^ v86);
		*(x_5 - 4) = v82;
		v89 = v100;
		v90 = v83;
		v91 = *(x_5 - 1) + *(_DWORD *)&h_3_array[v100] + *(int *)((char *)&dword_4ABD44[0] + v100) + v88;
		v92 = __ROL__(v83, 13);
		*(x_5 - 8) = v83;
		v83 = __ROR__(v83, 10);
		v90 = __ROL__(v90, 2);
		v93 = v91 + (v90 ^ v83 ^ v92) + (*(x_5 - 7) & v61 | *(x_5 - 8) & (*(x_5 - 7) | v61));
		v94 = v91 + *(x_5 - 5);
		v5 = v89 + 16;
		*(x_5 - 9) = v93;
		*(x_5 - 5) = v94;
		x_5 -= 4;
		v100 = v5;


	}

#endif
}
#endif

static void sha256_update(struct sha256_ctx *ctx, 
						  unsigned char *data, unsigned int len)
{
	if (!len)
		return;

	unsigned int l = ctx->Nl + (len << 3);
	/* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
	 * Wei Dai <weidai@eskimo.com> for pointing it out. */
	if (l < ctx->Nl) /* overflow */
		++ctx->Nh;
	ctx->Nh += len >> 29;	/* might cause compiler warning on 16-bit */
	ctx->Nl = l;

	do {
		unsigned int cp = 64 - ctx->num;
		if (len < cp)
			cp = len;
		memcpy(ctx->buffer + ctx->num, data, cp);
		ctx->num += cp;
		data += cp;
		len -= cp;
		if (ctx->num == 64) {
			sha256_block(ctx, ctx->buffer);
			ctx->num = 0;
		}
	} while (len);
}

static void sha256_final(struct sha256_ctx *ctx, unsigned char *md,
						 unsigned int md_len)
{
	unsigned int Nl = SWAP32(ctx->Nl);
	unsigned int Nh = SWAP32(ctx->Nh);
	unsigned char end[2] = { 0x80, 0x00 };

	sha256_update(ctx, &end[0], 1);
	while (ctx->num != 56)
		sha256_update(ctx, &end[1], 1);

	sha256_update(ctx, (unsigned char *)&Nh, 4);
	sha256_update(ctx, (unsigned char *)&Nl, 4);
}

static void sha256_copy(struct sha256_ctx *ctx, unsigned char *md, 
						unsigned int md_len)
{
	unsigned int _md[8];

	for (int i = 0; i < 8; ++i)
		_md[i] = SWAP32(ctx->state[i]);

	memset(md, 0, md_len);

	unsigned char *p = (unsigned char *)_md;
	int k;
	for (k = 0, i = 0; k < 32; ++k, ++i) {
		if (i >= md_len)
			i -= md_len;

		md[i] ^= p[k];
	}
}

void sha256(unsigned char *md, unsigned int md_len,
			unsigned char *data, unsigned int len)
{
	struct sha256_ctx ctx;

	sha256_init(&ctx);
	sha256_update(&ctx, data, len);
	sha256_final(&ctx, md, md_len);
	sha256_copy(&ctx, md, md_len);
}
