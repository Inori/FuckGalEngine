#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

// TODO: 存在嵌套型fpk，以后可能要改成fio的方式

/*
0044C330  /$  53            PUSH EBX
0044C331  |.  8B5C24 0C     MOV EBX,DWORD PTR SS:[ESP+C]
0044C335  |.  56            PUSH ESI
0044C336  |.  8B35 EC348300 MOV ESI,DWORD PTR DS:[8334EC]
0044C33C  |.  8B0B          MOV ECX,DWORD PTR DS:[EBX]
0044C33E  |.  57            PUSH EDI
0044C33F  |.  8B7C24 10     MOV EDI,DWORD PTR SS:[ESP+10]
0044C343  |.  83E9 04       SUB ECX,4
0044C346  |.  8D1439        LEA EDX,DWORD PTR DS:[ECX+EDI]
0044C349  |.  03CE          ADD ECX,ESI
0044C34B  |.  3BD7          CMP EDX,EDI
0044C34D  |.  72 22         JB SHORT KazokuAi.0044C371
0044C34F  |.  55            PUSH EBP
0044C350  |>  8B02          /MOV EAX,DWORD PTR DS:[EDX]
0044C352  |.  8D2C0E        |LEA EBP,DWORD PTR DS:[ESI+ECX]
0044C355  |.  2BC1          |SUB EAX,ECX
0044C357  |.  83EA 04       |SUB EDX,4
0044C35A  |.  33C6          |XOR EAX,ESI
0044C35C  |.  83E9 03       |SUB ECX,3
0044C35F  |.  2BF0          |SUB ESI,EAX
0044C361  |.  8942 04       |MOV DWORD PTR DS:[EDX+4],EAX
0044C364  |.  C1E5 07       |SHL EBP,7
0044C367  |.  C1EE 07       |SHR ESI,7
0044C36A  |.  33F5          |XOR ESI,EBP
0044C36C  |.  3BD7          |CMP EDX,EDI
0044C36E  |.^ 73 E0         \JNB SHORT KazokuAi.0044C350
0044C370  |.  5D            POP EBP
0044C371  |>  8B03          MOV EAX,DWORD PTR DS:[EBX]
0044C373  |.  8B7C38 F8     MOV EDI,DWORD PTR DS:[EAX+EDI-8]
0044C377  |.  8D4F 03       LEA ECX,DWORD PTR DS:[EDI+3]
0044C37A  |.  C1E9 02       SHR ECX,2
0044C37D  |.  8D148D 080000>LEA EDX,DWORD PTR DS:[ECX*4+8]
0044C384  |.  3BC2          CMP EAX,EDX
0044C386  |.  74 06         JE SHORT KazokuAi.0044C38E
0044C388  |.  5F            POP EDI
0044C389  |.  5E            POP ESI
0044C38A  |.  33C0          XOR EAX,EAX
0044C38C  |.  5B            POP EBX
0044C38D  |.  C3            RETN
 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MOONHIR_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".fpk .dat"),		/* package */
	_T("1.0.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-1 13:54"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "FPK" */
	s8 version[4];		/* "0100" */
	u32 index_offset;
	u32 index_entries;
	u32 zeroed;
	//s8 *path;
} MOONHIR_header_t;

typedef struct {
	u32 is_encrypt;
	u32 offset;
	DWORD length;
	s8 name[12];	
} MOONHIR_entry_t;

typedef struct {
	s8 magic[4];		/* 	"FBX\x1" */
	s8 suffix[3];		/* "txt", "CPD", "gkx" */
	u8 header_length;	
	u32 comprLen;
	u32 uncomprLen;	
} fbx_header_t;
#pragma pack ()

static DWORD key_table[] = { 0, 0x430a939d, 0xe09a774b, -1 };
static int valid_key = -1;

static int decrypt(BYTE *buf, DWORD len, DWORD key = 0)
{
	DWORD *p = (DWORD *)buf;
	int loop = len / 4;
	DWORD esi = key;
	DWORD ecx = len - 4 + esi;

	for (int i = loop - 1; i >= 0; --i) {
		DWORD ebp;

		p[i] = (p[i] - ecx) ^ esi;
		ebp = (ecx + esi) << 7;
		ecx -= 3;
		esi = ((esi - p[i]) >> 7) ^ ebp;
	}
	return (((*(DWORD *)&buf[len - 8] + 3) & ~3) + 8) == len;
}

