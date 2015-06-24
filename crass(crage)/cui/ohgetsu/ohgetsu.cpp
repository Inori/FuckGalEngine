#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>
#include <vector>

/*
Q:\PS2 Torrent\Sony Official PS2 SDK\Sony SDK (PS2)\sce\tools\conftool_20\skbres

找索引位置的方法：
CF下断点 第一个CF封包的地方所在的函数 其调用的第一个子函数就是分发偏移位置的函数。
子函数参数：
int type, int req_id, BYTE *ret_name
返回？

或者exe搜D3DFMT_R3G3B2
其下面0x01000100 0x00000000的地方就是index
 */

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ohgetsu_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".PAC .BIN"),		/* package */
	_T("1.0.4"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-4-19 18:57"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			// "LZS"
	u32 uncomprlen;
} PAC_entry_t;

typedef struct {
	u16 is_compressed;
	u16 resource_id;
	u32 offset;
	u32 length;
} exe_entry_t;

typedef struct {	
	u32 offset;
	u32 length;
	u16 is_compressed;
	u16 resource_id;
} old_exe_entry_t;

typedef struct {	
	u16 unknown;	// 0x899d
	u8 maigc[3];
	u8 comprlen[3];
	u8 unknown1[2];
	u8 uncomprlen[3];
} TAK_BIN_header_t;

typedef struct {
	u32 resource_id;
	u32 offset;		// 2K
	u32 length;
} BIND_entry_t;

typedef struct {	
	s8 maigc[4];		// "WSTR"
	u16 channels;
	u16 bpp;
	u32 freq;
	u32 data_len;
	u32 unknown[4];		// 1,0,0,0
} STR_header_t;
#pragma pack ()

typedef struct {
	TCHAR name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD is_compressed;
	void *release;
} my_pac_entry_t;

struct game_config {
	int is_old;
	BYTE *buffer;
	BYTE *index;
	DWORD index_length;
};

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static BYTE *search_index(BYTE *buf, DWORD buf_len, DWORD size, int old)
{
	BYTE *p = buf;
	BYTE *end_buf;
	int ret = -1;

	if (!old) {
		end_buf = p + (buf_len - (sizeof(exe_entry_t) * 2));
		if (end_buf <= p)
			return NULL;

		while (p < end_buf) {
			exe_entry_t *entry = (exe_entry_t *)p;
			exe_entry_t *next_entry = entry + 1;

			if (!entry->offset && (((entry->length + 2047) & ~2047) == next_entry->offset)
					&& ((s32)entry->length > 0) && (!entry->is_compressed || entry->is_compressed == 1)
					&& (entry->resource_id == 1)) {
				break;
			}
			p += 4;
		}
	} else {
		end_buf = p + (buf_len - (sizeof(old_exe_entry_t) * 2));
		if (end_buf <= p)
			return NULL;

		while (p < end_buf) {
			old_exe_entry_t *entry = (old_exe_entry_t *)p;
			old_exe_entry_t *next_entry = entry + 1;

			if (!entry->offset && (((entry->length + 255) & ~255) == next_entry->offset)
					&& ((s32)entry->length > 0) && (!entry->is_compressed || entry->is_compressed == 1)
					&& (entry->resource_id == 1)) {
				break;
			}
			p += 4;
		}
	}

	return p < end_buf ? p : NULL;
}

