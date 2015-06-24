#ifndef CXDEC_H
#define CXDEC_H
	
#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

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

extern DWORD xcode_rand(struct cxdec_xcode_status *xcode);
extern int push_bytexcode(struct cxdec_xcode_status *xcode, BYTE code);
extern int push_2bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1);
extern int push_3bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2);
extern int push_4bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3);
extern int push_5bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4);
extern int push_6bytesxcode(struct cxdec_xcode_status *xcode, 
		BYTE code0, BYTE code1, BYTE code2, BYTE code3, BYTE code4, BYTE code5);
extern int push_dwordxcode(struct cxdec_xcode_status *xcode, DWORD code);

#endif
	