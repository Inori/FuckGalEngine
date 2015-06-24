#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <utility.h>
#include <cui_error.h>
#include <stdio.h>
#include <cui_common.h>

// 调试方法：
// 1. 在exe中的加载资源的文件名处下断点
// 2. 进入AioLib.dll下CreateFile断点（在所有vfs封包中寻找指定的资源文件，不息每次重新读取dir）
// 3, 进入kernel32，下ReadFile和SetFilePointer断点

/*
王賊(体験版)
グリンスヴァールの森の中
ダンシング?クレイジーズ
南国ドミニオン
巣作りドラゴン
レベルジャスティス
ブラウン通り三番目
アルフレッド学園魔物大隊
真昼に踊る犯罪者
海賊王冠
うえはぁす～お姫様は今日も危険でした～
葵屋まっしぐら
*/

/* ipf33: 读取iph文件的前0x38字节后：
003F1ABE    81BC24 9C020000>CMP DWORD PTR SS:[ESP+29C],20001 <--- 文件偏移0x14
003F1AC9    0F85 C9010000   JNZ ipf33.003F1C98
003F1ACF    83BC24 A0020000>CMP DWORD PTR SS:[ESP+2A0],0 <--- 文件偏移0x18
003F1AD7    0F85 22010000   JNZ ipf33.003F1BFF
003F1ADD    83BC24 B0020000>CMP DWORD PTR SS:[ESP+2B0],0 <--- 文件偏移0x28
003F1AE5    0F85 3E010000   JNZ ipf33.003F1C29
003F1AEB    8B8424 44030000 MOV EAX,DWORD PTR SS:[ESP+344]
*/

/* ipf33: 再读取32字节：
003F1CAC    66:8B8424 CA020>MOV AX,WORD PTR SS:[ESP+2CA] <--- 文件偏移0x42（height）
003F1CB4    50              PUSH EAX
003F1CB5    31C0            XOR EAX,EAX
003F1CB7    66:8B8424 CC020>MOV AX,WORD PTR SS:[ESP+2CC] <--- 文件偏移0x40（width）
003F1CBF    50              PUSH EAX
003F1CC0    8B8C24 4C030000 MOV ECX,DWORD PTR SS:[ESP+34C]
003F1CC7    51              PUSH ECX
003F1CC8    E8 33FDFFFF     CALL ipf33.IPFDLL_CreateCell

malloc(width * 2 * height)

003F1CD5    8B9424 DC020000 MOV EDX,DWORD PTR SS:[ESP+2DC] <--- 文件偏移0x54
003F1CDC    66:83FA FF      CMP DX,0FFFF ！！！！！！！！！是否为0xffff
003F1CE0    0F85 DD010000   JNZ ipf33.003F1EC3
003F1CE6    8B8424 44030000 MOV EAX,DWORD PTR SS:[ESP+344]
003F1CED    C740 1C 0000008>MOV DWORD PTR DS:[EAX+1C],80000000
003F1CF4    31C0            XOR EAX,EAX
003F1CF6    8B9424 44030000 MOV EDX,DWORD PTR SS:[ESP+344]
003F1CFD    66:8B8424 D0020>MOV AX,WORD PTR SS:[ESP+2D0] <--- 文件偏移0x48
003F1D05    8942 0C         MOV DWORD PTR DS:[EDX+C],EAX
003F1D08    31C0            XOR EAX,EAX
003F1D0A    66:8B8424 D2020>MOV AX,WORD PTR SS:[ESP+2D2] <--- 文件偏移0x4a
003F1D12    8942 10         MOV DWORD PTR DS:[EDX+10],EAX
003F1D15    31C0            XOR EAX,EAX
003F1D17    66:8B8424 D4020>MOV AX,WORD PTR SS:[ESP+2D4] <--- 文件偏移0x4c
003F1D1F    8942 14         MOV DWORD PTR DS:[EDX+14],EAX
003F1D22    31C0            XOR EAX,EAX
003F1D24    66:8B8424 D6020>MOV AX,WORD PTR SS:[ESP+2D6] <--- 文件偏移0x4e
003F1D2C    C742 24 0000000>MOV DWORD PTR DS:[EDX+24],0
003F1D33    8942 18         MOV DWORD PTR DS:[EDX+18],EAX
003F1D36    66:83BC24 DA020>CMP WORD PTR SS:[ESP+2DA],0 <--- 文件偏移0x52！！！！！！！！！是否为1
003F1D3F   /0F84 92010000   JE ipf33.003F1ED7
003F1D45   |8B42 04         MOV EAX,DWORD PTR DS:[EDX+4] <--- width(文件偏移0x40)
003F1D48   |8B9424 C4020000 MOV EDX,DWORD PTR SS:[ESP+2C4] <--- 文件偏移0x3c
003F1D4F   |01C0            ADD EAX,EAX
003F1D51   |83EA 20         SUB EDX,20   size - 0x20 + width * 2 =  width * 2 + bmp_data_length（实际数据读在这里） @ 1d2aff0
003F1D54   |01D0            ADD EAX,EDX
003F1D56   |50              PUSH EAX
003F1D57   |E8 B4F3FFFF     CALL ipf33.003F1110 malloc()


herhere!!!!!!!!!!!!
003F1D85    2E:FF15 38823F0>CALL DWORD PTR CS:[<&KERNEL32._hread>]   ; read_bmp_data()


*/
#if 0
static unsigned int iph_decompress()
{

	y = 0;	// [ESP+310]
	x = 0;	// [ESP+314]
//3f1db8:
	line_length = x * width;
	cur_line = uncompr[line_length];
	cur_line_act_uncomprlen = 0;	// edi

	{
		u8 flag;

		//[ESP+304] = cur_line
		flag = compr[curbyte++];
		if (flag) {	// 3f1de6
			//[ESP+328] = 0
			u8 code;
			u16 *cur_p;	// [ESP+318]
repeat:
			code = compr[curbyte++];
			if (code == 0xff) {	// 3f1dfd
				code = compr[curbyte++];

				if (code) {	// 3f1e0b

				} else {
					curbyte++;
					// 3f1e83
					y++;
					x += 2;
					if (y < height)
						goto 3f1db8;
				}
			} else if (code == 0xfe) {	// 3f1f47
				unsigned int copy_bytes;
				u16 rep_word;	// [ESP+324]

				cur_p = (u16 *)&cur_line[cur_line_act_uncomprlen];
				copy_bytes = compr[curbyte] + 1;
				rep_word = (u16 *)&compr[curbyte + 1];
				curbyte += 3;

				for (unsigned int i = 0; i < copy_words; i++) {
					cur_p[i] = rep_word;
					cur_line_act_uncomprlen += 2;
				}
				//[ESP+328] = save rep_word
				goto repeat;
			} else if (code < 0x80) {	// 3f206f
				cur_p = &cur_line[cur_line_act_uncomprlen];

			} else {	// 3f1fa1
				cur_p = &cur_line[cur_line_act_uncomprlen];
				code &= 0x7f;
				
				code / 5;
				//[ESP+308] = code % 5 - 2;
rep_word = 乱起八遭
(((rep_word & 0x03e0) >> 5) + (code % 25 - 2)) << 5) 
	| (((code % 5 - 2) + ((rep_word & 0x7c00) >> 10)) << 10)
		| (rep_word & 0x001f) + (code % 125 - 2)
			}

*cur_p ＝ (u16)乱起八遭
cur_line_act_uncomprlen += 2;
goto repeat;
		} else {	// 3f1efb

		}

	}

}
#endif