static BYTE *get_index(BYTE *index, DWORD buf_len, int old, DWORD size, 
					   DWORD *index_entries)
{
	BYTE *p = index;
	BYTE *end_buf;
	BYTE *base;
	int ret = -1;

	if (!old) {
		end_buf = p + (buf_len - sizeof(exe_entry_t));
		if (end_buf <= p)
			return NULL;

		while (p < end_buf) {
			exe_entry_t *entry = (exe_entry_t *)p;

			if (entry->offset) {
				if (!entry->resource_id && (entry->offset == entry->length) 
						&& (entry->offset == size))
					break;
			} else if (((s32)entry->length > 0) && 
					(!entry->is_compressed || entry->is_compressed == 1)
					&& (entry->resource_id))
				base = p;

			p += 4;
		}
		*index_entries = (p - base) / sizeof(exe_entry_t);
	} else {		
		end_buf = p + (buf_len - sizeof(old_exe_entry_t));
		if (end_buf <= p)
			return NULL;

		while (p < end_buf) {
			old_exe_entry_t *entry = (old_exe_entry_t *)p;

			if (entry->offset) {
				if (!entry->resource_id && (entry->offset == entry->length) 
						&& (entry->offset == size))
					break;
			} else if (((s32)entry->length > 0) && 
					(!entry->is_compressed || entry->is_compressed == 1)
					&& (entry->resource_id))
				base = p;

			p += 4;
		}
		*index_entries = (p - base) / sizeof(old_exe_entry_t);
	}

	return p < end_buf ? p : NULL;
}

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;
	const DWORD win_size = 4096;
	WORD flag = 0;

	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte >= comprlen)
				break;
		}
  
		if (flag & 1) {
			uncompr[act_uncomprlen++] = compr[curbyte++];
			if (act_uncomprlen >= uncomprlen || curbyte >= comprlen)
				break;
		} else {
			DWORD win_pos = (act_uncomprlen + 0xfee) & 0xfff;// 以当前uncompr的指针，虚拟为window的win_pos
			DWORD win_offset = compr[curbyte++];
			DWORD copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			if (curbyte >= comprlen)
				break;

			if (win_offset <= win_pos)
				win_offset = win_pos - win_offset;	// 计算出win_pos和offset之间的长度
			else
				win_offset = win_pos + win_size - win_offset;
			
			BYTE *src = uncompr + act_uncomprlen - win_offset;
			BYTE *src_end = src + copy_bytes;

			while (src < uncompr) {
				uncompr[act_uncomprlen++] = ' ';
				if (act_uncomprlen >= uncomprlen)
					return act_uncomprlen;
				++src;
			}

			while (src < src_end) {
				uncompr[act_uncomprlen++] = *src++;
				if (act_uncomprlen >= uncomprlen)
					return act_uncomprlen;
			}
		}
	}

	return act_uncomprlen;
}

#if 1
#define get_bit()	\
	bv = flag & 1;	\
	flag >>= 1;	\
	if (!--bits) {	\
		flag = *(u16 *)compr;	\
		compr += 2;	\
		bits = 16;	\
	}

