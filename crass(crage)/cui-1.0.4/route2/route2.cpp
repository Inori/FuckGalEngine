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
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information route2_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".MHU .CGF .CG .IAF .SUD .WPF"),		/* package */
	_T("1.0.5"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-31 15:56"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 uncomprlen;				
} MHU_header_t;

typedef struct {
	u32 index_entries;				
} CGF_header_t;

typedef struct {
	s8 name[28];
	u32 offset;
} CGF_entry_t;

typedef struct {
	s8 name[16];
	u32 offset;
} CG_entry_t;

typedef struct {
	u32 orig_x_left;
	u32 orig_y_up;
	u32 orig_x_right;
	u32 orig_y_down;
} CGF_info_t;

typedef struct {
	u8 fake;
	DWORD comprLen;				
} IAF_header_t;

typedef struct {
	DWORD unknown0;
	DWORD unknown1;
	DWORD unknown2;
	DWORD unknown3;
	DWORD uncomprLen;				
} IAF_tailer_t;

typedef struct {
	DWORD comprLen;				
} IAF_old_header_t;

typedef struct {
	DWORD unknown0;	// 0
	DWORD unknown1;	// 0
	DWORD uncomprLen;
} IAF_old_tailer_t;

typedef struct {
	u32 comprLen;	
	u32 uncomprLen;				
} CGF_lzss_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 flag;
} my_cgf_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_iaf_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} SUD_entry_t;

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				break;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen >= uncomprlen)
					return act_uncomprlen;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

#if 1
// 见鬼！
static int rle_uncompress(BYTE *uncompr, DWORD uncomprLen, 
						  BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	
	while (1) {
		BYTE flag;
		BYTE copy_bytes, data;
		unsigned int i;
		
		if (curByte >= comprLen)
			break; 
		
		flag = compr[curByte++];
		if (!flag) {
			if (curByte >= comprLen)
				goto out;
			copy_bytes = compr[curByte++];
			
			for (i = 0; i < copy_bytes; i++) {
				if (curByte >= comprLen)
					goto out;
				if (act_uncomprLen >= uncomprLen)
					goto out;					
				uncompr[act_uncomprLen++] = compr[curByte++];					
			}
		} else {	
			copy_bytes = flag;
			
			if (curByte >= comprLen)
				goto out;	
			data = compr[curByte++];	
						
			for (i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					goto out;					
				uncompr[act_uncomprLen++] = data;					
			}
		}
	}
out:
	return act_uncomprLen;	
}
#else
static int rle_uncompress(BYTE *uncompr, DWORD uncomprLen, 
						  BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	
	while (act_uncomprLen < uncomprLen) {
		BYTE flag;
		BYTE copy_bytes, data;
		unsigned int i;

		flag = compr[curByte++];
		if (!flag) {
			copy_bytes = compr[curByte++];			
			for (i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprLen++] = compr[curByte++];					
		} else {	
			copy_bytes = flag;			
			data = compr[curByte++];						
			for (i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprLen++] = data;					
		}
	}

	return act_uncomprLen;	
}
#endif

/********************* MHU *********************/

