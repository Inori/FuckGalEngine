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

//アトリエかぐや
//ダンジョンクルセイダーズ ～TALES OF DEMON EATER～[OK]
//マジカルウィッチアカデミー～ボクと先生のマジカルレッスン～??
//淫皇覇伝アマツ～白濁の呪印～[OK]
//Schoolぷろじぇくと☆【×】(找不到KaGuYa，但是op里有，system 3.41, “LINK”）
//人妻コスプレ喫茶２[OK]
//姉汁 ～白川三姉妹におまかせ～【×】(“LINK”)
//牝奴隷 ～犯された放課後～[OK]
//瀬里奈[OK]
//マジカルウィッチアカデミー ～ボクと先生のマジカルレッスン～
//裏入学 ～淫液に濡れた教科書～
//ナースにおまかせ
//まほこい ～エッチな魔法で恋×濃しちゃう～
//人妻コスプレ喫茶【×】(找到系统名KaGuYa，同样封包也有.arc，但没有magic “WFL1”,“LINK”）
//最終痴漢電車2???
//人妻コスプレ喫茶??
//最終痴漢電車??
//女教師???
//人形の館 ～淫夢に抱かれたメイドたち～???

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information KaGuYa_cui_information = {
	_T("アトリエかぐや"),	/* copyright */
	NULL,					/* system */
	_T(".arc .ari .bg_ .cg_ .cgw ._sp .bgm .vo* .aps .parts"),	/* package */
	_T("0.6.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2008-1-30 21:15"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/*
finfo.ini结构：
<header>
[0](4)?
<entry>
s8 arc_name[4];	// eg: "bgd"
u32 data_length;
u32 index_entries;
time_t time_stamp;	//localtime()获得修改时间
u32 crc;			// 计算方法不明（在48bf9f()里某出计算出的）
<data>
u16 name_length;
s8 *name;	//(后缀名与arc_name相同，名字部分和arc里的名字一致)
u32 offset；	// 实际数据（不包含资源名）在arc中的偏移
u32 length;		// 纯数据长度
u16 ??;	// 0
*/

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "WFL1" */
} arc_header_t;

typedef struct {
	u32 name_length;
	s8 *name;
	u16 type;			/* 2 - 明文; 1 - lz77; 0 - etc(need more process); bit0 - is_compressed */
	u32 length;
} ari_entry_t;

typedef struct {
	s8 magic[2];		/* "AP" */
	u32 width;
	u32 height;
	u16 bpp;
} ap_header_t;

typedef struct {	// scr中一个个命令基本格式是：op码+命令长度（包含op码和自身）+参数+额外数据长度，15命令就是字符串，内容取反即可
	u8 magic[8];
	u32 length;
} scr_header_t;

typedef struct {
	DWORD id;
	DWORD start_height;
	DWORD width;
	DWORD end_height;
	DWORD cdepth;
} prs_info_t;

#if 0
typedef struct {
	u16 orig_parts;
	//[ contains orig_parts count
	u32 orig_path_length;
	s8 *orig_path;
	//] 
	u16 parts;
	//[ contains parts count
	u32 desc_length;
	s8 *desc;
	u16 part_index;			// from 0
	u16 sub_parts_entries;	// contain a last dummy entry
	u32 sub_part_index;		// from 0
	....
	u32 last_part_index;	// -1
		//[ contains sub_parts_entries count
		u32 start_x;		// last entry is 0
		u32 start_x;		// last entry is 0
		u32 end_y;			// last entry is 0
		u32 end_y;			// last entry is 0
		u64 zero;
		u32 bpp;			// last entry is -1 or -2
		//]
	//]
	u16 type;
	u32 comprlen;
	u32 uncomprlen;
	s8 *compr;
} parts_struct_t;
#endif

typedef struct {
	s8 magic[5];		// "LINK3"
	s8 name[3];			// 与封包名相同
} link3_header_t;

typedef struct {
	u32 length;
	u16 is_compressed;
	u16 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
	u8 name_length;
	u16 unknown1;
} link3_entry_t;

typedef struct {
	s8 magic[5];		// "LINK5"
	s8 name[5];			// 与封包名相同
} link5_header_t;

typedef struct {
	s8 magic[4];		/* "AP-0" */
	u32 width;
	u32 height;
} ap0_header_t;

typedef struct {
	s8 magic[4];		/* "AP-2" */
	u32 orig_x;			// 显示位置的原点坐标
	u32 orig_y;
	u32 width;
	u32 height;
	u32 sizeof_header;
} ap2_header_t;

typedef struct {
	s8 magic[4];		/* "AN00" */
	u32 orig_x;			// 显示位置的原点坐标(播放时不使用)
	u32 orig_y;
	u32 width;
	u32 height;
	u16 frames;
	u8 play_mode;		// 0 - 只播放第一帧；1 - 播放所有帧
	u8 pad;
	// play list
	u16 play_time;		// 大概以25ms为一个单位
	u16 frame_num;
	//.....
	u16 _frames;
	u32 _x;				// 播放时参数，x,y是横纵向平移间隔
	u32 _y;				
	u32 _width;
	u32 _height;
} an00_header_t;

typedef struct {
	s8 magic[3];
	u8 pad;
	u32 uncomprlen2;
	u32 unknown0;
	u32 uncomprlen;
	u32 comprlen;
} bmr_header_t;

#if 0
typedef struct {
	s8 magic[8];		// "[MESNAM]"
	u16 ;
	u16 ;
	u16 
} mes_header_t;
#endif
#pragma pack ()

typedef struct {
	s8 name[256];
	u16 type;
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} my_ari_entry_t;

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD is_compressed;
} my_link_entry_t;

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD width;
	DWORD height;
} anm_entry_t;