// 从se.vfs开始调试

struct acui_information SystemAoi_cui_information = {
	_T("SofthouseChara"),		/* copyright */
	_T("SystemAoi Game System"),/* system */
	_T(".vfs"),					/* package */
	_T(""),					/* revision */
	_T("痴漢公賊"),				/* author */
	_T(""),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[2];		// "VF" or "VL"
	u16 version;		// 0x0101
	u16 index_entries;
	u16 index_entry_size;
	u32 index_length;
	u32 vfs_length;
} vfs_header_t;

/* 
00393B46    66:817D 02 0101 CMP WORD PTR SS:[EBP+2],101
00393B4C    56              PUSH ESI
00393B4D    57              PUSH EDI
00393B4E    0F85 D7000000   JNZ AoiLib.00393C2B
00393B54    66:837D 04 05   CMP WORD PTR SS:[EBP+4],5
00393B59    0F86 CC000000   JBE AoiLib.00393C2B
00393B5F    8B7424 14       MOV ESI,DWORD PTR SS:[ESP+14]
00393B63    8B7C24 1C       MOV EDI,DWORD PTR SS:[ESP+1C]
*/
typedef struct {
	s8 name[19];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u8 is_compr;
} vfs_entry_t;
#pragma pack ()

/********************* vfs *********************/

static int SystemAoi_vfs_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "VF", 2) && strncmp(magic, "VL", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int SystemAoi_vfs_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	vfs_header_t vfs_header;

	if (pkg->pio->readvec(pkg, &vfs_header, sizeof(vfs_header),
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_length = vfs_header.index_entries * vfs_header.index_entry_size;
	BYTE *index = new BYTE[index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	if (vfs_header.index_entry_size != sizeof(vfs_entry_t)) {
		delete [] index;
		return -CUI_EMATCH;		
	}

	pkg_dir->index_entries = vfs_header.index_entries;
	pkg_dir->index_entry_length = vfs_header.index_entry_size;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;

	return 0;
}

static int SystemAoi_vfs_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	vfs_entry_t *vfs_entry;

	vfs_entry = (vfs_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, vfs_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = vfs_entry->comprlen;
	pkg_res->actual_data_length = vfs_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = vfs_entry->offset;
	if (vfs_entry->comprlen != vfs_entry->uncomprlen) {
		printf("%d\n",vfs_entry->is_compr);
		exit(0);
	}

	return 0;
}

static int SystemAoi_vfs_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr;
	DWORD comprlen;

	comprlen = (DWORD)pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (strstr(pkg_res->name, ".aog")) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (strstr(pkg_res->name, ".obj")) {
		u32 d1 = *(u32 *)&compr[comprlen - 4];
		u32 d2 = *(u32 *)&compr[comprlen - 8];
		for (DWORD i = 0; i < (comprlen - d2 - 9) / 4; ++i)
			*(u32 *)&compr[i * 4] = ~*(u32 *)&compr[i * 4];
		u32 *ebx = (u32 *)compr;
		for (i = 0; i < d1 / d2; ++i) {
			for (DWORD k = 0; k < d2; ++k) {
				if (compr[comprlen - d2 - 8] == 0xff) {
					ebx += compr + d1 * 4;
				}

			}
		}
	}
	pkg_res->raw_data = compr;

	return 0;
}

static cui_ext_operation SystemAoi_vfs_operation = {
	SystemAoi_vfs_match,				/* match */
	SystemAoi_vfs_extract_directory,	/* extract_directory */
	SystemAoi_vfs_parse_resource_info,	/* parse_resource_info */
	SystemAoi_vfs_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

int CALLBACK SystemAoi_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".vfs"), NULL, 
		NULL, &SystemAoi_vfs_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".iph"), _T(".bmp"), 
//		_T("Inteligent Picture Format 32"), &SystemAoi_iph_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
//			return -1;

	return 0;
}