static DWORD BIN_uncompress(BYTE *uncompr, DWORD uncomprlen, 
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
#else
static DWORD BIN_uncompress(BYTE *uncompr, DWORD uncomprlen,
							BYTE *compr)
{
	u16 flag = *(u16 *)compr;
	compr += 2;
	int bits = 16;
	BYTE *bak_uncompr = uncompr;

#define get_bit()	\
	f = flag & 1;	\
	flag >>= 1;	\
	if (!--bits) {	\
		flag = *(u16 *)compr;	\
		compr += 2;	\
		bits = 16;	\
	}

	while (1) {
		int f;

		get_bit();  
		if (f)	// 40157e
			*uncompr++ = *compr++;
		else {	// 401587
			get_bit();
			if (f) {	// 40162C
				u32 offset = *compr++ - 256;
				u32 cnt = 0;
				get_bit();
				if (!f)
					cnt += 256;
				get_bit();
				if (!f) {	// 401660
					offset -= 512;
					get_bit();
					if (!f) {
						cnt *= 2;
						get_bit();
						if (!f)
							cnt += 256;
						offset -= 512;
						get_bit();
						if (!f) {	// 4016A3
							cnt *= 2;
							get_bit();
							if (!f)
								cnt += 256;
							offset -= 1024;
							get_bit();
							if (!f) {	// 4016CF
								offset -= 2048;
								cnt *= 2;
								get_bit();
								if (!f)
									cnt += 256;
							}
						}
					}
				}

				offset -= cnt;
				get_bit();
				if (!f) {	// 401707
					get_bit();
					if (!f) {	// 401722
						get_bit();
						if (!f) {	// 40173D
							get_bit();
							if (!f) {	// 401758
								get_bit();
								if (!f) {	// 401788
									get_bit();
									if (!f) {	// 4017A8
										cnt = 9;
										get_bit();
										if (f)
											cnt += 4;
										get_bit();
										if (f)
											cnt += 2;
										get_bit();
										if (f)
											++cnt;
									} else	// 401799
										cnt = *compr++ + 17;	
								} else {	// 401769
									get_bit();
									if (!f)
										cnt = 7;
									else
										cnt = 8;
								}
							} else
								cnt = 6;			
						} else
							cnt = 5;			
					} else
						cnt = 4;
				} else	// 4016FD
					cnt = 3;

				// 4017E7
				BYTE *src = uncompr + offset;
				for (DWORD i = 0; i < cnt; ++i)
					*uncompr++ = *src++;
			} else {	// 40159C
				u32 offset = *compr++ - 256;
				get_bit();
				if (f) {	// 4015D8
					offset -= 256;
					get_bit();
					if (!f)
						offset -= 1024;
					get_bit();
					if (!f)
						offset -= 512;
					get_bit();
					if (!f)
						offset -= 256;
					*(u16 *)uncompr = *(u16 *)(uncompr + offset);
					uncompr += 2;
				} else {	// 4015B7
					if (offset != -1) {	// 4015BC
						*(u16 *)uncompr = *(u16 *)(uncompr + offset);
						uncompr += 2;
					} else {	// 4015C2
						get_bit();
						if (!f)
							break;
					}
				}
			}
		}
	}

	return uncompr - bak_uncompr;
}
#endif

/********************* PAC *********************/

/* 封包匹配回调函数 */
static int ohgetsu_pac_match(struct package *pkg)
{
	int is_old;

	const TCHAR *exe_name = get_options2(_T("exe"));
	if (!exe_name)
		return -CUI_EMATCH;
printf("sdfsdf\n");
	HANDLE file = MyOpenFile(exe_name);
	if (file == INVALID_HANDLE_VALUE)
		return -CUI_EOPEN;

	DWORD fsize;
	if (MyGetFileSize(file, &fsize)) {
		MyCloseFile(file);
		return -CUI_ELEN;
	}

	BYTE *buf = new BYTE[fsize];
	if (!buf) {
		MyCloseFile(file);
		return -CUI_EMEM;
	}

	if (MyReadFile(file, buf, fsize)) {
		delete [] buf;
		MyCloseFile(file);
		return -CUI_EREAD;
	}
	MyCloseFile(file);

	u32 pkg_size;
	if (pkg->pio->open(pkg, IO_READONLY)) {
		delete [] buf;
		return -CUI_EOPEN;
	}
	pkg->pio->length_of(pkg, &pkg_size);

	BYTE *index = search_index(buf, fsize, pkg_size, 0);
	if (!index) {
		index = search_index(buf, fsize, pkg_size, 1);
		if (!index) {
			pkg->pio->close(pkg);
			delete [] buf;
			return -CUI_EMATCH;
		}
		is_old = 1;
	} else
		is_old = 0;

	struct game_config *game_cfg = new struct game_config;
	if (!game_cfg) {
		pkg->pio->close(pkg);
		delete [] buf;
		return -CUI_EMEM;
	}

	game_cfg->is_old = is_old;
	game_cfg->buffer = buf;
	game_cfg->index = index;
	game_cfg->index_length = fsize - (index - buf);
	package_set_private(pkg, game_cfg);

	return 0;	
}

/* 封包索引目录提取函数 */
static int ohgetsu_pac_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	struct game_config *cfg = (struct game_config *)package_get_private(pkg);
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	DWORD index_entries;
	exe_entry_t *exe_entry = (exe_entry_t *)get_index(cfg->index, cfg->index_length, 
		0, fsize, &index_entries);
	if (!exe_entry)
		return -CUI_EMATCH;

	my_pac_entry_t *my_pac_index = new my_pac_entry_t[index_entries];
	if (!index_entries)
		return -CUI_EMEM;

	TCHAR pkg_name[MAX_PATH];
	_tcscpy(pkg_name, pkg->name);
	*_tcsstr(pkg_name, _T(".")) = 0;

	exe_entry -= index_entries;
	for (DWORD i = 0; i < index_entries; ++i) {
		_stprintf(my_pac_index[i].name, _T("%s%05d"), pkg_name, exe_entry->resource_id); 
		my_pac_index[i].length = exe_entry->length;
		my_pac_index[i].is_compressed = exe_entry->is_compressed;
		my_pac_index[i].offset = exe_entry->offset;
		++exe_entry;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_pac_index;
	pkg_dir->directory_length = index_entries * sizeof(my_pac_entry_t);
	pkg_dir->index_entry_length = sizeof(my_pac_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ohgetsu_pac_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	my_pac_entry_t *my_pac_entry;

	my_pac_entry = (my_pac_entry_t *)pkg_res->actual_index_entry;
	_tcscpy((TCHAR *)pkg_res->name, my_pac_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pac_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_pac_entry->offset;
	pkg_res->flags = PKG_RES_FLAG_UNICODE;

	return 0;
}

static void rename_res(struct package *pkg,
					   struct package_resource *pkg_res, BYTE *data)
{
	if (!strncmp((char *)data, "TIM2", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".TM2");
	} else if (!strncmp((char *)data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".OGG");
	} else if (!lstrcmpi(_T("SCRIPT.PAC"), pkg->name) || !lstrcmpi(_T("TAK.BIN"), pkg->name)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".BIN");
	} else if (!lstrcmpi(_T("MOVIE.PAC"), pkg->name)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".MPG");
	} else if (!lstrcmpi(_T("ANIME.PAC"), pkg->name) || !lstrcmpi(_T("ANM.BIN"), pkg->name)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".BIN");
	}
}