//0x444710
static DWORD fbx_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprLen = 0;
	unsigned int ret_flag = 0;

	while (1) {
		BYTE flag;

//		if (curbyte >= comprLen)
//		break;

		flag = compr[curbyte++];
		for (unsigned int i = 0; i < 4; i++) {
			unsigned int k;
			DWORD flag_word, copy_offset, copy_bytes;
			BYTE tmp_flag = (flag >> (i * 2)) & 0x3;
			BYTE flag_value;

			switch (tmp_flag) {
			case 0:
//				if (curbyte >= comprLen)
//					goto out;
				if (act_uncomprLen >= uncomprLen)
					goto out;
				uncompr[act_uncomprLen++] = compr[curbyte++];
				ret_flag = 0;
				break;
			case 1:
//				if (curbyte >= comprLen)
//					goto out;
				copy_bytes = compr[curbyte++] + 2;
				for (k = 0; k < copy_bytes; k++) {
//					if (curbyte >= comprLen)
//						goto out;
					if (act_uncomprLen >= uncomprLen)
						goto out;
					uncompr[act_uncomprLen++] = compr[curbyte++];
				}
				ret_flag = 0;
				break;
			case 2:
//				if (curbyte + 1 >= comprLen)
//					goto out;
				flag_word = compr[curbyte++] << 8;
				flag_word |= compr[curbyte++];
				copy_offset = (flag_word >> 5) & 0x7ff;
				copy_bytes = (flag_word & 0x1f) + 4;
				for (k = 0; k < copy_bytes; k++) {
					if (act_uncomprLen >= uncomprLen)
						goto out;
					uncompr[act_uncomprLen] = uncompr[act_uncomprLen - copy_offset - 1];
					act_uncomprLen++;
				}
				ret_flag = 0;
				break;
			case 3:
//				if (curbyte >= comprLen)
//					goto out;
				flag_value = compr[curbyte++];
				copy_bytes = flag_value & 0x3f;
				flag_value &= 0xc0;
			
				switch (flag_value) {
				case 0x00:
//					if (curbyte >= comprLen)
//						goto out;
					copy_bytes = (copy_bytes << 8) + compr[curbyte++] + 0x102;
					for (k = 0; k < copy_bytes; k++) {
//						if (curbyte >= comprLen)
//							goto out;
						if (act_uncomprLen >= uncomprLen)
							goto out;
						uncompr[act_uncomprLen++] = compr[curbyte++];
					}
					ret_flag = 0;
					break;
				case 0x40:
//					if (curbyte + 1 >= comprLen)
//						goto out;
					flag_word = compr[curbyte++] << 8;
					flag_word |= compr[curbyte++];
					copy_bytes = (copy_bytes << 5) + (flag_word & 0x1f) + 0x24;
					copy_offset = (flag_word >> 5) & 0x7ff;
					for (k = 0; k < copy_bytes; k++) {
						if (act_uncomprLen >= uncomprLen)
							goto out;
						uncompr[act_uncomprLen] = uncompr[act_uncomprLen - copy_offset - 1];
						act_uncomprLen++;
					}
					ret_flag = 0;
					break;
				case 0xc0:
					curbyte += copy_bytes;
					i = 4;
					ret_flag++;
				default:
					if (ret_flag > 1)
						goto out;
				}
			}
		}
	}
out:
	return act_uncomprLen;
}

/********************* fpk *********************/