static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_be(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << (7 - pos)));
}

static DWORD lz_uncompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	DWORD curbit = 0;
	/* compr中的当前扫描字节 */
	DWORD curbyte = 0;
	DWORD nCurWindowByte = 1;
	BYTE window[4096];
	
	memset(window, 0, sizeof(window));
	while (1) {
		DWORD i;
		
		if (curbit == 8) {	
			curbit = 0;						
			curbyte++;
		}
		/* 如果为1, 表示接下来的1个字节原样输出 */
		if (getbit_be(compr[curbyte], curbit++)) {
			BYTE data = 0;
			
			for (i = 0; i < 8; i++) {
				if (curbit == 8) {	
					curbit = 0;						
					curbyte++;
				}				
				data |= getbit_be(compr[curbyte], curbit++) << (7 - i);
			}
			/* 输出1字节非压缩数据 */
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			window[nCurWindowByte++] = data;
			nCurWindowByte &= sizeof(window) - 1;
		} else {
			DWORD copy_bytes, win_offset = 0;
			
			/* 该循环次数由窗口大小决定 */
			/* 得到窗口内压缩数据的索引 */
			for (i = 0; i < 12; i++) {
				if (curbit == 8) {	
					curbit = 0;						
					curbyte++;
				}
				win_offset |= getbit_be(compr[curbyte], curbit++) << (11 - i);
			}
			if (!win_offset)
				goto out;
			
			copy_bytes = 0;
			for (i = 0; i < 4; i++) {
				if (curbit == 8) {	
					curbit = 0;						
					curbyte++;
				}
				copy_bytes |= getbit_be(compr[curbyte], curbit++) << (3 - i);
			}
			copy_bytes += 2;			
			for (i = 0; i < copy_bytes; i++) {
				BYTE data;
									
				data = window[(win_offset + i) & (sizeof(window) - 1)];
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				window[nCurWindowByte++] = data;
				nCurWindowByte &= sizeof(window) - 1;		
			}
		}
	}
out:	
	return act_uncomprlen;	
}


static int huffman_tree_create(struct bits *bits, u16 children[2][255], 
							   unsigned int *index, u16 *retval)
{
	unsigned int bitval;
	u16 child;

	if (bit_get_high(bits, &bitval))
		return -1;
	
	if (bitval) {
		unsigned int parent;

		parent = *index;
		*index = parent + 1;
					
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[0][parent - 256] = child;
		
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[1][parent - 256] = child;

		child = parent;
	} else {
		unsigned int byteval;
		
		if (bits_get_high(bits, 8, &byteval))
			return -1;
		
		child = byteval;			
	}
	*retval = child;
	
	return 0;
}