/* 封包资源提取函数 */
static int ohgetsu_pac_extract_resource(struct package *pkg,
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

	my_pac_entry_t *my_pac_entry = (my_pac_entry_t *)pkg_res->actual_index_entry;
	// is_compressed字段并非总是有效
	if ((pkg_res->flags & PKG_RES_FLAG_RAW) || strncmp((char *)raw, "LZS", 4)) {
		pkg_res->raw_data = raw;
		rename_res(pkg, pkg_res, raw);
		return 0;
	}

	PAC_entry_t *PAC_entry = (PAC_entry_t *)raw;
	BYTE *compr = (BYTE *)(PAC_entry + 1);
	DWORD uncomprlen = PAC_entry->uncomprlen;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = lzss_uncompress(uncompr, uncomprlen,
		compr, pkg_res->raw_data_length);
	delete [] raw;
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	rename_res(pkg, pkg_res, uncompr);

	return 0;
}

/* 资源保存函数 */
static int ohgetsu_pac_save_resource(struct resource *res, 
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
static void ohgetsu_pac_release_resource(struct package *pkg, 
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
static void ohgetsu_pac_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	struct game_config *cfg = (struct game_config *)package_get_private(pkg);
	if (cfg) {
		delete [] cfg->buffer;
		delete cfg;
		package_set_private(pkg, 0);
	}

	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_pac_operation = {
	ohgetsu_pac_match,					/* match */
	ohgetsu_pac_extract_directory,		/* extract_directory */
	ohgetsu_pac_parse_resource_info,	/* parse_resource_info */
	ohgetsu_pac_extract_resource,		/* extract_resource */
	ohgetsu_pac_save_resource,			/* save_resource */
	ohgetsu_pac_release_resource,		/* release_resource */
	ohgetsu_pac_release					/* release */
};

/********************* BIN *********************/

/* 封包索引目录提取函数 */
static int ohgetsu_bin_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	struct game_config *cfg = (struct game_config *)package_get_private(pkg);
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	DWORD index_entries;
	old_exe_entry_t *exe_entry = (old_exe_entry_t *)get_index(cfg->index, cfg->index_length, 
		1, fsize, &index_entries);
	if (!exe_entry)
		return -CUI_EMATCH;

	my_pac_entry_t *my_pac_index = new my_pac_entry_t[index_entries];
	if (!index_entries)
		return -CUI_EMEM;

	TCHAR pkg_name[MAX_PATH];
	_tcscpy(pkg_name, pkg->name);
	*_tcsstr(pkg_name, _T(".")) = 0;

	exe_entry -= index_entries;
	for (DWORD i = 0; i < index_entries; ++i) {
		_stprintf(my_pac_index[i].name, _T("%s%05d"), pkg_name, exe_entry->resource_id); 
		my_pac_index[i].length = exe_entry->length;
		my_pac_index[i].is_compressed = exe_entry->is_compressed;
		my_pac_index[i].offset = exe_entry->offset;
		++exe_entry;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_pac_index;
	pkg_dir->directory_length = index_entries * sizeof(my_pac_entry_t);
	pkg_dir->index_entry_length = sizeof(my_pac_entry_t);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_bin_operation = {
	ohgetsu_pac_match,					/* match */
	ohgetsu_bin_extract_directory,		/* extract_directory */
	ohgetsu_pac_parse_resource_info,	/* parse_resource_info */
	ohgetsu_pac_extract_resource,		/* extract_resource */
	ohgetsu_pac_save_resource,			/* save_resource */
	ohgetsu_pac_release_resource,		/* release_resource */
	ohgetsu_pac_release					/* release */
};

/********************* STR *********************/

/* 封包匹配回调函数 */
static int ohgetsu_STR_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "WSTR", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}


/* 封包资源提取函数 */
static int ohgetsu_STR_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	STR_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *data = new BYTE[header.data_len];
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, header.data_len, sizeof(header), IO_SEEK_SET)) {
		delete [] data;
		return -CUI_EREADVEC;
	}

	if (MySaveAsWAVE(data, header.data_len, 1, header.channels,
			header.freq, header.bpp, NULL, 0, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] data;
		return -CUI_EMEM;
	}
	delete [] data;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_STR_operation = {
	ohgetsu_STR_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ohgetsu_STR_extract_resource,	/* extract_resource */
	ohgetsu_pac_save_resource,		/* save_resource */
	ohgetsu_pac_release_resource,	/* release_resource */
	ohgetsu_pac_release				/* release */
};

