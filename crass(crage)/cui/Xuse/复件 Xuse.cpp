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
#include <utility.h>

/*
Xuse: 
ETERNAL
輝光翼戦記 天空のユミナ
D:\揤嬻偺儐儈僫愴摤懱尡斉\Data
*/

struct acui_information Xuse_cui_information = {
	_T(""),	/* copyright */
	NULL,							/* system */
	_T(".arc .xarc .bin .4ag .wag bv* .xsa"),	/* package */
	_T("0.2.6"),					/* revision */
	_T("痴漢公賊"),
	_T("2009-1-22 22:50"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "XARC" or "MIKO"
	u32 version;		// 2
	u16 crc;
} arc_header_t;

typedef struct {
	u16 sync;			// must be 0x1001
	u32 mode;
	s32 index_entries;
	u16 crc;
} arc_dir_header_t;

typedef struct {
	s8 magic[4];		// "DFNM"
	s32 CADR_offset_lo;
	s32 CADR_offset_hi;
	u16 crc;
} arc_DFNM_t;

typedef struct {
	s8 magic[4];		// "DFNM"
	u16 name_len;
	u16 name_len2;
	u16 unknown;
	u16 crc;
} __arc_DFNM1_t;

typedef struct {
	__arc_DFNM1_t DFNM1;
	s8 *name;
	//s8 *name2;
	u16 crc2;
} arc_DFNM1_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u32 CTIF_offset;
	u16 crc;
} arc_NDIX_t;

// 紧接在NDIX数据段之后，"EDIX"后接和NDIX完全相同的EDIX数据
typedef struct {
	u16 sync;		// must be 0x1001
	u32 CTIF_offset;
	u16 crc;
} arc_EDIX_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u16 unknown;	// 0
	u16 index_number;
	u16 name_length;
	u16 crc;
	//s8 *name;
	//u16 crc;
} arc_CTIF_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u32 DATA_offset_lo;
	u32 DATA_offset_hi;
	u16 crc;
} arc_CADR_t;

typedef struct {
	s8 magic[4];		// "DATA"
	u32 unknown;		// 0x20 or 0x80 or 0x26(Thumbs.db)
	// 时间戳
	u16 year, month, day, hour, minute, second;
	u16 sec_60, ms;		// 1/60秒和ms?
	u32 length;
	u16 crc;
} arc_DATA_t;

typedef struct {
	u32 sync;
} bv_header_t;			// must be 1

typedef struct {
	u32 length;
	u32 offset;
	u32 reserved[2];
} bv_entry_t;

typedef struct {
	s8 magic[4];		// "GAF4" or "WAG@"
	u16 sync;			// muse be 0x300
	s8 desc[64];		// "generic","NarukanaTrial","Narukana","YuminaTrial" and so on...	
	u32 index_entries;
} _4ag_header_t;

typedef struct {
	u32 magic;
	u32 chunk_counts;
	u16 crc;
} _4ag_DSET_t;

typedef struct {
	u32 magic;
	u32 length;
	u16 crc;
} _4ag_sub_chunk_t;

typedef struct {
	u32 value;
	u16 crc;
} _4ag_PICT_parameter_t;

typedef struct {
	s8 magic[4];		// "VAR"
	u32 sync;			// 0x10000
	u32 unknown_lo;		// 0
	u32 unknown_hi;		// 0
	u32 unknown_length;
	u32 unknown[3];
} var_header_t;

typedef struct {
	s8 magic[4];		// "NORI"
	u32 sync;			// 0x10000
	u8 unknown[0x24];
	u16 crc;
} nori_header_t;

typedef struct {
	s8 magic[6];		// "KOTORI"
	u32 sync;			// 0x001a1a00
	u32 sync1;			// 0
	u16 crc;
} KOTORI_header_t;

typedef struct {
	u32 sync;			// 0x0100a618
	u16 index_entries;
	u16 crc;
} KOTORI_info_t;

typedef struct {
	u32 offset;
	u16 crc;
} KOTORI_entry_t;

typedef struct {
	s8 magic[6];		// "KOTORi"
	u32 sync;			// 0x001a1a00
	u32 sync1;			// 0
	u16 crc;
} KOTORi_header_t;

typedef struct {
	u32 sync;			// 0x0100a618
	u32 pcm_data_length;
	u32 unknown0;		// 0
	u32 unknown1;		// 0
	u8 key[16];
	u16 crc;
} KOTORi_info_t;
#pragma pack ()

typedef struct {
	s8 name[260];
	u32 offset;
	u32 length;
} my_arc_entry_t;

typedef struct {
	s8 *name;
	u32 offset;
	u32 length;
} my_4ag_entry_t;

typedef struct {
	void *dec_buf;
	unsigned int dec_buf_len;
} my_4ag_private_t;