/* 封包匹配回调函数 */
static int MOONHIR_MOONHIR_match(struct package *pkg)
{
	s8 magic[4];
	s8 version[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "FPK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, version, sizeof(version))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(version, "0100", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MOONHIR_MOONHIR_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	MOONHIR_header_t MOONHIR_header;
	DWORD path_name_len, index_buffer_length;	
	s8 *path_name;
	MOONHIR_entry_t *index_buffer;

	if (pkg->pio->readvec(pkg, &MOONHIR_header, sizeof(MOONHIR_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	path_name_len = MOONHIR_header.index_offset - sizeof(MOONHIR_header);
	path_name = (s8 *)malloc(path_name_len);
	if (!path_name)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, path_name, path_name_len)) {
		free(path_name);
		return -CUI_EREAD;
	}
	free(path_name);

	index_buffer_length = MOONHIR_header.index_entries * sizeof(MOONHIR_entry_t);
	index_buffer = (MOONHIR_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = MOONHIR_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(MOONHIR_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MOONHIR_MOONHIR_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	MOONHIR_entry_t *MOONHIR_entry;

	MOONHIR_entry = (MOONHIR_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, MOONHIR_entry->name, 12);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = MOONHIR_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = MOONHIR_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MOONHIR_MOONHIR_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	MOONHIR_entry_t *MOONHIR_entry;
	BYTE *compr, *uncompr;
	DWORD uncomprlen;

	MOONHIR_entry = (MOONHIR_entry_t *)pkg_res->actual_index_entry;
	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (MOONHIR_entry->is_encrypt) {
		if (valid_key != -1) {
			if (!decrypt(compr, pkg_res->raw_data_length, key_table[valid_key])) {
				if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
					free(compr);
					return -CUI_EREADVEC;
				}
				valid_key == -1;
			}
		}

		if (valid_key == -1) {
			for (DWORD i = 0; key_table[i] != -1; ++i) {
				if (decrypt(compr, pkg_res->raw_data_length, key_table[i]))
					break;

				if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
					free(compr);
					return -CUI_EREADVEC;
				}
			}
			if (key_table[i] == -1) {
				free(compr);
				return -CUI_EMATCH;				
			}
			valid_key = i;
		}
	}

	if (!strncmp((char *)compr, "FBX\x1", 4)) {
		fbx_header_t *fbx = (fbx_header_t *)compr;
		BYTE *fbx_dat;

		uncomprlen = fbx->uncomprLen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;		
		}
			
		fbx_dat = compr + fbx->header_length;
		if (fbx_decompress(uncompr, uncomprlen, fbx_dat, fbx->comprLen) != uncomprlen) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}

		if (!strncmp((char *)uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		}
	// 好像没什么效果
	//	alpha_blending(uncompr + ((BITMAPFILEHEADER *)uncompr)->bfOffBits,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)uncompr + 1))->biWidth,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)uncompr + 1))->biHeight,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)uncompr + 1))->biBitCount);
	} else {
		if (!strncmp((char *)compr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		}
		uncompr = NULL;
		uncomprlen = 0;
	//	alpha_blending(uncompr + ((BITMAPFILEHEADER *)compr)->bfOffBits,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)compr + 1))->biWidth,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)compr + 1))->biHeight,
	//		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)compr + 1))->biBitCount);
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int MOONHIR_MOONHIR_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void MOONHIR_MOONHIR_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void MOONHIR_MOONHIR_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation MOONHIR_MOONHIR_operation = {
	MOONHIR_MOONHIR_match,					/* match */
	MOONHIR_MOONHIR_extract_directory,		/* extract_directory */
	MOONHIR_MOONHIR_parse_resource_info,	/* parse_resource_info */
	MOONHIR_MOONHIR_extract_resource,		/* extract_resource */
	MOONHIR_MOONHIR_save_resource,			/* save_resource */
	MOONHIR_MOONHIR_release_resource,		/* release_resource */
	MOONHIR_MOONHIR_release					/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int MOONHIR_dat_match(struct package *pkg)
{
	BYTE dat[0x1c];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, dat, sizeof(dat))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!decrypt(dat, sizeof(dat))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp((char *)&dat[12], "NMT1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MOONHIR_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *dat;
	u32 fsize;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	dat = (BYTE *)malloc(fsize);
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dat, fsize, 0, IO_SEEK_SET)) {
		free(dat);
		return -CUI_EREADVEC;
	}

	decrypt(dat, 0x1c);

	pkg_res->raw_data = dat;
	pkg_res->raw_data_length = fsize;
	return 0;
}

/* 资源保存函数 */
static int MOONHIR_dat_save_resource(struct resource *res, 
									 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void MOONHIR_dat_release_resource(struct package *pkg, 
										 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void MOONHIR_dat_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation MOONHIR_dat_operation = {
	MOONHIR_dat_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	MOONHIR_dat_extract_resource,	/* extract_resource */
	MOONHIR_dat_save_resource,		/* save_resource */
	MOONHIR_dat_release_resource,	/* release_resource */
	MOONHIR_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MOONHIR_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".fpk"), NULL, 
		NULL, &MOONHIR_MOONHIR_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RECURSION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &MOONHIR_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