/********************* TAK_BIN *********************/

/* 封包匹配回调函数 */
static int ohgetsu_TAK_BIN_match(struct package *pkg)
{
	u8 data[13];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, data, sizeof(data))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (data[2] != 'i' || data[3] != 'k' || data[4] != 'e') {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}


/* 封包资源提取函数 */
static int ohgetsu_TAK_BIN_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	u32 comprlen;
	pkg->pio->length_of(pkg, &comprlen);

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 0, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	TAK_BIN_header_t *header = (TAK_BIN_header_t *)compr;
	DWORD uncomprlen = ((((header->uncomprlen[0] / 4) << 8) | header->uncomprlen[2]) << 8) | header->uncomprlen[1];
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}
	
	DWORD act_uncomprlen = BIN_uncompress(uncompr, uncomprlen, compr + sizeof(TAK_BIN_header_t));
	delete [] compr;
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_TAK_BIN_operation = {
	ohgetsu_TAK_BIN_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	ohgetsu_TAK_BIN_extract_resource,	/* extract_resource */
	ohgetsu_pac_save_resource,			/* save_resource */
	ohgetsu_pac_release_resource,		/* release_resource */
	ohgetsu_pac_release					/* release */
};

/********************* BIND *********************/

/* 封包匹配回调函数 */
static int ohgetsu_BIND_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	BIND_entry_t entry;

	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	DWORD index_entries = (entry.resource_id >> 16) - 1;
	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, &entry, sizeof(entry), 
			index_entries * sizeof(BIND_entry_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	if (((((entry.offset << 11) + entry.length) + 0x7ff) & ~0x7ff) != fsize) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, index_entries);

	return 0;
}

