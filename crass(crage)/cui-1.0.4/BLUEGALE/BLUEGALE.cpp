#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <stdio.h>

 /* TODO:
 * indexwww.dat 什么用处？
 * ブルゲ的脱衣将棋: BLK支持
 */

//BLUE GALE（ブルーゲイル）
//エルフィン·ローズ～囚われの三皇女～ 体験版[OK]
//ツグナヒ２～もうひとりの奈々～ 体験版[OK]
//ないしょのティンティンたいむ 体験版[OK]
//IMMORAL
//懲らしめ２～狂育的デパガ指導～ 体験版[OK]
//はじらひ
//性裁～白濁の禊～
//コスって!My Honey [OK, 进通过升级补丁的exe文件判断]
//SPOTLIGHT～羨望と欲望の狭間～ [OK, 进通过升级补丁的exe文件判断]
//ブルゲ的脱衣将棋 体験版[OK]
//懲らしめ～狂育的指導～ [OK, 进通过升级补丁的exe文件判断]
//マーヴェリックマックス(MAVERICK MAX)
//Treating 2U [OK, 进通过升级补丁的exe文件判断]
//ツグナヒ
//PILE☆DRIVER
//sonnet～心かさねて～
//My Dear アレながおじさん
//月光獣

//ブルゲ ON DEMAND:
//カフェ·ジャンキー[OK]
//MILK·ジャンキー３ 体験版[OK]
//MILK·ジャンキー1＆2パック
//ミセス·ジャンキー 体験版[OK]
//MILK·ジャンキー２[OK]
//MILK·ジャンキー 体験版[OK]
//女医○っくす
//Ｍ＆Ｍ ～とろける魔法と甘い呪文～

//ブルゲはいぶりっど:
//おねすく！　～おねえさんすくらんぶる～ 体验版[OK]

struct acui_information BLUEGALE_cui_information = {
	_T("BLUE GALE（ブルーゲイル）"),/* copyright */
	NULL,							/* system */
	_T(".zbm .bdt .inx .snn"),		/* package */
	_T("1.0.0"),					/* revision */
	_T("痴汉公贼"),					/* author */
	_T("2007-5-17 11:48"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	/* "amp_" */
	u16 version;	/* must be 0x0001 */
	u32 uncomprlen;
	u32 data_offset;/* >= 14 */
//	u8 *name;		/* strlen(name) < 256 */
} zbm_header_t;

typedef struct {
	u32 index_entries;		
} inx_header_t;

typedef struct {
	s8 name[64];
	u32 offset;
	u32 length;
} inx_entry_t;
#pragma pack ()	
	
	
static inline unsigned char getbit_be(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << (7 - pos)));
}

static unsigned int zbm_decompress(u8 *uncompr, unsigned int uncomprlen,
				   u8 *compr, unsigned int comprlen)
{
	unsigned int curbyte = 0;
	unsigned int curbit = 1;
	unsigned int act_uncomprlen = 0;
	u8 copy_bytes;
	unsigned int i;

	while (1) {
		copy_bytes = 0;
		for (i = 0; i < 8; i++) {
			copy_bytes <<= 1;
			copy_bytes |= getbit_be(compr[curbyte], curbit++);
			if (curbit == 8) {
				curbit = 0;
				curbyte++;
				if (curbyte == comprlen)
					goto out;
			}
		}
				
		if (copy_bytes & 0x80) {
			unsigned int offset;
			
			copy_bytes &= ~0x80;
			offset = 0;
			for (i = 0; i < 10; i++) {
				offset <<= 1;
				offset |= getbit_be(compr[curbyte], curbit++);
				if (curbit == 8) {
					curbit = 0;
					curbyte++;
					if (curbyte == comprlen)
						goto out;
				}
			}
				
			for (i = 0; i < copy_bytes; i++) {	
				if (act_uncomprlen >= uncomprlen)
					goto out;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		} else {
			for (i = 0 ; i < copy_bytes; i++) {
				u8 data;
	
				data = 0;
				for (unsigned int k = 0; k < 8; k++) {
					data <<= 1;
					data |= getbit_be(compr[curbyte], curbit++);
					if (curbit == 8) {
						curbit = 0;
						curbyte++;
						if (curbyte == comprlen)
							goto out;
					}
				}
				
				if (act_uncomprlen >= uncomprlen)
					break;
				uncompr[act_uncomprlen++] = data;
			}
		}
	}
out:	
	return act_uncomprlen;
}

/********************* zbm *********************/

static int BLUEGALE_zbm_match(struct package *pkg)
{
	zbm_header_t *zbm_header;
	
	if (!pkg)
		return -CUI_EPARA;

	zbm_header = (zbm_header_t *)malloc(sizeof(*zbm_header));
	if (!zbm_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(zbm_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, zbm_header, sizeof(*zbm_header))) {
		pkg->pio->close(pkg);
		free(zbm_header);
		return -CUI_EREAD;
	}

	if (memcmp(zbm_header->magic, "amp_", 4)) {
		pkg->pio->close(pkg);
		free(zbm_header);
		return -CUI_EMATCH;
	}

	if (zbm_header->version != 1) {
		pkg->pio->close(pkg);
		free(zbm_header);
		return -CUI_EMATCH;
	}

	if (zbm_header->data_offset < 14) {
		pkg->pio->close(pkg);
		free(zbm_header);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, zbm_header);

	return 0;	
}

static int BLUEGALE_zbm_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	zbm_header_t *zbm_header;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen, act_uncomprlen;
	unsigned long zbm_size;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;
	
	if (pkg->pio->length_of(pkg, &zbm_size)) 
		return -CUI_ELEN;
	
	zbm_header = (zbm_header_t *)package_get_private(pkg);	
	comprlen = zbm_size - zbm_header->data_offset;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, compr, comprlen,	zbm_header->data_offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}	
	
	uncomprlen = zbm_header->uncomprlen;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}	
	
	act_uncomprlen = zbm_decompress(uncompr, uncomprlen, compr, comprlen);
	free(compr);
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}
	
	if (uncompr[0] == 0xbd && uncompr[1] == 0xb2)
		for (unsigned int k = 0; k < 64; k++)
			uncompr[k] = ~uncompr[k];
				
	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = zbm_size;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;	

	return 0;
}