/**
 * Default CRC function. Note that avrmote has a much more efficient one.
 *
 * This CRC-16 function produces a 16-bit running CRC that adheres to the
 * ITU-T CRC standard.
 *
 * The ITU-T polynomial is: G_16(x) = x^16 + x^12 + x^5 + 1
 *
 */
static u16 do_crc(BYTE *data, DWORD data_len, u16 crc)
{
	for (DWORD i = 0; i < data_len; ++i) {
		crc ^= data[i] << 8;
		for (DWORD k = 0; k < 8; ++k)
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
	}

	return crc;
}

static int Xuse_crc_match(BYTE *data, DWORD len)
{
	return !do_crc(data, len, 0);
}

static int init_4ag_decode_buffer(char *string, BYTE **ret_dec_buf, unsigned int *ret_dec_buf_len)
{
	int name_len;
	int v = 0;
	int dec_buf_len;
	BYTE *dec_buf;

	name_len = strlen(string);
	for (int i = 0; i < name_len; i++)
		v = ((string[i] + i) ^ v) + name_len;
	
	dec_buf_len = (v & 0xff) + 64;

	for (i = 0; i < name_len; i++)
		v += string[i];

	dec_buf = (BYTE *)malloc(dec_buf_len);
	if (!dec_buf)
		return -1;
	dec_buf[dec_buf_len - 1] = 0;

	dec_buf[0] = v & 0x0f;
	dec_buf[1] = (v >> 8) & 0xff;
	dec_buf[2] = 0x46;
	dec_buf[3] = 0x88;
	v &= 0x0f;
	if (dec_buf_len - 1 > 4) {
		for (i = 4; i < dec_buf_len - 1; i++) {
			v += ((string[i % name_len] ^ v) + i) & 0xff;
			dec_buf[i] = v;
		}
	}
	*ret_dec_buf = dec_buf;
	*ret_dec_buf_len = dec_buf_len;

	return 0;
}

static void decode_4ag_resource(u8 *buf, unsigned int len, u32 offset, u8 *dec_buf, unsigned int dec_buf_len)
{
	for (unsigned int i = 0; i < len; i++)
		buf[i] ^= dec_buf[(offset + i) % (dec_buf_len - 1)];
}

static void decode_cd(u8 *buf, unsigned int len, u32 offset, u8 *dec_buf, unsigned int dec_buf_len)
{
	for (unsigned int i = 0; i < len; i++)
		buf[i] ^= dec_buf[(offset + i) % (dec_buf_len - 1)];
}

static void decode_ScenarioVariable(u8 *buf, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		buf[i] = (((buf[i] & 1) << 7) | (buf[i] >> 1)) ^ 0x53;
}

/********************* arc/xarc *********************/