/* 封包索引目录提取函数 */
static int ohgetsu_BIND_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	DWORD index_entries = package_get_private(pkg);
	BIND_entry_t *BIND_index = new BIND_entry_t[index_entries];
	if (!BIND_index)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, BIND_index, sizeof(BIND_entry_t) * index_entries, 
			sizeof(BIND_entry_t), IO_SEEK_SET)) {
		delete [] BIND_index;
		return -CUI_EREADVEC;
	}

	my_pac_entry_t *my_pac_index = new my_pac_entry_t[index_entries];
	if (!index_entries) {
		delete [] BIND_index;
		return -CUI_EMEM;
	}

	TCHAR pkg_name[MAX_PATH];
	_tcscpy(pkg_name, pkg->name);
	*_tcsstr(pkg_name, _T(".")) = 0;

	for (DWORD i = 0; i < index_entries; ++i) {
		_stprintf(my_pac_index[i].name, _T("%s%04d"), pkg_name, BIND_index[i].resource_id); 
		my_pac_index[i].length = BIND_index[i].length;
		my_pac_index[i].offset = BIND_index[i].offset << 11;
	}
	delete [] BIND_index;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_pac_index;
	pkg_dir->directory_length = index_entries * sizeof(my_pac_entry_t);
	pkg_dir->index_entry_length = sizeof(my_pac_entry_t);

	return 0;
}

/* 封包资源提取函数 */
static int ohgetsu_BIND_extract_resource(struct package *pkg,
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

	BYTE *data;
	DWORD data_len;
	if (raw[2] == 'i' && raw[3] == 'k' && raw[4] == 'e') {
		TAK_BIN_header_t *header = (TAK_BIN_header_t *)raw;
		DWORD uncomprlen = ((((header->uncomprlen[0] / 4) << 8) | header->uncomprlen[2]) << 8) | header->uncomprlen[1];
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}
		
		DWORD act_uncomprlen = BIN_uncompress(uncompr, uncomprlen, raw + sizeof(TAK_BIN_header_t));
		delete [] raw;
		if (act_uncomprlen != uncomprlen) {
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
		data = uncompr;
		data_len = uncomprlen;
	} else {
		pkg_res->raw_data = raw;
		data = raw;
		data_len = pkg_res->raw_data_length;
	}

	if (!strncmp((char *)data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".BMP");
	} else if (!strncmp((char *)data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".WAV");
	}	

	return 0;
}

/* 封包卸载函数 */
static void ohgetsu_BIND_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_BIND_operation = {
	ohgetsu_BIND_match,					/* match */
	ohgetsu_BIND_extract_directory,		/* extract_directory */
	ohgetsu_pac_parse_resource_info,	/* parse_resource_info */
	ohgetsu_BIND_extract_resource,		/* extract_resource */
	ohgetsu_pac_save_resource,			/* save_resource */
	ohgetsu_pac_release_resource,		/* release_resource */
	ohgetsu_BIND_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ohgetsu_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".PAC"), NULL, 
		NULL, &ohgetsu_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_OPTION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".BIN"), NULL, 
		NULL, &ohgetsu_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_OPTION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".BIN"), NULL, 
		NULL, &ohgetsu_BIND_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".BIN"), NULL, 
		NULL, &ohgetsu_TAK_BIN_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".STR"), _T(".WAV"), 
		NULL, &ohgetsu_STR_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