static int huffman_uncompress(unsigned char *uncompr, unsigned long *uncomprlen,
					 unsigned char *compr, unsigned long comprlen)
{
	struct bits bits;
	u16 children[2][255];
	unsigned int index = 256;
	unsigned long max_uncomprlen;
	unsigned long act_uncomprlen;
	unsigned int bitval;
	u16 retval;	
	
	bits_init(&bits, compr, comprlen);
	if (huffman_tree_create(&bits, children, &index, &retval))
		return -1;
	if (retval != 256)
		return -1;

	index = 0;	/* 从根结点开始遍历 */
	act_uncomprlen = 0;
	max_uncomprlen = *uncomprlen;
	while (!bit_get_high(&bits, &bitval)) {
		if (bitval)
			retval = children[1][index];
		else
			retval = children[0][index];
	
		if (retval >= 256)
			index = retval - 256;
		else {
			if (act_uncomprlen >= max_uncomprlen)
				break;
			uncompr[act_uncomprlen++] = (u8)retval;
			index = 0;	/* 返回到根结点 */
		}
	}
	*uncomprlen = act_uncomprlen;

	return 0;
}


/********************* arc *********************/

/* 封包匹配回调函数 */
static int KaGuYa_arc_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "WFL1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_arc_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	my_ari_entry_t *my_index_buffer;
	DWORD my_index_length;
	u32 arc_size;	
	DWORD i, index_entries;

	if (pkg->pio->length_of(pkg, &arc_size))
		return -CUI_ELEN;

	DWORD offset = sizeof(arc_header_t);
	for (i = 0; offset < arc_size; i++) {
		u32 name_len;
		u16 mode;
		u32 length;

		if (pkg->pio->readvec(pkg, &name_len, 4, offset, IO_SEEK_SET))
			break;

		offset += 4 + name_len;

		if (pkg->pio->readvec(pkg, &mode, 2, offset, IO_SEEK_SET)) 
			break;

		offset += 2;

		if (pkg->pio->readvec(pkg, &length, 4, offset, IO_SEEK_SET)) 
			break;

		offset += 4 + length;
		if (mode == 1)
			offset += 4;
	}
	if (offset != arc_size)
		return -CUI_EREADVEC;

	index_entries = i;
	my_index_length = index_entries * sizeof(my_ari_entry_t);
	my_index_buffer = (my_ari_entry_t *)malloc(my_index_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	offset = sizeof(arc_header_t);
	for (i = 0; i < index_entries; i++) {
		u32 name_len;

		if (pkg->pio->readvec(pkg, &name_len, 4, offset, IO_SEEK_SET))
			break;

		if (pkg->pio->read(pkg, my_index_buffer[i].name, name_len))
			break;

		for (u32 k = 0; k < name_len; k++)
			my_index_buffer[i].name[k] = ~my_index_buffer[i].name[k];
		my_index_buffer[i].name[name_len] = 0;

		if (pkg->pio->read(pkg, &my_index_buffer[i].type, 2))
			break;

		if (pkg->pio->read(pkg, &my_index_buffer[i].comprlen, 4))
			break;

		if (my_index_buffer[i].type == 1) {
			if (pkg->pio->read(pkg, &my_index_buffer[i].uncomprlen, 4))
				break;
			offset += 4;
		} else
			my_index_buffer[i].uncomprlen = 0;

		offset += 4 + name_len + 2 + 4;
		my_index_buffer[i].offset = offset;
		offset += my_index_buffer[i].comprlen;
	}
	if (i != index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_ari_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int KaGuYa_arc_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_ari_entry_t *my_ari_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_ari_entry = (my_ari_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_ari_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_ari_entry->comprlen;
	pkg_res->actual_data_length = my_ari_entry->uncomprlen;
	pkg_res->offset = my_ari_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_arc_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_ari_entry_t *my_ari_entry;
	DWORD comprlen, uncomprlen;
	BYTE *compr, *uncompr;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}
		uncompr = NULL;
		uncomprlen = 0;
		goto out;
	}

	my_ari_entry = (my_ari_entry_t *)pkg_res->actual_index_entry;
	if (my_ari_entry->type == 1) {	/* .cgw, .sp_, .cg_, .bg_ */
		if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (lz_uncompress(uncompr, uncomprlen, compr, comprlen) != uncomprlen) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}
	} else if (my_ari_entry->type == 2) {	/* .bgm(ogg), .wav(ogg) */
		if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}
		uncompr = NULL;
	} else if (my_ari_entry->type == 0) {	/* .scr .parts(.aps) */
		if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}
		uncompr = NULL;
	}