static int Xuse_arc_match(struct package *pkg)
{
	arc_header_t arc_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(arc_header.magic, "XARC", 4) && strncmp(arc_header.magic, "MIKO", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (arc_header.version != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (!Xuse_crc_match((BYTE *)&arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Xuse_arc_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{		
	arc_dir_header_t arc_dir;
	arc_DFNM_t DFNM_header;
	arc_DFNM1_t DFNM1_header;
	s8 NDIX_magic[4], CADR_magic[4];
	my_arc_entry_t *index_buffer;
	unsigned int index_length;
	s32 CADR_offset_lo, CADR_offset_hi;
	int c;

	if (pkg->pio->read(pkg, &arc_dir, sizeof(arc_dir)))
		return -CUI_EREAD;

	if ((arc_dir.sync != 0x1001) || (arc_dir.index_entries <= 0)
			|| !Xuse_crc_match((BYTE *)&arc_dir, sizeof(arc_dir)))
		return -CUI_EMATCH;	

	switch (arc_dir.mode & 0xf) {
	case 0:
		if (pkg->pio->read(pkg, &DFNM_header, sizeof(DFNM_header)))
			return -CUI_EREAD;

		if (strncmp(DFNM_header.magic, "DFNM", 4) 
				|| !Xuse_crc_match((BYTE *)&DFNM_header, sizeof(DFNM_header)))
			return -CUI_EMATCH;

		if (DFNM_header.CADR_offset_hi < 0)
			return -CUI_EMATCH;

		CADR_offset_lo = DFNM_header.CADR_offset_lo;
		CADR_offset_hi = DFNM_header.CADR_offset_hi;
		break;
	case 1:	// no test @ 415F20
		if (pkg->pio->read(pkg, &DFNM1_header, sizeof(__arc_DFNM1_t)))
			return -CUI_EREAD;

		if (memcmp(DFNM1_header.DFNM1.magic, "DFNM", 4) || DFNM1_header.DFNM1.name_len > 260
				|| DFNM1_header.DFNM1.name_len2 > 260 || DFNM1_header.DFNM1.unknown > 10
				|| !Xuse_crc_match((BYTE *)&DFNM1_header, sizeof(__arc_DFNM1_t)))
			return -CUI_EMATCH;

		DFNM1_header.name = (char *)malloc(DFNM1_header.DFNM1.name_len 
			+ DFNM1_header.DFNM1.name_len2 + 2);
		if (!DFNM1_header.name)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg, &DFNM1_header.name, DFNM1_header.DFNM1.name_len 
				+ DFNM1_header.DFNM1.name_len2 + 2)) {
			free(DFNM1_header.name);
			return -CUI_EREAD;
		}

		if (!Xuse_crc_match((BYTE *)DFNM1_header.name, 
				DFNM1_header.DFNM1.name_len + DFNM1_header.DFNM1.name_len2 + 2)) {
			free(DFNM1_header.name);
			return -CUI_EMATCH;
		}

		for (c = 0; c < DFNM1_header.DFNM1.name_len + DFNM1_header.DFNM1.name_len2; ++c)
			DFNM1_header.name[c] ^= 0x56;

		//free(name);
		//CADR_offset_lo = DFNM1_header.CADR_offset_lo;
		//CADR_offset_hi = DFNM1_header.CADR_offset_hi;
		printf("unsupport dir mode 1\n");
		return -CUI_EMATCH;
		break;
	case 2:	// no test, 而且似乎系统也不支持这种模式了
		CADR_offset_lo = 22;
		CADR_offset_hi = 0;
		break;
	default:
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, NDIX_magic, 4))
		return -CUI_EREAD;

	if (strncmp(NDIX_magic, "NDIX", 4))
		return -CUI_EMATCH;

	u32 pos;
	if (pkg->pio->locate(pkg, &pos))
		return -CUI_EREAD;

	// 这里实际上应该在下面的循环中做CADR检查
	if (pkg->pio->seek(pkg, CADR_offset_lo, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, CADR_magic, 4))
		return -CUI_EREAD;

	if (strncmp(CADR_magic, "CADR", 4))
		return -CUI_EMATCH;

	index_length = arc_dir.index_entries * sizeof(my_arc_entry_t);
	index_buffer = (my_arc_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	int err = 0;
	for (int i = 0; i < arc_dir.index_entries; ++i) {
		arc_NDIX_t NDIX;
		arc_CTIF_t CTIF;
		arc_CADR_t CADR;

		if (pkg->pio->readvec(pkg, &NDIX, sizeof(NDIX), pos + i * sizeof(NDIX), IO_SEEK_SET)) {
			err = -CUI_EREADVEC;	
			break;
		}

		if ((NDIX.sync != 0x1001) || !Xuse_crc_match((BYTE *)&NDIX, sizeof(NDIX))) {
			err = -CUI_EMATCH;	
			break;
		}

		if (pkg->pio->seek(pkg, NDIX.CTIF_offset, IO_SEEK_SET)) {
			err = -CUI_ESEEK;	
			break;
		}

		if (pkg->pio->read(pkg, &CTIF, sizeof(CTIF))) {
			err = -CUI_EREAD;	
			break;
		}

		if (CTIF.sync != 0x1001 || CTIF.name_length > 260 
				|| !Xuse_crc_match((BYTE *)&CTIF, sizeof(CTIF))) {
			err = -CUI_EMATCH;	
			break;
		}
		
		if (pkg->pio->read(pkg, index_buffer[i].name, CTIF.name_length + 2)) {
			err = -CUI_EREAD;	
			break;
		}

		if (!Xuse_crc_match((BYTE *)index_buffer[i].name, CTIF.name_length + 2)) {
			err = -CUI_EMATCH;	
			break;
		}

		for (unsigned int c = 0; c < CTIF.name_length; c++)
			index_buffer[i].name[c] ^= 0x56;
		index_buffer[i].name[CTIF.name_length] = 0;

		if (pkg->pio->seek(pkg, CADR_offset_lo + CTIF.index_number * sizeof(arc_CADR_t) + 4, IO_SEEK_SET)) {
			err = -CUI_ESEEK;	
			break;
		}

		if (pkg->pio->read(pkg, &CADR, sizeof(CADR))) {
			err = -CUI_EREAD;	
			break;
		}

		if (CADR.sync != 0x1001 || !Xuse_crc_match((BYTE *)&CADR, sizeof(CADR))) {
			err = -CUI_EMATCH;	
			break;
		}

		index_buffer[i].offset = CADR.DATA_offset_lo;
#if 0
		arc_DATA_t DATA;

		if (pkg->pio->seek(pkg, CADR.DATA_offset_lo, IO_SEEK_SET)) {
			err = -CUI_ESEEK;	
			break;
		}

		if (pkg->pio->read(pkg, &DATA, sizeof(DATA))) {
			err = -CUI_EREAD;	
			break;
		}

		if (strncmp(DATA.magic, "DATA", 4)) {
			err = -CUI_EMATCH;	
			break;
		}

		if (!Xuse_crc_match((BYTE *)&DATA, sizeof(DATA))) {
			err = -CUI_EMATCH;	
			break;
		}
#if 0
		if (DATA.sec_60 >= 60 && DATA.ms >= 1000) {
			printf("%s: data offset %x, unknown %x,"
				"length %x, time %d-%d-%d %d:%d:%d %d/%d\n", index_buffer[i].name, index_buffer[i].offset,
				DATA.unknown, DATA.length, 
				DATA.year, DATA.month, DATA.day, DATA.hour, 
				DATA.minute, DATA.second, DATA.sec_60, DATA.ms
				);
		}
#endif
		index_buffer[i].offset += sizeof(DATA);
		index_buffer[i].length = DATA.length;
#endif
	}
	pkg_dir->index_entries = arc_dir.index_entries;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int Xuse_arc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;
	pkg_res->actual_data_length = 0;

	if (pkg->pio->seek(pkg, my_arc_entry->offset, IO_SEEK_SET))
		return -CUI_ESEEK;	

	arc_DATA_t DATA;
	if (pkg->pio->read(pkg, &DATA, sizeof(DATA)))
		return -CUI_EREAD;	

	if (strncmp(DATA.magic, "DATA", 4) || !Xuse_crc_match((BYTE *)&DATA, sizeof(DATA)))
		return -CUI_EMATCH;	

#if 0
	if (DATA.sec_60 >= 60 && DATA.ms >= 1000) {
		printf("%s: data offset %x, unknown %x,"
			"length %x, time %d-%d-%d %d:%d:%d %d/%d\n", index_buffer[i].name, index_buffer[i].offset,
			DATA.unknown, DATA.length, 
			DATA.year, DATA.month, DATA.day, DATA.hour, 
			DATA.minute, DATA.second, DATA.sec_60, DATA.ms
			);
	}
#endif

	pkg_res->offset = my_arc_entry->offset + sizeof(DATA);
	pkg_res->raw_data_length = DATA.length;

	return 0;
}

static int Xuse_arc_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length + 2);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length + 2,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	if (!Xuse_crc_match((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length + 2)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMATCH;
	}

	return 0;
}

static int Xuse_arc_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void Xuse_arc_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Xuse_arc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation Xuse_arc_operation = {
	Xuse_arc_match,					/* match */
	Xuse_arc_extract_directory,		/* extract_directory */
	Xuse_arc_parse_resource_info,	/* parse_resource_info */
	Xuse_arc_extract_resource,		/* extract_resource */
	Xuse_arc_save_resource,			/* save_resource */
	Xuse_arc_release_resource,		/* release_resource */
	Xuse_arc_release				/* release */
};

/********************* bv *********************/

static int Xuse_bv_match(struct package *pkg)
{
	bv_header_t bv_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &bv_header, sizeof(bv_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bv_header.sync != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if ((pkg->extension && _tcscmp(pkg->extension, _T(".bin"))) 
			&& (pkg->name[0] != _T('b') || pkg->name[1] != _T('v'))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Xuse_bv_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{		
	bv_entry_t *index_buffer;
	unsigned int index_entries, index_length;
	bv_entry_t bv_entry, *prev;

	if (pkg->pio->read(pkg, &bv_entry, sizeof(bv_entry))) 
		return -CUI_EREAD;

	index_length = bv_entry.offset - 4;
	index_entries = index_length / sizeof(bv_entry);
	if (!index_entries)
		return -CUI_EMATCH;

	index_buffer = (bv_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	memcpy(index_buffer, &bv_entry, sizeof(bv_entry));

	prev = &bv_entry;
	for (unsigned int i = 1; i < index_entries; i++) {
		bv_entry_t *cur = &index_buffer[i];

		if (pkg->pio->read(pkg, cur, sizeof(*cur))) 
			return -CUI_EREAD;

		if (cur->offset != prev->offset + prev->length)
			break;
		prev = cur;
	}

	pkg_dir->index_entries = i;
	pkg_dir->index_entry_length = sizeof(bv_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = i * sizeof(bv_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int Xuse_bv_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	bv_entry_t *bv_entry;

	bv_entry = (bv_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%05d.ogg", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = bv_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = bv_entry->offset;

	return 0;
}

static int Xuse_bv_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
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

static int Xuse_bv_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void Xuse_bv_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Xuse_bv_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation Xuse_bv_operation = {
	Xuse_bv_match,					/* match */
	Xuse_bv_extract_directory,		/* extract_directory */
	Xuse_bv_parse_resource_info,	/* parse_resource_info */
	Xuse_bv_extract_resource,		/* extract_resource */
	Xuse_bv_save_resource,			/* save_resource */
	Xuse_bv_release_resource,		/* release_resource */
	Xuse_bv_release					/* release */
};

/********************* xsa *********************/

static int Xuse_xsa_match(struct package *pkg)
{
	bv_header_t bv_header;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &bv_header, sizeof(bv_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bv_header.sync != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static cui_ext_operation Xuse_xsa_operation = {
	Xuse_xsa_match,					/* match */
	Xuse_bv_extract_directory,		/* extract_directory */
	Xuse_bv_parse_resource_info,	/* parse_resource_info */
	Xuse_bv_extract_resource,		/* extract_resource */
	Xuse_bv_save_resource,			/* save_resource */
	Xuse_bv_release_resource,		/* release_resource */
	Xuse_bv_release					/* release */
};

/********************* 4ag *********************/

static int Xuse_4ag_match(struct package *pkg)
{
	_4ag_header_t *_4ag_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	_4ag_header = (_4ag_header_t *)malloc(sizeof(*_4ag_header));
	if (!_4ag_header) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, _4ag_header, sizeof(*_4ag_header))) {
		free(_4ag_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(_4ag_header->magic, "GAF4", 4) && strncmp(_4ag_header->magic, "WAG@", 4)) {
		free(_4ag_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (_4ag_header->sync != 0x0300) {
		free(_4ag_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, _4ag_header);

	return 0;
}

static int Xuse_4ag_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{		
	_4ag_header_t *_4ag_header;
	char mbcs_4ag_name[260];
	char *_4ag_name_lower;
	BYTE *dec_buf;
	unsigned int dec_buf_len;
	unsigned int len0, len1;
	unsigned int i;

	_4ag_header = (_4ag_header_t *)package_get_private(pkg);
	if (pkg->pio->seek(pkg, 0, SEEK_SET))
		return -CUI_ESEEK;

	if (unicode2sj(mbcs_4ag_name, sizeof(mbcs_4ag_name), pkg->name, -1))
		return -CUI_EMEM;

	_4ag_name_lower = CharLowerA((char *)mbcs_4ag_name);
	if (init_4ag_decode_buffer(_4ag_name_lower, &dec_buf, &dec_buf_len))
		return -CUI_EMEM;
	
	len1 = len0 = 512;
	for (i = 0; i < dec_buf_len; i++) {
		len0 += dec_buf[i];
		len1 -= dec_buf[i];
	}

	for (i = 0; i < dec_buf_len; i++) {
		int shift;

		len0 ^= dec_buf[i];
		len0 = (len0 >> 1) | (len0 << 31);
		shift = dec_buf[i] % 32;
		len1 = (len1 << (32 - shift)) | (len1 >> shift);
	}

	for (i = 0; i < dec_buf_len; i++) {
		len0 ^= dec_buf[i];
		len1 ^= ~dec_buf[i];
		len0 = (len0 >> 1) | (len0 << 31);
		len1 = (len1 >> 1) | (len1 << 31);
	}
	len0 = len0 % 0x401;
	len1 = len1 % 0x401;

	int check_data_len = sizeof(_4ag_header_t) + len0 + _4ag_header->index_entries * 4 + len1 + 2;
	BYTE *check_data = (BYTE *)malloc(check_data_len);
	if (!check_data) {
		free(dec_buf);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, check_data, check_data_len)) {
		free(dec_buf);
		return -CUI_EREAD;
	}

	if (!Xuse_crc_match(check_data, check_data_len)) {
		free(check_data);
		free(dec_buf);
		return -CUI_EMATCH;
	}
	free(check_data);

	unsigned int index_offset_dec_buf_len = _4ag_header->index_entries * 4;
	BYTE *index_offset_dec_buf = (BYTE *)malloc(index_offset_dec_buf_len);
	if (!index_offset_dec_buf) {
		free(dec_buf);
		return -CUI_EMEM;
	}

	for (i = 0; i < index_offset_dec_buf_len; i++)	// 424e60 called from 425c65
		index_offset_dec_buf[i] = _4ag_header->index_entries + ((u8)dec_buf[(i + 1) % dec_buf_len] ^ ((u8)dec_buf[i % dec_buf_len] + (i & 0xff)));
	free(dec_buf);
	//ret from 427f2a
#if 0
	if (strcmp(_4ag_header->desc, "generic") 
			&& strcmp(_4ag_header->desc, "NarukanaTrial") 
			&& strcmp(_4ag_header->desc, "Narukana")) {
		free(index_offset_dec_buf);
		return -CUI_EMATCH;
	}
#endif
	_4ag_name_lower = (char *)_4ag_header->desc;
	if (init_4ag_decode_buffer(_4ag_name_lower, &dec_buf, &dec_buf_len)) {
		free(index_offset_dec_buf);
		return -CUI_EMEM;
	}

	my_4ag_entry_t *my_4ag_entry = (my_4ag_entry_t *)malloc(sizeof(my_4ag_entry_t) * _4ag_header->index_entries);
	if (!my_4ag_entry) {
		free(dec_buf);
		free(index_offset_dec_buf);
		return -CUI_EMEM;
	}

	int ret = 0;
	for (i = 0; i < _4ag_header->index_entries; i++) {
		u32 offset = sizeof(_4ag_header_t) + len0 + i * 4;
		u32 res_offset;

		if (pkg->pio->seek(pkg, offset, IO_SEEK_SET)) {
			ret = -CUI_ESEEK;
			break;
		}

		if (pkg->pio->read(pkg, &res_offset, 4)) {
			ret = -CUI_EREAD;
			break;
		}

		// 424d30 called from 426cfa
		decode_4ag_resource((u8 *)&res_offset, 4, offset, index_offset_dec_buf, index_offset_dec_buf_len);

		if (pkg->pio->seek(pkg, res_offset, IO_SEEK_SET)) {
			ret = -CUI_ESEEK;
			break;
		}

		_4ag_DSET_t dset;
		if (pkg->pio->read(pkg, &dset, sizeof(dset))) {
			ret = -CUI_EREAD;
			break;
		}

		if (!Xuse_crc_match((unsigned char *)&dset, sizeof(dset))) {
			ret = -CUI_EMATCH;
			break;
		}

		decode_4ag_resource((u8 *)&dset, 8, res_offset, dec_buf, dec_buf_len);
		res_offset += sizeof(dset);

		for (unsigned int j = 0; j < dset.chunk_counts; j++) {
			_4ag_sub_chunk_t sub_chunk;

			//res_offset = get_current_offset() - 4ag_offset
			if (pkg->pio->read(pkg, &sub_chunk, sizeof(sub_chunk)))	{//67f8ac
				ret = -CUI_EREAD;
				break;
			}

			if (!Xuse_crc_match((unsigned char *)&sub_chunk, sizeof(sub_chunk))) {
				ret = -CUI_EMATCH;
				break;
			}

			decode_4ag_resource((u8 *)&sub_chunk, 8, res_offset, dec_buf, dec_buf_len);

			res_offset += sizeof(sub_chunk);
			// 以上，根据mode返回当前的offset called from 426d78

			int ftags = 0;
			char *buf;
			switch (sub_chunk.magic) {
			case 0x54434950:	//PICT
				_4ag_PICT_parameter_t pict_parameter;

				//if (sub_chunk.length < 4)
				//	exit(0);

				if (pkg->pio->read(pkg, &pict_parameter, sizeof(pict_parameter))) {	//ebf0f0ee
					ret = -CUI_EREAD;
					break;
				}

				if (!Xuse_crc_match((unsigned char *)&pict_parameter, sizeof(pict_parameter))) {
					ret = -CUI_EMATCH;
					break;
				}

				decode_4ag_resource((u8 *)&pict_parameter.value, 4, res_offset, dec_buf, dec_buf_len);

				if (pict_parameter.value & 0xffff) {
					ret = -CUI_EMATCH;
					break;
				}

				my_4ag_entry[i].offset = res_offset + sizeof(pict_parameter);
				my_4ag_entry[i].length = sub_chunk.length - sizeof(pict_parameter);

				if (pkg->pio->seek(pkg, sub_chunk.length - sizeof(pict_parameter), IO_SEEK_CUR)) {
					ret = -CUI_EMATCH;
					break;
				}
				break;
			case 0x47415446:	//FTAG(资源数据路径，例如：WaitScreen\Load.png)
				if (ftags >= 3) {
					ret = -CUI_EMATCH;
					break;
				}

				ftags++;

				buf = (char *)malloc(sub_chunk.length);
				if (pkg->pio->read(pkg, buf, sub_chunk.length)) {
					ret = -CUI_EREAD;
					break;
				}

				if (!Xuse_crc_match((unsigned char *)buf, sub_chunk.length)) {
					free(buf);
					ret = -CUI_EMATCH;
					break;
				}

				decode_4ag_resource((u8 *)buf, sub_chunk.length - 2, res_offset, dec_buf, dec_buf_len);

				buf[sub_chunk.length - 2] = 0;
				my_4ag_entry[i].name = buf;
				break;
			case 0x4C415845:	//EXAL:
				fprintf(stderr, "unsupported EXAL\n");
				if (pkg->pio->seek(pkg, sub_chunk.length, IO_SEEK_CUR)) {
					ret = -CUI_ESEEK;
					break;
				}
				break;
			case 0x544E4D43:	//CMNT:
				fprintf(stderr, "unsupported CMNT\n");
				if (pkg->pio->seek(pkg, sub_chunk.length, IO_SEEK_CUR)) {
					ret = -CUI_ESEEK;
					break;
				}
				break;
			case 0x5F5A4F4D:	//MOZ_:
				if (pkg->pio->seek(pkg, sub_chunk.length, IO_SEEK_CUR)) {
					ret = -CUI_ESEEK;
					break;
				}
#if 0
				buf = (char *)malloc(sub_chunk.length);
				if (pkg->pio->read(pkg, buf, sub_chunk.length))	{//ebf0f0ee
					ret = -CUI_EREAD;
					break;
				}

				decode_4ag_resource((u8 *)buf, sub_chunk.length, res_offset, dec_buf, dec_buf_len);
				_stprintf(name, _T("%04d.MOZ"), i);
				MySaveFile(name, buf, sub_chunk.length);
				free(buf);
#endif
				break;
			default:
				fprintf(stderr, "unknown sub chunk\n");
				ret = -CUI_EMATCH;
				break;
			}	// sub chunck magic
			if (ret)
				break;

			res_offset += sub_chunk.length;
		}	// chunck_counts
		if (j != dset.chunk_counts)
			break;
	}
	free((void *)package_get_private(pkg));
	free(index_offset_dec_buf);
	pkg_dir->index_entries = _4ag_header->index_entries;
	pkg_dir->index_entry_length = sizeof(my_4ag_entry_t);
	pkg_dir->directory = my_4ag_entry;
	pkg_dir->directory_length = sizeof(my_4ag_entry_t) * _4ag_header->index_entries;
	my_4ag_private_t *_4ag_private = (my_4ag_private_t *)malloc(sizeof(my_4ag_private_t));
	if (!_4ag_private)
		ret = -CUI_EMEM;
	else {
		_4ag_private->dec_buf = dec_buf;
		_4ag_private->dec_buf_len = dec_buf_len;
		package_set_private(pkg, _4ag_private);
	}
	return ret;	
}

static int Xuse_4ag_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_4ag_entry_t *my_4ag_entry;

	my_4ag_entry = (my_4ag_entry_t *)pkg_res->actual_index_entry;
	/* 过滤掉
	 * Nas\02_揤嬻偺儐儈僫\040_夋憸\045_僼僃僀僗\0455_亂愴摤僼僃僀僗亃曇廤\03_png\bf312_0101.png
	 * @Data\fa0000.wag中错误的资源名	
	 */
	char *name = my_4ag_entry->name;
	while (*name == '\\')
		++name;
	strcpy(pkg_res->name, name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_4ag_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_4ag_entry->offset;

	return 0;
}

static int Xuse_4ag_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	my_4ag_private_t *_4ag_private;

	_4ag_private = (my_4ag_private_t *)package_get_private(pkg);

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	decode_4ag_resource((u8 *)pkg_res->raw_data, pkg_res->raw_data_length, 
		pkg_res->offset, (u8 *)_4ag_private->dec_buf, _4ag_private->dec_buf_len);

	return 0;
}

static int Xuse_4ag_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void Xuse_4ag_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Xuse_4ag_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	my_4ag_private_t *_4ag_private;

	if (pkg_dir->directory) {
		for (unsigned int i = 0; i < pkg_dir->index_entries; i++) {
			my_4ag_entry_t *my_4ag_entry = &(((my_4ag_entry_t *)pkg_dir->directory)[i]);
			free(my_4ag_entry->name);
		}
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	_4ag_private = (my_4ag_private_t *)package_get_private(pkg);
	free(_4ag_private->dec_buf);
	free(_4ag_private);
	pkg->pio->close(pkg);
}

static cui_ext_operation Xuse_4ag_operation = {
	Xuse_4ag_match,					/* match */
	Xuse_4ag_extract_directory,		/* extract_directory */
	Xuse_4ag_parse_resource_info,	/* parse_resource_info */
	Xuse_4ag_extract_resource,		/* extract_resource */
	Xuse_4ag_save_resource,			/* save_resource */
	Xuse_4ag_release_resource,		/* release_resource */
	Xuse_4ag_release				/* release */
};

/********************* var *********************/

static int Xuse_var_match(struct package *pkg)
{
	var_header_t var_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &var_header, sizeof(var_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(var_header.magic, "VAR", 4) || var_header.sync != 0x10000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Xuse_var_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		0, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}
	
	// 不包括crc
	decode_ScenarioVariable((BYTE *)pkg_res->raw_data + sizeof(var_header_t), 
		pkg_res->raw_data_length - sizeof(var_header_t) - 2);

	if (!Xuse_crc_match((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EMATCH;
	}

	return 0;
}

static cui_ext_operation Xuse_var_operation = {
	Xuse_var_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	Xuse_var_extract_resource,	/* extract_resource */
	Xuse_arc_save_resource,		/* save_resource */
	Xuse_arc_release_resource,	/* release_resource */
	Xuse_arc_release			/* release */
};

/********************* kotori *********************/

static int Xuse_KOTORI_match(struct package *pkg)
{
	KOTORI_header_t KOTORI_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &KOTORI_header, sizeof(KOTORI_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(KOTORI_header.magic, "KOTORI", sizeof(KOTORI_header.magic))
			|| KOTORI_header.sync != 0x001a1a00 || KOTORI_header.sync1
			|| !Xuse_crc_match((BYTE *)&KOTORI_header, sizeof(KOTORI_header))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Xuse_KOTORI_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	KOTORI_info_t KOTORI_info;

	if (pkg->pio->read(pkg, &KOTORI_info, sizeof(KOTORI_info))) 
		return -CUI_EREAD;

	if (KOTORI_info.sync != 0x0100a618 || !KOTORI_info.index_entries
			|| !Xuse_crc_match((BYTE *)&KOTORI_info, sizeof(KOTORI_info)))
		return -CUI_EMATCH;

	DWORD index_len = sizeof(KOTORI_entry_t) * KOTORI_info.index_entries;
	KOTORI_entry_t *index = (KOTORI_entry_t *)malloc(index_len); 
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = KOTORI_info.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

static int Xuse_KOTORI_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	KOTORI_entry_t *KOTORI_entry;

	KOTORI_entry = (KOTORI_entry_t *)pkg_res->actual_index_entry;
	if (!Xuse_crc_match((BYTE *)KOTORI_entry, sizeof(KOTORI_entry_t)))
		return -CUI_EMATCH;

	sprintf(pkg_res->name, "%05d", pkg_res->index_number);
		pkg_res->name_length = -1;
	if (pkg_res->flags & PKG_RES_FLAG_LAST) {
		u32 fsize;
		pkg->pio->length_of(pkg, &fsize);
		pkg_res->raw_data_length = fsize - KOTORI_entry->offset;
	} else
		pkg_res->raw_data_length = (KOTORI_entry+1)->offset - KOTORI_entry->offset;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = KOTORI_entry->offset;
	pkg_res->actual_index_entry_length = sizeof(KOTORI_entry_t);

	return 0;
}

static int Xuse_KOTORI_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
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

static cui_ext_operation Xuse_KOTORI_operation = {
	Xuse_KOTORI_match,				/* match */
	Xuse_KOTORI_extract_directory,	/* extract_directory */
	Xuse_KOTORI_parse_resource_info,/* parse_resource_info */
	Xuse_KOTORI_extract_resource,	/* extract_resource */
	Xuse_bv_save_resource,			/* save_resource */
	Xuse_bv_release_resource,		/* release_resource */
	Xuse_bv_release					/* release */
};

/********************* KOTORi *********************/

static int Xuse_KOTORi_match(struct package *pkg)
{
	KOTORi_header_t KOTORi_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &KOTORi_header, sizeof(KOTORi_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(KOTORi_header.magic, "KOTORi", sizeof(KOTORi_header.magic))
			|| KOTORi_header.sync != 0x001a1a00 || KOTORi_header.sync1
			|| !Xuse_crc_match((BYTE *)&KOTORi_header, sizeof(KOTORi_header))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Xuse_KOTORi_extract_resource(struct package *pkg,	
										struct package_resource *pkg_res)
{
	KOTORi_info_t KOTORi_info;

	if (pkg->pio->read(pkg, &KOTORi_info, sizeof(KOTORi_info))) 
		return -CUI_EREAD;

	if (KOTORi_info.sync != 0x0100a618 || !Xuse_crc_match((BYTE *)&KOTORi_info, sizeof(KOTORi_info)))
		return -CUI_EMATCH;

	DWORD data_len = pkg_res->raw_data_length - sizeof(KOTORi_header_t) - sizeof(KOTORi_info);
	BYTE *data = (BYTE *)malloc(data_len);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, data, data_len)) {
		free(data);
		return -CUI_EREAD;
	}

	// 实际key的偏移值是相对sizeof(KOTORi_header_t) + sizeof(KOTORi_info)开始的
	for (DWORD i = 0; i < data_len; ++i)
		data[i] ^= KOTORi_info.key[i % 16];

	pkg_res->raw_data = data;
	pkg_res->raw_data_length = data_len;

	return 0;
}

static cui_ext_operation Xuse_KOTORi_operation = {
	Xuse_KOTORi_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Xuse_KOTORi_extract_resource,	/* extract_resource */
	Xuse_arc_save_resource,			/* save_resource */
	Xuse_arc_release_resource,		/* release_resource */
	Xuse_arc_release				/* release */
};

int CALLBACK Xuse_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &Xuse_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".xarc"), NULL, 
		NULL, &Xuse_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Xuse_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".4ag"), NULL, 
		NULL, &Xuse_4ag_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wag"), NULL, 
		NULL, &Xuse_4ag_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &Xuse_bv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Xuse_bv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".xsa"), NULL, 
		NULL, &Xuse_xsa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".var"), _T("._var"), 
		NULL, &Xuse_var_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, NULL, _T(".bin"), 
		NULL, &Xuse_KOTORI_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T(".bin"), 
		NULL, &Xuse_KOTORI_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T(".ogg"), 
		NULL, &Xuse_KOTORi_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