/* 封包匹配回调函数 */
static int route2_MHU_match(struct package *pkg)
{
	s32 uncomprlen;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &uncomprlen, sizeof(uncomprlen))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u8 tmp;
	if (pkg->pio->read(pkg, &tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	// 为了区分MKSF.MHU（内容和存在意义不明）
	if (uncomprlen <= 0 || tmp != 0xff) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int route2_MHU_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	MHU_header_t MHU_header;
	u32 MHU_size;

	if (pkg->pio->length_of(pkg, &MHU_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &MHU_header, sizeof(MHU_header),
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD comprlen = MHU_size - sizeof(MHU_header_t);
	BYTE *compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen,
			sizeof(MHU_header), IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	BYTE *uncompr = (BYTE *)malloc(MHU_header.uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	DWORD act_uncomprlen = lzss_uncompress(uncompr, MHU_header.uncomprlen,
		compr, comprlen);
	if (act_uncomprlen != MHU_header.uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = MHU_header.uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation route2_MHU_operation = {
	route2_MHU_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	route2_MHU_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

#if 0
/********************* SD *********************/

/* 封包匹配回调函数 */
static int route2_SD_match(struct package *pkg)
{
	if (!get_options("decSD"))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int route2_SD_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	SIZE_T SD_size;

	if (pkg->pio->length_of(pkg, &SD_size))
		return -CUI_ELEN;

	BYTE *raw = (BYTE *)pkg->pio->readvec_only(pkg, SD_size,
			0, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	BYTE *act = (BYTE *)malloc(SD_size);
	if (!act)
		return -CUI_EMEM;

	for (DWORD i = 0; i < SD_size; i++)
		act[i] = ~raw[i];

	pkg_res->actual_data = act;
	pkg_res->actual_data_length = SD_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation route2_SD_operation = {
	route2_SD_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	route2_SD_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};
#endif

/********************* CGF *********************/

/* 封包匹配回调函数 */
static int route2_CGF_match(struct package *pkg)
{
	u32 index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int route2_CGF_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	CGF_header_t CGF_header;
	u32 CGF_size;

	if (pkg->pio->length_of(pkg, &CGF_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &CGF_header, sizeof(CGF_header),
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD entry_size;
	{
		u32 offset[2];

		if (pkg->pio->readvec(pkg, &offset[0], 4,
				sizeof(CGF_header) + 16, IO_SEEK_SET))
			return -CUI_EREADVEC;
		offset[0] &= ~0xc0000000;

		if (pkg->pio->readvec(pkg, &offset[1], 4,
				sizeof(CGF_header) + 28, IO_SEEK_SET))
			return -CUI_EREADVEC;
		offset[1] &= ~0xc0000000;

		if (offset[0] == sizeof(CGF_header)
				+ CGF_header.index_entries * sizeof(CG_entry_t))
			entry_size = sizeof(CG_entry_t);
		else if (offset[1] == sizeof(CGF_header)
				+ CGF_header.index_entries * sizeof(CGF_entry_t))
			entry_size = sizeof(CGF_entry_t);
		else
			return -CUI_EMATCH;
	}

	DWORD index_length = CGF_header.index_entries * entry_size;
	void *index_buffer = malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_length,
			sizeof(CGF_header), IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	DWORD my_index_length = CGF_header.index_entries * sizeof(my_cgf_entry_t);
	my_cgf_entry_t *my_index_buffer = (my_cgf_entry_t *)malloc(my_index_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}
	memset(my_index_buffer, 0, my_index_length);

	my_cgf_entry_t *entry = my_index_buffer;
	BYTE *p = (BYTE *)index_buffer;
	for (DWORD i = 0; i < CGF_header.index_entries; i++) {
		if (!p[entry_size - 5])
			strcpy(entry->name, (char *)p);
		else
			strncpy(entry->name, (char *)p, entry_size - 4);
		p += entry_size - 4;
		entry->offset = *(u32 *)p;
		entry->flag = entry->offset & 0xc0000000;
		entry->offset &= ~0xc0000000;
		p += 4;
		entry++;
	}
	free(index_buffer);

	entry = my_index_buffer;
	for (i = 0; i < CGF_header.index_entries - 1; i++) {
		entry->length = entry[1].offset - entry->offset;
		entry++;
	}
	entry->length = CGF_size - entry->offset;

	pkg_dir->index_entries = CGF_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_cgf_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int route2_CGF_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_cgf_entry_t *CGF_entry;

	CGF_entry = (my_cgf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, CGF_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = CGF_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = CGF_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int route2_CGF_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	my_cgf_entry_t *CGF_entry = (my_cgf_entry_t *)pkg_res->actual_index_entry;
	DWORD compr_offset;
	if (CGF_entry->flag & 0x80000000)
		compr_offset = sizeof(CGF_info_t);
	else
		compr_offset = 0;

	if (!(CGF_entry->flag & 0x40000000)) {
		CGF_lzss_t *CGF_lzss = (CGF_lzss_t *)(raw + compr_offset);
		BYTE *compr = (BYTE *)(CGF_lzss + 1);
		DWORD comprlen = CGF_lzss->comprLen;
		DWORD uncomprlen = CGF_lzss->uncomprLen;
		DWORD flag = uncomprlen & 0xc0000000;
		uncomprlen &= ~0xc0000000;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		DWORD act_uncomprlen;
		if (flag & 0x80000000)
			act_uncomprlen = rle_uncompress(uncompr, uncomprlen, compr, comprlen);
		else if (flag & 0x40000000) {
			memcpy(uncompr, compr, uncomprlen);
			act_uncomprlen = uncomprlen;
		} else
			act_uncomprlen = lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
		if (act_uncomprlen != uncomprlen) {
			printf("%s: %x %x %x\n",pkg_res->name, flag,act_uncomprlen, uncomprlen);
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else {
		pkg_res->raw_data = raw + compr_offset;
		pkg_res->raw_data_length = pkg_res->raw_data_length - compr_offset;
	}

	return 0;
}

/* 封包资源释放函数 */
static void Lambda_CGF_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包处理回调函数集合 */
static cui_ext_operation route2_CGF_operation = {
	route2_CGF_match,					/* match */
	route2_CGF_extract_directory,		/* extract_directory */
	route2_CGF_parse_resource_info,		/* parse_resource_info */
	route2_CGF_extract_resource,		/* extract_resource */
	cui_common_save_resource,
	Lambda_CGF_release_resource,
	cui_common_release
};

/********************* CG *********************/

static int route2_CG_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	CGF_header_t CGF_header;
	u32 CG_size;

	if (pkg->pio->length_of(pkg, &CG_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &CGF_header, sizeof(CGF_header),
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	// 自探测每个entry的长度
	BYTE tmp[96];
	if (pkg->pio->read(pkg, tmp, sizeof(tmp)))
		return -CUI_EREADVEC;

	// 假定名字字段至少8字节长
	int name_len = 8;
	while (1) {
		// name字段之后应该是数据偏移
		if (*(u32 *)&tmp[name_len] == sizeof(CGF_header) 
				+ CGF_header.index_entries * (name_len + 4))
			break;
		// 假定名字字段一定4字节对齐
		name_len += 4;
	}
	DWORD entry_size = name_len + 4;

	DWORD index_length = CGF_header.index_entries * entry_size;
	BYTE *index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_length, sizeof(CGF_header), IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	DWORD my_index_length = CGF_header.index_entries * sizeof(my_cgf_entry_t);
	my_cgf_entry_t *my_index_buffer = (my_cgf_entry_t *)malloc(my_index_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}
	memset(my_index_buffer, 0, my_index_length);

	my_cgf_entry_t *entry = my_index_buffer;
	BYTE *pp = index_buffer;
	for (DWORD i = 0; i < CGF_header.index_entries; i++) {
		if (!pp[name_len - 1])
			strcpy(entry->name, (char *)pp);
		else
			strncpy(entry->name, (char *)pp, name_len);
		entry->offset = *(u32 *)&pp[name_len];
		entry->flag = entry->offset & 0xc0000000;
		entry->offset &= ~0xc0000000;
		++entry;
		pp += entry_size;
	}
	free(index_buffer);

	entry = my_index_buffer;
	for (i = 0; i < CGF_header.index_entries - 1; i++) {
		entry->length = entry[1].offset - entry->offset;
		entry++;
	}
	entry->length = CG_size - entry->offset;

	pkg_dir->index_entries = CGF_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_cgf_entry_t);

	return 0;
}

static int route2_CG_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;	
}

static cui_ext_operation route2_CG_operation = {
	route2_CGF_match,				/* match */
	route2_CG_extract_directory,	/* extract_directory */
	route2_CGF_parse_resource_info,	/* parse_resource_info */
	route2_CG_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* WPF *********************/

/* 封包匹配回调函数 */
static int route2_WPF_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int route2_WPF_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 WPF_size;

	if (pkg->pio->length_of(pkg, &WPF_size))
		return -CUI_ELEN;

	DWORD offset = 0;
	DWORD index_entries = 0;
	while (offset < WPF_size) {
		u32 length;

		if (pkg->pio->readvec(pkg, &length, 4,
				offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		offset += length + 4;
		++index_entries;
	}

	DWORD my_index_length = index_entries * sizeof(my_cgf_entry_t);
	my_cgf_entry_t *my_index_buffer = (my_cgf_entry_t *)malloc(my_index_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	offset = 0;
	for (DWORD i = 0; i < index_entries; ++i) {
		if (pkg->pio->readvec(pkg, &my_index_buffer[i].length, 4,
				offset, IO_SEEK_SET)) {
			free(my_index_buffer);
			return -CUI_EREADVEC;
		}

		my_index_buffer[i].offset = offset + 4;	
		offset += my_index_buffer[i].length + 4;
		sprintf(my_index_buffer[i].name, "%06d", i);
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_cgf_entry_t);

	return 0;
}

static int route2_WPF_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;	
}

static cui_ext_operation route2_WPF_operation = {
	route2_WPF_match,				/* match */
	route2_WPF_extract_directory,	/* extract_directory */
	route2_CGF_parse_resource_info,	/* parse_resource_info */
	route2_CG_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* IAF *********************/

/* 封包匹配回调函数 */
static int route2_IAF_match(struct package *pkg)
{
	u32 IAF_size;
	BYTE tmp[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->length_of(pkg, &IAF_size)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	DWORD is_old_version;
	if (IAF_size == 1 + 4 + *(u32 *)&tmp[1] + sizeof(IAF_tailer_t))
		is_old_version = 0;
	else if (IAF_size == 1 + 4 + *(u32 *)&tmp[1] + sizeof(IAF_old_tailer_t))
		is_old_version = 2;
	else if (IAF_size == 4 + *(u32 *)&tmp[0] + sizeof(IAF_old_tailer_t))
		is_old_version = 1;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, is_old_version);

	return 0;	
}

/* 封包资源提取函数 */
static int route2_IAF_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 IAF_size;
	DWORD is_old_version = package_get_private(pkg);
	BYTE *raw, *compr;
	DWORD comprlen, uncomprlen;

	if (pkg->pio->length_of(pkg, &IAF_size))
		return -CUI_ELEN;

	raw = (BYTE *)malloc(IAF_size);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, IAF_size, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (!is_old_version) {
		IAF_header_t *IAF_header = (IAF_header_t *)raw;
		IAF_tailer_t *IAF_tailer = (IAF_tailer_t *)(raw + IAF_size - sizeof(IAF_tailer_t));

		comprlen = IAF_header->comprLen;
		uncomprlen = IAF_tailer->uncomprLen;
		compr = (BYTE *)(IAF_header + 1);
	} else if (is_old_version == 2) {
		IAF_header_t *IAF_header = (IAF_header_t *)raw;
		IAF_old_tailer_t *IAF_old_tailer = (IAF_old_tailer_t *)(raw + IAF_size - sizeof(IAF_old_tailer_t));

		comprlen = IAF_header->comprLen;
		uncomprlen = IAF_old_tailer->uncomprLen;
		compr = (BYTE *)(IAF_header + 1);
	} else {
		IAF_old_header_t *IAF_old_header = (IAF_old_header_t *)raw;
		IAF_old_tailer_t *IAF_old_tailer = (IAF_old_tailer_t *)(raw + IAF_size - sizeof(IAF_old_tailer_t));

		comprlen = IAF_old_header->comprLen;
		uncomprlen = IAF_old_tailer->uncomprLen;
		//compr = (BYTE *)(IAF_old_tailer + 1);	// 手误还是？
		compr = (BYTE *)(IAF_old_header + 1);
	}

	DWORD flag = uncomprlen & 0xc0000000;
	uncomprlen &= ~0xc0000000;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(raw);
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen;
	if (flag & 0x80000000)
		act_uncomprlen = rle_uncompress(uncompr, uncomprlen, compr, comprlen);
	else if (flag & 0x40000000) {
		memcpy(uncompr, compr, uncomprlen);
		act_uncomprlen = uncomprlen;
	} else
		act_uncomprlen = lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		free(raw);
		return -CUI_EUNCOMPR;
	}

	// width < 0, 8 bits, only have palette
	if (*(u32 *)&uncompr[18] & 0x80000000) {
		*(u32 *)&uncompr[18] = 0 - *(u32 *)&uncompr[18];		
	}

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = IAF_size;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation route2_IAF_operation = {
	route2_IAF_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	route2_IAF_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* SUD *********************/

/* 封包匹配回调函数 */
static int route2_SUD_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int route2_SUD_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 SUD_size;

	if (pkg->pio->length_of(pkg, &SUD_size))
		return -CUI_ELEN;

	DWORD offset = 0;
	for (DWORD index_entries = 0; offset < SUD_size; index_entries++) {
		u32 ogg_size;

		if (pkg->pio->readvec(pkg, &ogg_size, sizeof(ogg_size),
				offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		offset += sizeof(ogg_size) + ogg_size;
	}

	DWORD index_length = index_entries * sizeof(SUD_entry_t);
	SUD_entry_t *index_buffer = (SUD_entry_t *)malloc(index_length);	
	if (!index_buffer)
		return -CUI_EMEM;

	offset = 0;
	for (DWORD i = 0; i < index_entries; i++) {
		u32 ogg_size;

		if (pkg->pio->readvec(pkg, &ogg_size, sizeof(ogg_size),
				offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_EREADVEC;
		}
		offset += sizeof(ogg_size);
		index_buffer[i].length = ogg_size;
		index_buffer[i].offset = offset;
		sprintf(index_buffer[i].name, "%08d.ogg", i);
		offset += ogg_size;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(SUD_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int route2_SUD_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	SUD_entry_t *SUD_entry;

	SUD_entry = (SUD_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, SUD_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = SUD_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = SUD_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int route2_SUD_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包资源释放函数 */
static void Lambda_SUD_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包处理回调函数集合 */
static cui_ext_operation route2_SUD_operation = {
	route2_SUD_match,					/* match */
	route2_SUD_extract_directory,		/* extract_directory */
	route2_SUD_parse_resource_info,		/* parse_resource_info */
	route2_SUD_extract_resource,		/* extract_resource */
	cui_common_save_resource,
	Lambda_CGF_release_resource,
	cui_common_release
};

/********************* IAF *********************/

/* 封包匹配回调函数 */
static int route2_IAFd_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int route2_IAFd_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	u32 IAF_size;

	if (pkg->pio->length_of(pkg, &IAF_size))
		return -CUI_ELEN;

	DWORD offset = 0;
	DWORD is_old_version = -1;
	DWORD index_entries = 0;
	while (1) {
		u8 fake;
		u32 comprlen;

		if (pkg->pio->readvec(pkg, &fake, 1, offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		if (pkg->pio->read(pkg, &comprlen, 4))
			return -CUI_EREAD;

		offset += 1 + 4 + comprlen + sizeof(IAF_tailer_t);

		if (offset > IAF_size) {
			if (index_entries >= 116) {	// for SKF\CG\TRS09.IAF
				is_old_version = 0;
				break;
			}
			break;
		}
		if (offset == IAF_size) {
			is_old_version = 0;
			break;
		}
		index_entries++;
	}

	if (is_old_version == -1) {
		offset = 0;
		index_entries = 0;
		while (1) {
			u8 fake;
			u32 comprlen;

			if (pkg->pio->readvec(pkg, &fake, 1, offset, IO_SEEK_SET))
				return -CUI_EREADVEC;
			
			if (pkg->pio->read(pkg, &comprlen, 4))
				return -CUI_EREAD;

			offset += 1 + 4 + comprlen + sizeof(IAF_old_tailer_t);

			if (offset > IAF_size)
				break;
			if (offset == IAF_size) {
				is_old_version = 2;
				break;
			}
		}

		if (is_old_version == -1)
			return -CUI_EMATCH;
	}

	DWORD index_length = index_entries * sizeof(my_iaf_entry_t);
	my_iaf_entry_t *index_buffer = (my_iaf_entry_t *)malloc(index_length);	
	if (!index_buffer)
		return -CUI_EMEM;

	offset = 0;
	int ret;
	for (DWORD i = 0; i < index_entries; i++) {
		u8 fake;
		u32 comprlen;

		index_buffer[i].offset = offset;

		if (pkg->pio->readvec(pkg, &fake, 1, offset, IO_SEEK_SET)) {
			ret = -CUI_EREADVEC;
			break;
		}

		if (pkg->pio->read(pkg, &comprlen, 4)) {
			ret = -CUI_EREAD;
			break;
		}

		offset += 1 + 4 + comprlen;

		if (!is_old_version) {
			IAF_tailer_t IAF_tailer;

			if (pkg->pio->read(pkg, &IAF_tailer, sizeof(IAF_tailer))) {
				ret = -CUI_EREAD;
				break;
			}
			index_buffer[i].length = 1 + 4 + comprlen + sizeof(IAF_tailer);
			offset += sizeof(IAF_tailer);
		} else if (is_old_version == 2) {
			IAF_old_tailer_t IAF_old_tailer;

			if (pkg->pio->read(pkg, &IAF_old_tailer, sizeof(IAF_old_tailer))) {
				ret = -CUI_EREAD;
				break;
			}
			index_buffer[i].length = 1 + 4 + comprlen + sizeof(IAF_old_tailer);
			offset += sizeof(IAF_old_tailer);
		}
		sprintf(index_buffer[i].name, "%08d.IAF", i);
	}
	if (i != index_entries) {
		free(index_buffer);
		return ret;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_iaf_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int route2_IAFd_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_iaf_entry_t *my_iaf_entry;

	my_iaf_entry = (my_iaf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_iaf_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_iaf_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_iaf_entry->offset;

	return 0;
}

static int route2_IAFd_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation route2_IAFd_operation = {
	route2_IAFd_match,				/* match */
	route2_IAFd_extract_directory,	/* extract_directory */
	route2_IAFd_parse_resource_info,/* parse_resource_info */
	route2_IAFd_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK route2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".CGF"), NULL, 
		NULL, &route2_CGF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".WPF"), _T(".bmp"), 
		NULL, &route2_WPF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".CG"), NULL, 
		NULL, &route2_CG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".IAF"), _T(".bmp"), 
		NULL, &route2_IAF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".IAF"), NULL, 
		NULL, &route2_IAFd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".SUD"), _T(".ogg"), 
		NULL, &route2_SUD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".MHU"), _T(".MHU.txt"), 
		NULL, &route2_MHU_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".SD"), _T(".SD.dump"), 
//		NULL, &route2_SD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_OPTION))
//			return -1;

	return 0;
}
