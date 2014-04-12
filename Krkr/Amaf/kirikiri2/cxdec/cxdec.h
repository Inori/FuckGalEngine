#ifndef CXDEC_H
#define CXDEC_H

struct cxdec_xcode_status {
	BYTE *start;
	BYTE *curr;
	DWORD space_size;
	DWORD seed;
	int (*xcode_building)(struct cxdec_xcode_status *, int);
};

struct cxdec_callback {
	const char *name;
	DWORD key[2];
	int (*xcode_building)(struct cxdec_xcode_status *, int);
};

extern DWORD xcode_rand(struct cxdec_xcode_status *xcode)
{
	DWORD seed = xcode->seed;
	xcode->seed = 1103515245 * seed + 12345;
	return xcode->seed ^ (seed << 16) ^ (seed >> 16);
}

extern int push_bytexcode(struct cxdec_xcode_status *xcode, BYTE code)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 1 > xcode->space_size)
		return 0;

	*xcode->curr++ = code;

	return 1;
}

extern int push_2bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 2 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	
	return 1;
}

extern int push_3bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 3 > xcode->space_size)
		return 0;

	*xcode->curr++ = code0;
	*xcode->curr++ = code1;
	*xcode->curr++ = code2;
	
	return 1;
}

extern int push_4bytesxcode(struct cxdec_xcode_status *xcode, 
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

extern int push_5bytesxcode(struct cxdec_xcode_status *xcode, 
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

extern int push_6bytesxcode(struct cxdec_xcode_status *xcode, 
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
extern int push_dwordxcode(struct cxdec_xcode_status *xcode, DWORD code)
{
	if ((DWORD)xcode->curr - (DWORD)xcode->start + 4 > xcode->space_size)
		return 0;

	*xcode->curr++ = (BYTE)code;
	*xcode->curr++ = (BYTE)(code >> 8);
	*xcode->curr++ = (BYTE)(code >> 16);
	*xcode->curr++ = (BYTE)(code >> 24);

	return 1;
}

#endif
