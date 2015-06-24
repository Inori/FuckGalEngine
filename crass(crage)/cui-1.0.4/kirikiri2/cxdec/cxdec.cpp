#include "..\xp3filter_decode.h"
#include "cxdec.h"

DWORD xcode_rand(struct cxdec_xcode_status *xcode)
{
	DWORD seed = xcode->seed;
	xcode->seed = 1103515245 * seed + 12345;
	return xcode->seed ^ (seed << 16) ^ (seed >> 16);
}

int push_bytexcode(struct cxdec_xcode_status *xcode, BYTE code)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 1 > xcode->space_size)
		return 0;

	*xcode->curr++ = code;

	return 1;
}

int push_2bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 2 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	
	return 1;
}

int push_3bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 3 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	*xcode->curr++ = code2;
	
	return 1;
}

int push_4bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 4 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	*xcode->curr++ = code2;
	*xcode->curr++ = code3;
	
	return 1;
}

int push_5bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 5 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	*xcode->curr++ = code2;
	*xcode->curr++ = code3;
	*xcode->curr++ = code4;
	
	return 1;
}

int push_6bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4, BYTE code5)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 6 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	*xcode->curr++ = code2;
	*xcode->curr++ = code3;
	*xcode->curr++ = code4;
	*xcode->curr++ = code5;
	
	return 1;
}

int push_dwordxcode(struct cxdec_xcode_status *xcode, DWORD code)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 4 > xcode->space_size)
		return 0;

	*xcode->curr++ = (BYTE)code;
	*xcode->curr++ = (BYTE)(code >> 8);
	*xcode->curr++ = (BYTE)(code >> 16);
	*xcode->curr++ = (BYTE)(code >> 24);

	return 1;
}

// @1E0179C0
static int xcode_building_start(struct cxdec_xcode_status *xcode, int stage)
{
	// PUSH EDI, PUSH ESI, PUSH EBX, PUSH ECX, PUSH EDX
	if (!push_5bytesxcode(xcode, 0x57, 0x56, 0x53, 0x51, 0x52))
		return 0;

	// MOV EDI, DWORD PTR SS:[ESP+18] (load parameter0)
	if (!push_4bytesxcode(xcode, 0x8b, 0x7c, 0x24, 0x18))
		return 0;

	if (!xcode->xcode_building(xcode, stage))
		return 0;

	// POP EDX, POP ECX, POP EBX, POP ESI, POP EDI
	if (!push_5bytesxcode(xcode, 0x5a, 0x59, 0x5b, 0x5e, 0x5f))
		return 0;

	// RETN
	return push_bytexcode(xcode, 0xc3);
}

// @1E017AE0
static int xcode_building(struct cxdec_callback *callback, DWORD seed, void *start, DWORD size)
{
	struct cxdec_xcode_status xcode;
	
	xcode.start = (BYTE *)start;
	xcode.curr = (BYTE *)start;
	xcode.space_size = size;
	xcode.seed = seed;
	xcode.xcode_building = callback->xcode_building;

	// @1E017A90
  	for (int stage = 5; stage > 0; --stage) {  		
  		if (xcode_building_start(&xcode, stage))
  			break;
  		xcode.curr = (BYTE *)start;
  	}
  	if (!stage) {
		printf("Insufficient code space\n");
		return 0;
	}

	FlushInstructionCache(GetCurrentProcess(), start, size);
	return 1;
}

// 1E01F434
struct cxdec {
	BYTE *xcode;			// 容纳128个解密函数，每个函数100字节
	void *address_list[128];// 128个解密函数的地址(用index索引)
	u32 current_count;		// 当前有效的解密函数的个数
	DWORD index_list[100];	// 记录有效的index编号
	int init_flag;
};

static struct cxdec cxdec;