out:
	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_arc_operation = {
	KaGuYa_arc_match,				/* match */
	KaGuYa_arc_extract_directory,	/* extract_directory */
	KaGuYa_arc_parse_resource_info,	/* parse_resource_info */
	KaGuYa_arc_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* arc_lst *********************/

/* 封包匹配回调函数 */
static int KaGuYa_arc_lst_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "WFL1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_arc_lst_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	my_ari_entry_t *my_index_buffer;
	DWORD my_index_length;
	u32 index_buffer_length;	
	DWORD i, index_entries;

	if (pkg->pio->length_of(pkg->lst, &index_buffer_length))
		return -CUI_ELEN;

	index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	BYTE *p_lst = index_buffer;
	for (i = 0; p_lst < index_buffer + index_buffer_length; i++) {
		u32 name_len;
		u16 mode;

		name_len = *(u32 *)p_lst;
		p_lst += 4 + name_len;
		mode = *(u16 *)p_lst;
		p_lst += 2 + 4;
	}

	index_entries = i;
	my_index_length = index_entries * sizeof(my_ari_entry_t);
	my_index_buffer = (my_ari_entry_t *)malloc(my_index_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	p_lst = index_buffer;
	DWORD offset = sizeof(arc_header_t);
	for (i = 0; i < index_entries; i++) {
		u32 name_len;

		name_len = *(u32 *)p_lst;
		p_lst += 4;
		strncpy(my_index_buffer[i].name, (char *)p_lst, name_len);
		for (u32 k = 0; k < name_len; k++)
			my_index_buffer[i].name[k] = ~my_index_buffer[i].name[k];
		my_index_buffer[i].name[name_len] = 0;
		p_lst += name_len;
		my_index_buffer[i].type = *(u16 *)p_lst;
		p_lst += 2;
		my_index_buffer[i].comprlen = *(u32 *)p_lst;
		p_lst += 4;
		if (my_index_buffer[i].type == 1) {
			my_index_buffer[i].uncomprlen = *(u32 *)p_lst;
			offset += 4;
		} else
			my_index_buffer[i].uncomprlen = 0;
		offset += 4 + name_len + 2 + 4;
		my_index_buffer[i].offset = offset;
		offset += my_index_buffer[i].comprlen;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_ari_entry_t);

	return 0;
}

/* 封包卸载函数 */
static void KaGuYa_arc_lst_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_arc_lst_operation = {
	KaGuYa_arc_lst_match,				/* match */
	KaGuYa_arc_lst_extract_directory,	/* extract_directory */
	KaGuYa_arc_parse_resource_info,		/* parse_resource_info */
	KaGuYa_arc_extract_resource,		/* extract_resource */
	cui_common_save_resource,			/* save_resource */
	cui_common_release_resource,		/* release_resource */
	KaGuYa_arc_lst_release				/* release */
};

#if 0
/* 文字index */
/********************* tblstr.arc *********************/

/* 封包匹配回调函数 */
static int KaGuYa_tblstr_arc_lst_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_tblstr_arc_lst_extract_directory(struct package *pkg,
												   struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	dat_entry_t *index = index_buffer;
	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (pkg->pio->read(pkg, &index->name_length, 4))
			break;

		if (pkg->pio->read(pkg, index->name, index->name_length))
			break;
		index->name[index->name_length] = 0;

		if (pkg->pio->read(pkg, &index->offset, 4))
			break;

		if (pkg->pio->read(pkg, &index->length, 4))
			break;

		index++;
	}
	if (i != pkg_dir->index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int KaGuYa_arc_lst_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_ari_entry_t *my_ari_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_ari_entry = (my_ari_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_ari_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_ari_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_ari_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_arc_lst_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

/* 资源保存函数 */
static int KaGuYa_arc_lst_save_resource(struct resource *res, 
										struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void KaGuYa_arc_lst_release_resource(struct package *pkg, 
											struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void KaGuYa_arc_lst_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_tblstr_arc_lst_operation = {
	KaGuYa_tblstr_arc_lst_match,					/* match */
	KaGuYa_tblstr_arc_lst_extract_directory,		/* extract_directory */
	KaGuYa_tblstr_arc_lst_parse_resource_info,		/* parse_resource_info */
	KaGuYa_tblstr_arc_lst_extract_resource,		/* extract_resource */
	KaGuYa_tblstr_arc_lst_save_resource,			/* save_resource */
	KaGuYa_tblstr_arc_lst_release_resource,		/* release_resource */
	KaGuYa_tblstr_arc_lst_release					/* release */
};
#endif

/********************* .bg_ *********************/

/* 封包匹配回调函数 */
static int KaGuYa_bg_match(struct package *pkg)
{
	ap_header_t ap_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &ap_header, sizeof(ap_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(ap_header.magic, "AP", 2) && strncmp(ap_header.magic, "BM", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	if (!strncmp(ap_header.magic, "AP", 2) && (ap_header.bpp != 24)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_bg_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *orig;
	DWORD orig_len;
	u32 bg_size;
	ap_header_t *ap_header;
	
	if (pkg->pio->length_of(pkg, &bg_size))
		return -CUI_ELEN;
	
	orig_len = bg_size;
	orig = (BYTE *)malloc(orig_len);
	if (!orig)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, orig, orig_len, 0, IO_SEEK_SET)) {
		free(orig);
		return -CUI_EREADVEC;
	}

	ap_header = (ap_header_t *)orig;
	if (!strncmp((char *)orig, "AP", 2)) {
		if (MySaveAsBMP((BYTE *)(ap_header + 1), orig_len - sizeof(ap_header_t), 
				NULL, 0, ap_header->width, ap_header->height, 32, 0,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(orig);
			return -CUI_EMEM;
		}
	}

	pkg_res->raw_data = orig;
	pkg_res->raw_data_length = orig_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_bg_operation = {
	KaGuYa_bg_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	KaGuYa_bg_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* .ogg *********************/

/* 封包匹配回调函数 */
static int KaGuYa_ogg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "OggS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_ogg_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *ogg;
	u32 ogg_size;
	
	if (pkg->pio->length_of(pkg, &ogg_size))
		return -CUI_ELEN;

	ogg = (BYTE *)malloc(ogg_size);
	if (!ogg)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ogg, ogg_size, 0, IO_SEEK_SET)) {
		free(ogg);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = ogg;
	pkg_res->raw_data_length = ogg_size;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = 0;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_ogg_operation = {
	KaGuYa_ogg_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	KaGuYa_ogg_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* .aps *********************/

/* 封包匹配回调函数 */
static int KaGuYa_aps_match(struct package *pkg)
{
	const char *skip;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	skip = get_options("CRUSADER");
	if (skip) {
		u32 aps_size;

		if (pkg->pio->length_of(pkg, &aps_size)) {
			pkg->pio->close(pkg);
			return -CUI_ELEN;
		}
		
		if (aps_size == 7200) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	}

	return 0;	
}

/* 封包资源提取函数 */
static int KaGuYa_aps_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	DWORD comprlen, uncomprlen;
	BYTE *aps, *compr, *uncompr;
	u32 aps_size;

	if (pkg->pio->length_of(pkg, &aps_size))
		return -CUI_ELEN;

	aps = (BYTE *)malloc(aps_size);
	if (!aps)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, aps, aps_size, 0, IO_SEEK_SET)) {
		free(aps);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = aps;
		pkg_res->raw_data_length = aps_size;
		pkg_res->actual_data = NULL;
		pkg_res->actual_data_length = 0;
		return 0;		
	}

	if (!strncmp((char *)aps, "AP", 2)) {
		ap_header_t *ap_header = (ap_header_t *)aps;
		if (MySaveAsBMP((BYTE *)(ap_header + 1), aps_size - sizeof(ap_header_t), 
				NULL, 0, ap_header->width, ap_header->height, 32, 0,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(aps);
			return -CUI_EMEM;
		}
	} else if (!strncmp((char *)aps, "BM", 2)) {
		pkg_res->actual_data = NULL;
		pkg_res->actual_data_length = 0;
	} else {
		u16 entries, parts, type;
		BYTE *p = aps;
		
		entries = *(u16 *)p;
		p += 2;
		for (u16 i = 0; i < entries; i++) {
			u32 path_len = *(u32 *)p;
			p += 4 + path_len;	
		}		

		parts = *(u16 *)p;
		p += 2;	

		for (i = 0; i < parts; i++) {
			u32 desc_len;
			u16 flag, sub_parts;

			desc_len = *(u32 *)p;
			p += 4 + desc_len;

			flag = *(u16 *)p;
			p += 2;

			sub_parts = *(u16 *)p;
			p += 2;

			for (u16 j = 0; j < sub_parts; j++)
				p += 4;

			for (j = 0; j < sub_parts; j++) {
				p += 4;	/* start width */
				p += 4;	/* start height */
				p += 4;	/* end width */
				p += 4;	/* end height */
				p += 8;	/* pad */
				p += 4;	/* color depth */
			}	
		}

		type = *(u16 *)p;
		if (type != 1) {
			printf("unknown type %x\n", type);
		}
		p += 2;
		comprlen = *(u32 *)p;
		p += 4;
		uncomprlen = *(u32 *)p;
		p += 4;	
		compr = p;
		comprlen = aps_size - (p - aps);
		uncompr = (BYTE *)malloc(uncomprlen);	
		if (!uncompr) {
			free(aps);
			return -CUI_EMEM;
		}	

		lz_uncompress(uncompr, uncomprlen, compr, comprlen);
		ap_header_t *ap_header = (ap_header_t *)uncompr;
		if (MySaveAsBMP((BYTE *)(ap_header + 1), uncomprlen - sizeof(ap_header_t), 
				NULL, 0, ap_header->width, ap_header->height, 32, 0,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(aps);
			return -CUI_EMEM;
		}
		free(uncompr);
	}

	pkg_res->raw_data = aps;
	pkg_res->raw_data_length = aps_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_aps_operation = {
	KaGuYa_aps_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	KaGuYa_aps_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* link3 *********************/

/* 封包匹配回调函数 */
static int KaGuYa_link3_match(struct package *pkg)
{
	s8 magic[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "LINK3", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, sizeof(link3_header_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_link3_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	u32 arc_size;	

	if (pkg->pio->length_of(pkg, &arc_size))
		return -CUI_ELEN;

	DWORD offset = sizeof(link3_header_t);
	for (DWORD index_entries = 0; offset < arc_size - 4; index_entries++) {
		link3_entry_t entry;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			break;

		offset += entry.length;
	}
	if (offset != arc_size - 4)
		return -CUI_EREADVEC;

	u32 eof;
	if (pkg->pio->readvec(pkg, &eof, sizeof(eof), offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (eof)
		return -CUI_EMATCH;

	DWORD my_index_length = sizeof(my_link_entry_t) * index_entries;
	my_link_entry_t *my_index_buffer = new my_link_entry_t[index_entries];
	if (!my_index_buffer)
		return -CUI_EMEM;

	offset = sizeof(link3_header_t);
	for (DWORD i = 0; i < index_entries; i++) {
		link3_entry_t entry;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			break;

		if (pkg->pio->read(pkg, my_index_buffer[i].name, entry.name_length))
			break;
		my_index_buffer[i].name[entry.name_length] = 0;
		my_index_buffer[i].offset = offset + sizeof(entry) + entry.name_length;
		my_index_buffer[i].length = entry.length - entry.name_length - sizeof(entry);
		my_index_buffer[i].is_compressed = entry.is_compressed;
		offset += entry.length;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_link_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int KaGuYa_link3_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_link_entry_t *my_link_entry;

	my_link_entry = (my_link_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_link_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_link_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_link_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_link3_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (!strncmp((char *)raw, "AP-0", 4)) {
		ap0_header_t *ap0_header = (ap0_header_t *)raw;

		DWORD act_len = ap0_header->width * ap0_header->height;
		BYTE *act = (BYTE *)(ap0_header + 1);	
		if (MyBuildBMPFile(act, act_len, NULL, 0, ap0_header->width, 
				ap0_header->height, 8, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
		delete [] raw;
	} else if (!strncmp((char *)raw, "AP-2", 4)) {
		ap2_header_t *ap2_header = (ap2_header_t *)raw;

		DWORD pixel_count = ap2_header->width * ap2_header->height;
		DWORD *src = (DWORD *)(ap2_header + 1);
		DWORD act_len = pixel_count * 4;
		DWORD *act = (DWORD *)new BYTE[act_len];
		for (DWORD i = 0; i < pixel_count; i++)
			act[i] = src[i];
	
		if (MyBuildBMPFile((BYTE *)act, act_len, NULL, 0, ap2_header->width, 
				ap2_header->height, 32, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] act;
			delete [] raw;
			return -CUI_EMEM;
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
		delete [] act;
		delete [] raw;
	} else if (!strncmp((char *)raw, "BMR", 3)) {
		bmr_header_t *bmr_header = (bmr_header_t *)raw;

		BYTE *uncompr = new BYTE[bmr_header->uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}
		BYTE *compr = (BYTE *)(bmr_header + 1);
		DWORD act_uncomprLen = bmr_header->uncomprlen;
		if (huffman_uncompress(uncompr, &act_uncomprLen, compr, bmr_header->comprlen)) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EUNCOMPR;
		}
		if (act_uncomprLen != bmr_header->uncomprlen) {
			delete [] uncompr;
			delete [] raw;			
			return -CUI_EUNCOMPR;			
		}
		printf("ok\n");
		MySaveFile(_T("ddff"), uncompr, act_uncomprLen);
		exit(0);
	} else
		pkg_res->raw_data = raw;

	return 0;
}

static int KaGuYa_link3_save_resource(struct resource *res, 
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
static void KaGuYa_link3_release_resource(struct package *pkg, 
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
static void KaGuYa_link3_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_link3_operation = {
	KaGuYa_link3_match,					/* match */
	KaGuYa_link3_extract_directory,		/* extract_directory */
	KaGuYa_link3_parse_resource_info,	/* parse_resource_info */
	KaGuYa_link3_extract_resource,		/* extract_resource */
	KaGuYa_link3_save_resource,
	KaGuYa_link3_release_resource,
	KaGuYa_link3_release
};

/********************* link5 *********************/

/* 封包匹配回调函数 */
static int KaGuYa_link5_match(struct package *pkg)
{
	s8 magic[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "LINK5", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, sizeof(link5_header_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_link5_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	u32 arc_size;	

	if (pkg->pio->length_of(pkg, &arc_size))
		return -CUI_ELEN;

	DWORD offset = sizeof(link5_header_t);
	for (DWORD index_entries = 0; offset < arc_size - 4; index_entries++) {
		link3_entry_t entry;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			break;

		offset += entry.length;
	}
	if (offset != arc_size - 4)
		return -CUI_EREADVEC;

	u32 eof;
	if (pkg->pio->readvec(pkg, &eof, sizeof(eof), offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (eof)
		return -CUI_EMATCH;

	DWORD my_index_length = sizeof(my_link_entry_t) * index_entries;
	my_link_entry_t *my_index_buffer = (my_link_entry_t *)malloc(my_index_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	offset = sizeof(link5_header_t);
	for (DWORD i = 0; i < index_entries; i++) {
		link3_entry_t entry;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			break;

		if (pkg->pio->read(pkg, my_index_buffer[i].name, entry.name_length))
			break;
		my_index_buffer[i].name[entry.name_length] = 0;
		my_index_buffer[i].offset = offset + sizeof(entry) + entry.name_length;
		my_index_buffer[i].length = entry.length - entry.name_length - sizeof(entry);
		offset += entry.length;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_link_entry_t);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KaGuYa_link5_operation = {
	KaGuYa_link5_match,					/* match */
	KaGuYa_link5_extract_directory,		/* extract_directory */
	KaGuYa_link3_parse_resource_info,	/* parse_resource_info */
	KaGuYa_link3_extract_resource,		/* extract_resource */
	KaGuYa_link3_save_resource,
	KaGuYa_link3_release_resource,
	KaGuYa_link3_release
};

/********************* anm *********************/

/* 封包匹配回调函数 */
static int KaGuYa_anm_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AN00", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KaGuYa_anm_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 width, height;
	u16 frames;

	if (pkg->pio->readvec(pkg, &width, 4, 12, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->read(pkg, &height, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &frames, 2))
		return -CUI_EREAD;

	anm_entry_t *frame = new anm_entry_t[frames];
	if (!frame)
		return -CUI_EMEM;

	DWORD offset = 0x18 + frames * 4 + 0x12;
	DWORD length = width * height * 4;
	for (DWORD i = 0; i < frames; i++) {
		sprintf(frame[i].name, "%d.anm.bmp", i);
		frame[i].length = length;
		frame[i].offset = offset;
		frame[i].width = width;
		frame[i].height = height;
		offset += length;
	}

	pkg_dir->index_entries = frames;
	pkg_dir->directory = frame;
	pkg_dir->directory_length = sizeof(anm_entry_t) * frames;
	pkg_dir->index_entry_length = sizeof(anm_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int KaGuYa_anm_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	anm_entry_t *anm_entry;

	anm_entry = (anm_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, anm_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = anm_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = anm_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KaGuYa_anm_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	anm_entry_t *anm_entry = (anm_entry_t *)pkg_res->actual_index_entry;
	if (MyBuildBMPFile(raw, raw_len, NULL, 0, anm_entry->width, 
			anm_entry->height, 32, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] raw;

	return 0;
}

static cui_ext_operation KaGuYa_anm_operation = {
	KaGuYa_anm_match,					/* match */
	KaGuYa_anm_extract_directory,		/* extract_directory */
	KaGuYa_anm_parse_resource_info,		/* parse_resource_info */
	KaGuYa_anm_extract_resource,		/* extract_resource */
	KaGuYa_link3_save_resource,
	KaGuYa_link3_release_resource,
	KaGuYa_link3_release
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK KaGuYa_register_cui(struct cui_register_callback *callback)
{
//	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
//		NULL, &KaGuYa_tbl_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
//			return -1;

	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &KaGuYa_arc_lst_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &KaGuYa_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".alp"), _T(".bmp"), 
		NULL, &KaGuYa_bg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bg_"), _T(".bmp"), 
		NULL, &KaGuYa_bg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cg_"), _T(".bmp"), 
		NULL, &KaGuYa_bg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cgw"), _T(".bmp"), 
		NULL, &KaGuYa_bg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sp_"), _T(".bmp"), 
		NULL, &KaGuYa_bg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	
	if (callback->add_extension(callback->cui, _T(".bgm"), _T(".ogg"), 
		NULL, &KaGuYa_ogg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".vo*"), _T(".ogg"), 
		NULL, &KaGuYa_ogg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".aps"), _T(".bmp"), 
		NULL, &KaGuYa_aps_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".parts"), _T(".bmp"), 
		NULL, &KaGuYa_aps_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".anm"), _T(".bmp"), 
		NULL, &KaGuYa_anm_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &KaGuYa_link3_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &KaGuYa_link5_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
