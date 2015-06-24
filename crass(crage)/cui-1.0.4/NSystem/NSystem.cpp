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
#include "md5.h"

/* .MSD资源的key_string搜索方法：
 首先定位wsprintfA的调用，这些调用中格式参数是%s%d的就是key_string.
 或者exe里找GAMECLASS，它前面的就是.
 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information NSystem_cui_information = {
	NULL,					/* copyright */
	_T("NSystem"),			/* system */
	_T(""),					/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-10-14 19:19"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];				// "FJSYS" or "SM2MPX10" or "MGCFILE"
	u32 data_offset;
	u32 name_index_length;
	u32 index_entries;
	u8 reserved[64];			// 0
} fjsys_header_t;

typedef struct {
	u32 name_offset;
	u32 length;
	u32 offset_lo;
	u32 offset_hi;
} fjsys_entry_t;

typedef struct {
	s8 magic[4];		// "MGD " 
	u16 data_offset;
	u16 format;			// < 2 || > 5
	u32 reserved;		// 0
	u16 width;
	u16 height;
	u32 uncomprlen;		// 解码后RGB的长度
	u32 data_length;
	u16 decode_mode;	// 0 - DecodeNone; 1 - DecodeSGD; 2 - DecodePNG
	u8 no_used[66];
} mgd_header_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 length;
	u32 offset;
} my_fjsys_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int DecodeSGD(BYTE **ret_uncompr, DWORD *ret_uncomprlen, mgd_header_t *mgd_header, DWORD comprlen)
{
	BYTE *compr = (BYTE *)(mgd_header + 1);
	BYTE *uncompr, *act_uncompr;
	DWORD uncomprlen, curbyte;
	int len;
	DWORD i;
	u32 pixel;

	comprlen = *(u32 *)compr;
	compr += 4;
	uncomprlen = mgd_header->width * mgd_header->height * 4;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;
	memset(uncompr, 0, uncomprlen);

	act_uncompr = uncompr;
	len = *(u32 *)(&compr[0]);
	curbyte = 4;

	/* 创建alpha */
	while (len > 0) {
		u16 flag;
		u32 alpha;

		flag = *(u16 *)(&compr[curbyte]);
		curbyte += 2;
		len -= 2;
		if (flag & 0x8000) {	/* 拥有连续α的象素 */
			pixel = (flag & 0x7fff) + 1;
			alpha = ((u32)compr[curbyte++]) << 24;
			for (i = 0; i < pixel; i++) {
				*(u32 *)act_uncompr = alpha;
				act_uncompr += 4;
			}
			len--;
		} else {	/* 拥有不连续α的象素 */
			pixel = flag;
			for (i = 0; i < pixel; i++) {
				alpha = ((u32)compr[curbyte++]) << 24;							
				*(u32 *)act_uncompr = alpha;
				act_uncompr += 4;
			}
			len -= pixel;
		}
	}

	/* 创建BGR */
	act_uncompr = uncompr;
	len = *(u32 *)(&compr[curbyte]);
	curbyte += 4;
	while (len > 0) {
		u8 flag;
		u32 bgr;		

		flag = compr[curbyte++];
		len--;
		switch (flag & 0xc0) {
		case 0x80:
			pixel = flag & 0x3f;
			for (i = 0; i < pixel; i++) {
				u32 delta;
				BYTE b, g, r;

				b = act_uncompr[-4];
				g = act_uncompr[-3];
				r = act_uncompr[-2];
				delta = *(u16 *)(&compr[curbyte]);
				curbyte += 2;
				len -= 2;
				if (delta & 0x8000) {
					b += delta & 0x1f;
					g += (delta >> 5) & 0x1f;
					r += (delta >> 10) & 0x1f;
				} else {	
					if (delta & 0x4000) {
						r -= (delta >> 10) & 0xf;
					} else {
						r += (delta >> 10) & 0xf;
					}

					if (delta & 0x0200) {
						g -= (delta >> 5) & 0xf;
					} else {
						g += (delta >> 5) & 0xf;
					}

					if (delta & 0x0010) {
						b -= delta & 0xf;
					} else {
						b += delta & 0xf;
					}
				}
				act_uncompr[0] = b;
				act_uncompr[1] = g;
				act_uncompr[2] = r;
				act_uncompr += 4;
			}
			break;
		case 0x40:	/* 拥有连续相同BGR分量的象素 */
			pixel = (flag & 0x3f) + 1;
			bgr = (u32)(compr[curbyte] | (compr[curbyte + 1] << 8) | (compr[curbyte + 2] << 16));
			curbyte += 3;
			len -= 3;
			for (i = 0; i < pixel; i++) {
				*(u32 *)act_uncompr |= bgr;
				act_uncompr += 4;				
			}
			break;
		case 0:
			pixel = flag;
			for (i = 0; i < pixel; i++) {
				bgr = (u32)(compr[curbyte] | (compr[curbyte + 1] << 8) | (compr[curbyte + 2] << 16));
				curbyte += 3;
				len -= 3;
				*(u32 *)act_uncompr |= bgr;
				act_uncompr += 4;				
			}
			break;
		default:	// ＲＧＢ展開に失敗
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	}
	*ret_uncompr = uncompr;
	*ret_uncomprlen = uncomprlen;
	return 1;
}

static int DecodeNone(BYTE **ret_uncompr, DWORD *ret_uncomprlen, mgd_header_t *mgd_header, DWORD comprlen)
{
	BYTE *raw = (BYTE *)(mgd_header + 1);
	BYTE *actual;
	DWORD actual_length;

	actual_length = *(u32 *)raw;
	raw += 4;
	actual = (BYTE *)malloc(actual_length);
	if (!actual)
		return -CUI_EMEM;

	memcpy(actual, raw, actual_length);
	*ret_uncompr = actual;
	*ret_uncomprlen = actual_length;
	return 0;
}

static int DecodePNG(BYTE **ret_uncompr, DWORD *ret_uncomprlen, mgd_header_t *mgd_header, DWORD comprlen)
{
	int ret;

	ret = DecodeNone(ret_uncompr, ret_uncomprlen, mgd_header, comprlen);
	return ret < 0 ? ret : 2;
}

static int DecodeMain(BYTE **ret_uncompr, DWORD *ret_uncomprlen, BYTE *compr, DWORD comprlen)
{
	mgd_header_t *mgd_header = (mgd_header_t *)compr;
	DWORD uncomprlen;
	BYTE *uncompr;
	int ret;

	if (mgd_header->format >= 2 && mgd_header->format <= 5)
		return -CUI_EMATCH;

	switch (mgd_header->decode_mode) {
	case 0:
		ret = DecodeNone(&uncompr, &uncomprlen, mgd_header, comprlen);
		break;
	case 1:
		ret = DecodeSGD(&uncompr, &uncomprlen, mgd_header, comprlen);
		break;
	case 2:
		ret = DecodePNG(&uncompr, &uncomprlen, mgd_header, comprlen);
		break;
	default:
		ret = -CUI_EMATCH;
	}

	if (ret == 0 || ret == 1) {
		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, mgd_header->width, 
				0 - mgd_header->height, 32, ret_uncompr, ret_uncomprlen, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
		free(uncompr);
	} else if (ret == 2) {
		*ret_uncompr = uncompr;
		*ret_uncomprlen = uncomprlen;
	}

	return ret;
}

/********************* *********************/

/* 封包匹配回调函数 */
static int NSystem_match(struct package *pkg)
{
	s8 magic[8];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "FJSYS", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NSystem_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	fjsys_header_t fjsys_header;
	fjsys_entry_t *index_buffer;
	BYTE *name_buffer;
	my_fjsys_entry_t *my_index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->readvec(pkg, &fjsys_header, sizeof(fjsys_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = fjsys_header.index_entries * sizeof(fjsys_entry_t);
	index_buffer = (fjsys_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = fjsys_header.index_entries * sizeof(my_fjsys_entry_t);
	my_index_buffer = (my_fjsys_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < fjsys_header.index_entries; i++) {
		my_index_buffer[i].length = index_buffer[i].length;
		my_index_buffer[i].offset = index_buffer[i].offset_lo;
	}

	name_buffer = (BYTE *)pkg->pio->read_only(pkg, fjsys_header.name_index_length);
	if (!name_buffer) {
		free(my_index_buffer);
		free(index_buffer);
		return -CUI_EREADONLY;
	}

	for (i = 0; i < fjsys_header.index_entries; i++)
		strcpy(my_index_buffer[i].name, (char *)&name_buffer[index_buffer[i].name_offset]);
	free(index_buffer);

	pkg_dir->index_entries = fjsys_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_fjsys_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NSystem_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_fjsys_entry_t *my_fjsys_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_fjsys_entry = (my_fjsys_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_fjsys_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_fjsys_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_fjsys_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NSystem_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	const char *key_string;
	BYTE *compr, *uncompr;
	DWORD uncomprlen;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	key_string = get_options("key_string");

	if (key_string && strncmp((char *)compr, "MSCENARIO FILE  ", 16) && strstr(pkg_res->name, ".MSD")) {
		DWORD loop = (pkg_res->raw_data_length + 31) & ~31;
		DWORD curbyte = 0;
		for (DWORD i = 0; i < loop; i++) {
			char buf[MAX_PATH];
			md5byte digest[16];
			char xor[32];
			char *p;
			struct MD5Context ctx;

			/*
			MOV ECX,DWORD PTR DS:[5A566C]
			PUSH EBX
			PUSH ECX
			LEA EDX,DWORD PTR SS:[ESP+88]
			PUSH OSANA.0047AB7C
			PUSH EDX
			CALL DWORD PTR DS:[<&USER32.wsprintfA>]
			*/
			wsprintfA(buf, "%s%d", key_string, i);

			MD5Init(&ctx);
			MD5Update(&ctx, (BYTE *)buf, strlen(buf));
			MD5Final(digest, &ctx);

			p = xor;
			for (DWORD k = 0; k < 16; k++) {
				_snprintf(p, 2, "%02x", digest[k]);
				p += 2;
			}

			for (k = 0; k < 32; k++) {
				if (curbyte >= pkg_res->raw_data_length)
					goto out;
				
				((BYTE *)compr)[curbyte++] ^= xor[k];
			}
		}
	out:
		uncompr = NULL;
		uncomprlen = 0;
	} else if (!memcmp(compr, "MGD ", 4)) {
		int ret = DecodeMain(&uncompr, &uncomprlen, compr, pkg_res->raw_data_length);
		if (ret < 0) {
			free(compr);
			return ret;
		}
		
		if (ret == 0 || ret == 1) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else if (ret == 2) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".png");
		}
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int NSystem_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void NSystem_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

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
static void NSystem_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NSystem_operation = {
	NSystem_match,					/* match */
	NSystem_extract_directory,		/* extract_directory */
	NSystem_parse_resource_info,	/* parse_resource_info */
	NSystem_extract_resource,		/* extract_resource */
	NSystem_save_resource,			/* save_resource */
	NSystem_release_resource,		/* release_resource */
	NSystem_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NSystem_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &NSystem_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