static int cxdec_init(void)
{
	cxdec.xcode = (BYTE *)VirtualAlloc(NULL, 128 * 100, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!cxdec.xcode)
		return -CUI_EMEM;

	memset(cxdec.address_list, 0, sizeof(cxdec.address_list));
	cxdec.current_count = 0;
	memset(cxdec.index_list, -1, sizeof(cxdec.index_list));
	cxdec.init_flag = 1;

	return 0;
}

static void cxdec_release(void)
{
	VirtualFree(cxdec.xcode, 0, MEM_RELEASE);
	cxdec.init_flag = 0;
}

static void cxdec_execute_xcode(struct cxdec_callback *callback, DWORD hash, DWORD *ret1, DWORD *ret2)
{
	DWORD index = hash & 0x7f;
	hash >>= 7;	

	if (!cxdec.init_flag)
		cxdec_init();

	if (!cxdec.address_list[index]) {
		if (cxdec.index_list[cxdec.current_count] != index) {
			if (cxdec.index_list[cxdec.current_count] != -1)
				cxdec.address_list[cxdec.index_list[cxdec.current_count]] = 0;
			xcode_building(callback, index, cxdec.xcode + cxdec.current_count * 128, 128);
			cxdec.address_list[index] = cxdec.xcode + cxdec.current_count * 128;
			cxdec.index_list[cxdec.current_count++] = index;
			if (cxdec.current_count >= 100)
				cxdec.current_count = 0;
		}
	}

	*ret1 = (*(DWORD (*)(DWORD))cxdec.address_list[index])(hash);
	*ret2 = (*(DWORD (*)(DWORD))cxdec.address_list[index])(~hash);
}

// @1E017CD0
static void __cxdec_decode(struct cxdec_callback *callback, DWORD hash, DWORD offset, BYTE *buf, DWORD len)
{
	BYTE key[12];
	DWORD ret[2];
	
	// @1E017090(&cxdec, hash, key);
	//if (0)
		//cxdec_execute_decode(callback, hash, &ret[0], &ret[1]);
	//else
		cxdec_execute_xcode(callback, hash, &ret[0], &ret[1]);
	
	key[8] = ret[0] >> 8;
	key[9] = ret[0] >> 16;
	key[10] = ret[0];
	DWORD key1 = ret[1] >> 16;
	DWORD key2 = ret[1] & 0xffff;
	*(u32 *)&key[0] = key1;

	if (key1 == key2)
		++key2;

	*(u32 *)&key[4] = key2;
	
	if (!key[10])
		key[10] = 1;

	if ((key2 >= offset) && (key2 < offset + len))
		buf[key2 - offset] ^= key[9];
	
	if ((key1 >= offset) && (key1 < offset + len))
		buf[key1 - offset] ^= key[8];

	for (DWORD i = 0; i < len; ++i)
		buf[i] ^= key[10];
}

static void cxdec_decode(struct cxdec_callback *callback, DWORD hash, DWORD offset, BYTE *buf, DWORD len)
{
	DWORD bondary = (hash & callback->key[0]) + callback->key[1];
	DWORD dec_len;

	if (offset < bondary) {
		if (offset + len > bondary)
			dec_len = bondary - offset;
		else
			dec_len = len;
		__cxdec_decode(callback, hash, offset, &buf[offset], dec_len);
		offset += dec_len;
		dec_len = len - dec_len;
	} else
		dec_len = len;

	if (dec_len)
		__cxdec_decode(callback, (hash >> 16) ^ hash, offset, &buf[offset], dec_len);
}

