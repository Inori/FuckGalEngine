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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Adv_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".GR2 .PAC .VIC"),	/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-1 14:03"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PACK"
	u32 index_entries;
	u32 data_offset;
	u32 reserved;		// 0
} pack_header_t;

typedef struct {
	s8 name[16];
	u32 length;
	u32 offset;
} pack_entry_t;

typedef struct {
	s8 magic[8];	// "*Pola*  "
	u32 uncomprlen;
	u32 unknown[2];
} Pola_header_t;

typedef struct {
	s8 magic[4];	// "GR2_"
	u16 width;
	u16 height;
	u32 reserved;	// -1
	u16 bpp;
	u16 pad;		// 0
} GR2_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

#define get_bit()	\
	bv = flag & 1;	\
	flag >>= 1;	\
	if (!--bits) {	\
		flag = *(u16 *)compr;	\
		compr += 2;	\
		bits = 16;	\
	}

static DWORD __Adv_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							  BYTE *compr)
{
	u16 flag = *(u16 *)compr;
	compr += 2;
	DWORD bits = 16;
	BYTE *orig_uncompr = uncompr;

	while (1) {
		DWORD bv;

		get_bit();
		if (bv)
			*uncompr++ = *compr++;
		else {
			get_bit();
			if (bv) {
				u32 offset = *compr++ - 256;
				u32 cnt = 0;

				get_bit();
				if (!bv)
					cnt += 256;
				get_bit();
				if (!bv) {
					offset -= 512;
					get_bit();
					if (!bv) {
						cnt *= 2;
						get_bit();
						if (!bv)
							cnt += 256;
						offset -= 512;
						get_bit();
						if (!bv) {
							cnt *= 2;
							get_bit();
							if (!bv)
								cnt += 256;
							offset -= 1024;
							get_bit();
							if (!bv) {
								offset -= 2048;
								cnt *= 2;
								get_bit();
								if (!bv)
									cnt += 256;
							}
						}
					}
				}

				offset -= cnt;
				get_bit();
				if (bv)
					cnt = 3;
				else {
					get_bit();
					if (bv)
						cnt = 4;
					else {
						get_bit();
						if (bv)
							cnt = 5;
						else {
							get_bit();
							if (bv)
								cnt = 6;
							else {
								get_bit();
								if (bv) {
									get_bit();
									if (bv)
										cnt = 8;
									else
										cnt = 7;
								} else {
									get_bit();
									if (!bv) {
										cnt = 9;
										get_bit();
										if (bv)
											cnt += 4;
										get_bit();
										if (bv)
											cnt += 2;
										get_bit();
										if (bv)
											++cnt;
									} else
										cnt = *compr++ + 17;
								}
							}
						}
					}
				}

				u8 *src = uncompr + offset;
				for (DWORD i = 0; i < cnt; ++i)
					*uncompr++ = *src++;
			} else {
				u32 offset = *compr++ - 256;
				get_bit();
				if (!bv) {
					if (offset != -1) {
						u8 *src = uncompr + offset;
						*(u16 *)uncompr = *(u16 *)src;
						uncompr += 2;
					} else {
						get_bit();
						if (!bv)
							break;
					}
				} else {
					offset -= 256;
					get_bit();
					if (!bv)
						offset -= 1024;
					get_bit();
					if (!bv)
						offset -= 512;
					get_bit();
					if (!bv)
						offset -= 256;
					BYTE *src = uncompr + offset;
					*(u16 *)uncompr = *(u16 *)src;
					uncompr += 2;
				}
			}
		}
	}

	return uncompr - orig_uncompr;
}

static int Adv_uncompress(BYTE **ret_out, DWORD *ret_out_len, BYTE *in)
{
	Pola_header_t *Pola = (Pola_header_t *)in;

	BYTE *uncompr = new BYTE[Pola->uncomprlen + 4095];
	if (!uncompr)
		return -CUI_EMEM;
	memset(uncompr, 0, Pola->uncomprlen);

	BYTE *compr = in + sizeof(Pola_header_t);
	
	// Q:\ONLYONE2 CHR.GR2中有几张少解压256字节 但不知道问题在哪里
	DWORD act_uncomprlen = __Adv_uncompress(uncompr, Pola->uncomprlen, compr);
	if (act_uncomprlen != Pola->uncomprlen) {
	//	delete [] uncompr;
	//	return -CUI_EUNCOMPR;
	}

	*ret_out = uncompr;
	*ret_out_len = Pola->uncomprlen;

	return 0;
}

/*********************  *********************/

/* 封包匹配回调函数 */
static int Adv_match(struct package *pkg)
{
	pack_header_t pack_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pack_header, sizeof(pack_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(pack_header.magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pack_header.index_entries * sizeof(pack_entry_t) + sizeof(pack_header) 
			!= pack_header.data_offset) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pack_header.reserved) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Adv_extract_directory(struct package *pkg,
								 struct package_directory *pkg_dir)
{
	pack_header_t pack_header;

	if (pkg->pio->readvec(pkg, &pack_header, sizeof(pack_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = pack_header.index_entries * sizeof(pack_entry_t);
	pack_entry_t *index_buffer = new pack_entry_t[pack_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pack_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pack_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Adv_parse_resource_info(struct package *pkg,
								   struct package_resource *pkg_res)
{
	pack_entry_t *pack_entry;

	pack_entry = (pack_entry_t *)pkg_res->actual_index_entry;
	memset(pkg_res->name, 0, sizeof(pkg_res->name));
	strncpy(pkg_res->name, pack_entry->name, 16);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pack_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pack_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Adv_extract_resource(struct package *pkg,
								struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	if (!memcmp(raw, "GR2_", 4)) {
		GR2_header_t *head = (GR2_header_t *)raw;
		BYTE *in = (BYTE *)(head + 1);

		if (MyBuildBMPFile(in, pkg_res->raw_data_length - sizeof(GR2_header_t), 
			NULL, 0, head->width, 0 - head->height, head->bpp * 8, 
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				delete [] raw;
				return -CUI_EMEM;
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!memcmp(raw, "*Pola", 5)) {
		BYTE *p;
		DWORD p_len;

		int ret = Adv_uncompress(&p, &p_len, raw);
		if (ret) {
			delete [] raw;
			return ret;
		}

		if (!memcmp(p, "GR2_", 4)) {
			GR2_header_t *head = (GR2_header_t *)p;
			BYTE *in = (BYTE *)(head + 1);

			if (MyBuildBMPFile(in, p_len - sizeof(GR2_header_t), 
				NULL, 0, head->width, 0 - head->height, head->bpp * 8, 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
					delete [] p;
					delete [] raw;
					return -CUI_EMEM;
			}
			delete [] p;
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else {
			pkg_res->actual_data = p;
			pkg_res->actual_data_length	= p_len;
		}
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int Adv_save_resource(struct resource *res, 
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
static void Adv_release_resource(struct package *pkg,
								 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void Adv_release(struct package *pkg, 
						struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Adv_operation = {
	Adv_match,				/* match */
	Adv_extract_directory,	/* extract_directory */
	Adv_parse_resource_info,/* parse_resource_info */
	Adv_extract_resource,	/* extract_resource */
	Adv_save_resource,		/* save_resource */
	Adv_release_resource,	/* release_resource */
	Adv_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Adv_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".GR2"), NULL, 
		NULL, &Adv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PAC"), NULL, 
		NULL, &Adv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".VIC"), NULL, 
		NULL, &Adv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