static int BLUEGALE_zbm_save_resource(struct resource *res, 
								   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BLUEGALE_zbm_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BLUEGALE_zbm_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	zbm_header_t *zbm_header;
	
	if (!pkg)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	zbm_header = (zbm_header_t *)package_get_private(pkg);
	if (!zbm_header) {
		free(zbm_header);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation BLUEGALE_zbm_operation = {
	BLUEGALE_zbm_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BLUEGALE_zbm_extract_resource,	/* extract_resource */
	BLUEGALE_zbm_save_resource,		/* save_resource */
	BLUEGALE_zbm_release_resource,	/* release_resource */
	BLUEGALE_zbm_release			/* release */
};

/********************* bdt *********************/

static int BLUEGALE_bdt_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int BLUEGALE_bdt_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	unsigned long bdt_size;
	BYTE *bdt_data;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;
	
	if (pkg->pio->length_of(pkg, &bdt_size)) 
		return -CUI_ELEN;

	bdt_data = (BYTE *)malloc(bdt_size);
	if (!bdt_data)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, bdt_data, bdt_size, 0, IO_SEEK_SET)) {
		free(bdt_data);
		return -CUI_EREADVEC;
	}	
	
	for (unsigned int i = 0; i < bdt_size; i++)
		bdt_data[i] = ~bdt_data[i];

	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = bdt_size;
	pkg_res->actual_data = bdt_data;
	pkg_res->actual_data_length = bdt_size;	

	return 0;
}

static int BLUEGALE_bdt_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BLUEGALE_bdt_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BLUEGALE_bdt_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (!pkg)
		return;
	pkg->pio->close(pkg);
}

static cui_ext_operation BLUEGALE_bdt_operation = {
	BLUEGALE_bdt_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BLUEGALE_bdt_extract_resource,	/* extract_resource */
	BLUEGALE_bdt_save_resource,		/* save_resource */
	BLUEGALE_bdt_release_resource,	/* release_resource */
	BLUEGALE_bdt_release			/* release */
};
	
/********************* snn *********************/

static int BLUEGALE_snn_match(struct package *pkg)
{	
	if (!pkg || !pkg->lst)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

static int BLUEGALE_snn_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{		
	inx_header_t inx_header;
	unsigned int index_buffer_len;
	inx_entry_t *index_buffer;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg->lst, &inx_header, sizeof(inx_header)))
		return -CUI_EREAD;

	index_buffer_len = inx_header.index_entries * sizeof(inx_entry_t);		
	index_buffer = (inx_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	pkg_dir->index_entries = inx_header.index_entries;
	pkg_dir->index_entry_length = sizeof(inx_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int BLUEGALE_snn_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	inx_entry_t *inx_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	inx_entry = (inx_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, inx_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = inx_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = inx_entry->offset;

	return 0;
}

static int BLUEGALE_snn_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *data;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = data;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = pkg_res->raw_data_length;	

	return 0;
}

static int BLUEGALE_snn_save_resource(struct resource *res, 
								   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BLUEGALE_snn_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void BLUEGALE_snn_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir || !pkg->lst)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation BLUEGALE_snn_operation = {
	BLUEGALE_snn_match,					/* match */
	BLUEGALE_snn_extract_directory,		/* extract_directory */
	BLUEGALE_snn_parse_resource_info,	/* parse_resource_info */
	BLUEGALE_snn_extract_resource,		/* extract_resource */
	BLUEGALE_snn_save_resource,			/* save_resource */
	BLUEGALE_snn_release_resource,		/* release_resource */
	BLUEGALE_snn_release				/* release */
};
	
int CALLBACK BLUEGALE_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".snn"), NULL, 
		NULL, &BLUEGALE_snn_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".zbm"), _T(".bmp"), 
		NULL, &BLUEGALE_zbm_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bdt"), _T(".bdt.txt"), 
		NULL, &BLUEGALE_bdt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}