/* TODO: put new callback here */
extern struct cxdec_callback fanta_cxdec_callback;
extern struct cxdec_callback FairChildTrial_cxdec_callback;
extern struct cxdec_callback towa_cxdec_callback;
extern struct cxdec_callback EXE_cxdec_callback;
extern struct cxdec_callback FHA_cxdec_callback;
extern struct cxdec_callback FairChild_cxdec_callback;
extern struct cxdec_callback ruitomoTrial_cxdec_callback;
extern struct cxdec_callback yomibito_cxdec_callback;
//extern struct cxdec_callback _11eyesTrial_cxdec_callback;
extern struct cxdec_callback haruiroTrial_cxdec_callback;
extern struct cxdec_callback _11eyes_cxdec_callback;
extern struct cxdec_callback haruiro_cxdec_callback;
extern struct cxdec_callback natukanaTrial_cxdec_callback;
extern struct cxdec_callback kinomino_cxdec_callback;
extern struct cxdec_callback natukana_cxdec_callback;
extern struct cxdec_callback ojyo_cxdec_callback;
extern struct cxdec_callback nidaime_cxdec_callback;
extern struct cxdec_callback beniten_cxdec_callback;
extern struct cxdec_callback ruitomo_cxdec_callback;
extern struct cxdec_callback nidaimeTrial_cxdec_callback;
extern struct cxdec_callback mogla_cxdec_callback;
extern struct cxdec_callback yu_gu_cxdec_callback;
extern struct cxdec_callback lovebattle_cxdec_callback;
extern struct cxdec_callback ConcertoNote_cxdec_callback;
extern struct cxdec_callback seiin_cxdec_callback;
extern struct cxdec_callback hitozuma_cxdec_callback;
extern struct cxdec_callback lovelaby_cxdec_callback;
extern struct cxdec_callback silverblue_cxdec_callback;
extern struct cxdec_callback ojo2_cxdec_callback;
extern struct cxdec_callback tenshin_Trial_cxdec_callback;
extern struct cxdec_callback tenshin_cxdec_callback;
extern struct cxdec_callback kurenai_cxdec_callback;

static struct cxdec_callback *cxdec_callback_list[] = {
	&kurenai_cxdec_callback,
	&tenshin_cxdec_callback,
	&ojo2_cxdec_callback,
	&silverblue_cxdec_callback,
	&fanta_cxdec_callback,
	&FairChildTrial_cxdec_callback,
	&towa_cxdec_callback,
	&EXE_cxdec_callback,
	&FHA_cxdec_callback,
	&FairChild_cxdec_callback,
	&ruitomoTrial_cxdec_callback,
	&yomibito_cxdec_callback,
//	&_11eyesTrial_cxdec_callback,
//	&haruiroTrial_cxdec_callback,
	&_11eyes_cxdec_callback,
	&haruiro_cxdec_callback,
	&kinomino_cxdec_callback,
	&natukana_cxdec_callback,
	&ojyo_cxdec_callback,
	&nidaime_cxdec_callback,
	&beniten_cxdec_callback,
	&ruitomo_cxdec_callback,
	&mogla_cxdec_callback,
	&yu_gu_cxdec_callback,
	&lovebattle_cxdec_callback,
	&ConcertoNote_cxdec_callback,
	&seiin_cxdec_callback,
	&hitozuma_cxdec_callback,
	&lovelaby_cxdec_callback,
	&tenshin_Trial_cxdec_callback,
	&nidaimeTrial_cxdec_callback,
	&natukanaTrial_cxdec_callback,
	NULL,
};

static void __xp3filter_decode_cxdec(const char *name, BYTE *buf, DWORD len, DWORD offset, DWORD hash)
{
	for (DWORD i = 0; cxdec_callback_list[i]; i++) {
		struct cxdec_callback *callback = cxdec_callback_list[i];
		if (!strcmpi(callback->name, name)) {
			cxdec_decode(callback, hash, offset, buf, len);
			break;
		}
	}
}

void xp3filter_decode_cxdec(struct xp3filter *xp3filter)
{
#if 0
	if (xp3filter->length)
		__xp3filter_decode_cxdec(xp3filter->parameter, xp3filter->buffer, 
			xp3filter->length, xp3filter->offset, xp3filter->hash);
#else
	if (xp3filter->length && (xp3filter->length + xp3filter->offset == xp3filter->total_length)) {
		__xp3filter_decode_cxdec(xp3filter->parameter, xp3filter->buffer, 
			xp3filter->total_length, 0, xp3filter->hash);
	}
#endif
}