/* indexwww.dat的处理：
0040C247          |.  BF C015E600   MOV EDI,cafejs.00E615C0                  ;  ASCII "999-98"
0040C24C          |>  803F 00       /CMP BYTE PTR DS:[EDI],0
0040C24F          |.  74 43         |JE SHORT cafejs.0040C294
0040C251          |.  8BF7          |MOV ESI,EDI
0040C253          |.  8D4424 10     |LEA EAX,DWORD PTR SS:[ESP+10]
0040C257          |>  8A10          |/MOV DL,BYTE PTR DS:[EAX]
0040C259          |.  8A1E          ||MOV BL,BYTE PTR DS:[ESI]
0040C25B          |.  8ACA          ||MOV CL,DL
0040C25D          |.  3AD3          ||CMP DL,BL
0040C25F          |.  75 1E         ||JNZ SHORT cafejs.0040C27F
0040C261          |.  84C9          ||TEST CL,CL
0040C263          |.  74 16         ||JE SHORT cafejs.0040C27B
0040C265          |.  8A50 01       ||MOV DL,BYTE PTR DS:[EAX+1]
0040C268          |.  8A5E 01       ||MOV BL,BYTE PTR DS:[ESI+1]
0040C26B          |.  8ACA          ||MOV CL,DL
0040C26D          |.  3AD3          ||CMP DL,BL
0040C26F          |.  75 0E         ||JNZ SHORT cafejs.0040C27F
0040C271          |.  83C0 02       ||ADD EAX,2
0040C274          |.  83C6 02       ||ADD ESI,2
0040C277          |.  84C9          ||TEST CL,CL
0040C279          |.^ 75 DC         |\JNZ SHORT cafejs.0040C257
0040C27B          |>  33C0          |XOR EAX,EAX
0040C27D          |.  EB 05         |JMP SHORT cafejs.0040C284
0040C27F          |>  1BC0          |SBB EAX,EAX
0040C281          |.  83D8 FF       |SBB EAX,-1
0040C284          |>  85C0          |TEST EAX,EAX
0040C286          |.  74 0C         |JE SHORT cafejs.0040C294
0040C288          |.  83C7 10       |ADD EDI,10
0040C28B          |.  45            |INC EBP
0040C28C          |.  81FF C00FE700 |CMP EDI,cafejs.00E70FC0
0040C292          |.^ 7C B8         \JL SHORT cafejs.0040C24C
0040C294          |>  81FD A00F0000 CMP EBP,0FA0
0040C29A          |.  75 0D         JNZ SHORT cafejs.0040C2A9
0040C29C          |.  5F            POP EDI
0040C29D          |.  5E            POP ESI
0040C29E          |.  5D            POP EBP
0040C29F          |.  B8 63000000   MOV EAX,63
0040C2A4          |.  5B            POP EBX
0040C2A5          |.  83C4 0C       ADD ESP,0C
0040C2A8          |.  C3            RETN
0040C2A9          |>  C1E5 04       SHL EBP,4
0040C2AC          |.  8BC5          MOV EAX,EBP
0040C2AE          |.  5F            POP EDI
0040C2AF          |.  5E            POP ESI
0040C2B0          |.  5D            POP EBP
0040C2B1          |.  8B88 C815E600 MOV ECX,DWORD PTR DS:[EAX+E615C8]
0040C2B7          |.  8D90 C015E600 LEA EDX,DWORD PTR DS:[EAX+E615C0]
0040C2BD          |.  890D 30C19D01 MOV DWORD PTR DS:[19DC130],ECX
0040C2C3          |.  5B            POP EBX
0040C2C4          |.  8B02          MOV EAX,DWORD PTR DS:[EDX]
0040C2C6          |.  A3 E8EBE601   MOV DWORD PTR DS:[1E6EBE8],EAX
0040C2CB          |.  33C0          XOR EAX,EAX
0040C2CD          |.  66:8B4A 04    MOV CX,WORD PTR DS:[EDX+4]
0040C2D1          |.  66:890D ECEBE>MOV WORD PTR DS:[1E6EBEC],CX
0040C2D8          |.  83C4 0C       ADD ESP,0C
0040C2DB          \.  C3            RETN



  */

  /*
  obj:
[0]maj
[1]min
[4]dword0
[8]data_offset
[c]name
	u8 copy_bytes;
	unsigned int i;

	byteval = getbyte();
	if (byteval & 0x80) {
		unsigned int offset;
		copy_bytes = byteval & ~0x80;

		offset = getbits(10);
		for (i = 0 ; i < copy_bytes; i++) {
			if (act_uncomprlen > uncomprlen)
				break;
			uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
			act_uncomprlen++;
		}
	
	} else {
		copy_bytes = byteval;

		for (i = 0 ; i < copy_bytes; i++) {
			if (act_uncomprlen > uncomprlen)
				break;
			uncompr[act_uncomprlen++] = getbyte();
		}
		if (!byteval)
			goto 40125A;
			
	
	}
	*/
