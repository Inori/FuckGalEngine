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

struct acui_information Narukana_cui_information = {
	_T("ザウス【本醸造】(Xuse)"),	/* copyright */
	NULL,							/* system */
	_T(".arc .xarc .bin .4ag bv*"),	/* package */
	_T("1.0.0"),					/* revision */
	_T(""),
	_T("2007-8-15 21:08"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "XARC"
	u32 version;	// 2
	u16 crc;
} arc_header_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u32 mode;
	s32 index_entries;
	u16 crc;
} arc_dir_header_t;

typedef struct {
	s8 magic[4];		// "DFNM"
	u32 CADR_offset_lo;
	u32 CADR_offset_hi;
	u16 crc;
} arc_DFNM_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u32 CTIF_offset;
	u16 crc;
} arc_NDIX_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u32 CTIF_offset;
	u16 crc;
} arc_EDIX_t;

typedef struct {
	u16 sync;		// must be 0x1001
	u16 zero;
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
	s8 magic[4];	// "DATA"
	u32 entry_length;
	u16 para[8];
	u32 length;
	u16 crc;
} arc_DATA_t;

typedef struct {
	u32 sync;
} bv_header_t;		// must be 1

typedef struct {
	u32 length;
	u32 offset;
	u32 reserved[2];
} bv_entry_t;

typedef struct {
	s8 magic[4];	// "GAF4"
	u16 sync;		// muse be 0x300
	s8 desc[64];	// "generic", "NarukanaTrial" or "Narukana"		
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

static int Narukana_crc_match(BYTE *buf, unsigned int len)
{
	unsigned int _crc = 0;
	u16 orig_crc = 0;

	for (unsigned int i = 0; i < len; i++) {
		_crc = (buf[i] << 8) ^ orig_crc;
		for (unsigned int k = 0; k < 8; k++) {
			s16 tmp = _crc;

			_crc *= 2;
			if (tmp < 0)
				_crc ^= 0x1021;
		}
		orig_crc = _crc;
	}
	return !_crc;
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

/********************* arc/xarc *********************/

static int Narukana_arc_match(struct package *pkg)
{
	arc_header_t arc_header;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(arc_header.magic, "XARC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (arc_header.version != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (!Narukana_crc_match((BYTE *)&arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Narukana_arc_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{		
	arc_dir_header_t arc_dir;
	arc_DFNM_t DFNM_header;
	s8 NDIX_magic[4], CADR_magic[4];
	my_arc_entry_t *index_buffer;
	unsigned int index_length;
	unsigned long CADR_offset_lo, CADR_offset_hi;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &arc_dir, sizeof(arc_dir)))
		return -CUI_EREAD;

	if ((arc_dir.sync != 0x1001) || (arc_dir.index_entries <= 0)
			|| !Narukana_crc_match((BYTE *)&arc_dir, sizeof(arc_dir)))
		return -CUI_EMATCH;	

	switch (arc_dir.mode) {
	case 0:
		if (pkg->pio->read(pkg, &DFNM_header, sizeof(DFNM_header)))
			return -CUI_EREAD;

		if (memcmp(DFNM_header.magic, "DFNM", 4) || !Narukana_crc_match((BYTE *)&DFNM_header, sizeof(DFNM_header)))
			return -CUI_EMATCH;

		CADR_offset_lo = DFNM_header.CADR_offset_lo;
		CADR_offset_hi = DFNM_header.CADR_offset_hi;
		break;
	case 1:
		//418e40
		printf("unsupported mode 1\n");
		break;
	case 2:
		CADR_offset_lo = 24;
		CADR_offset_hi = 0;
		break;
	default:
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, NDIX_magic, 4))
		return -CUI_EREAD;

	if (memcmp(NDIX_magic, "NDIX", 4))
		return -CUI_EMATCH;

	unsigned long pos;
	if (pkg->pio->locate(pkg, &pos))
		return -CUI_EREAD;

	if (pkg->pio->seek(pkg, CADR_offset_lo, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, CADR_magic, 4))
		return -CUI_EREAD;

	if (memcmp(CADR_magic, "CADR", 4))
		return -CUI_EMATCH;

	index_length = arc_dir.index_entries * sizeof(my_arc_entry_t);
	index_buffer = (my_arc_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	int err = 0;
	for (int i = 0; i < arc_dir.index_entries; i++) {
		arc_NDIX_t NDIX;
		arc_CTIF_t CTIF;
		arc_CADR_t CADR;

		if (pkg->pio->readvec(pkg, &NDIX, sizeof(NDIX), pos + i * sizeof(NDIX), IO_SEEK_SET)) {
			err = -CUI_EREADVEC;	
			break;
		}

		if ((NDIX.sync != 0x1001) || !Narukana_crc_match((BYTE *)&NDIX, sizeof(NDIX))) {
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

		if (CTIF.sync != 0x1001 || CTIF.zero || CTIF.index_number != i) {
			err = -CUI_EMATCH;	
			break;
		}

		if (pkg->pio->read(pkg, index_buffer[i].name, CTIF.name_length + 2)) {
			err = -CUI_EREAD;	
			break;
		}

		if (!Narukana_crc_match((BYTE *)index_buffer[i].name, CTIF.name_length + 2)) {
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

		if (CADR.sync != 0x1001 || !Narukana_crc_match((BYTE *)&CADR, sizeof(CADR))) {
			err = -CUI_EMATCH;	
			break;
		}

		index_buffer[i].offset = CADR.DATA_offset_lo;


		arc_DATA_t DATA;

		if (pkg->pio->seek(pkg, CADR.DATA_offset_lo, IO_SEEK_SET)) {
			err = -CUI_ESEEK;	
			break;
		}

		if (pkg->pio->read(pkg, &DATA, sizeof(DATA))) {
			err = -CUI_EREAD;	
			break;
		}

		if (memcmp(DATA.magic, "DATA", 4)) {
			err = -CUI_EMATCH;	
			break;
		}

		if (!Narukana_crc_match((BYTE *)&DATA, sizeof(DATA))) {
			err = -CUI_EMATCH;	
			break;
		}

		index_buffer[i].offset += sizeof(DATA);
		index_buffer[i].length = DATA.length;
	}
	pkg_dir->index_entries = arc_dir.index_entries;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int Narukana_arc_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_arc_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_arc_entry->offset;

	return 0;
}

static int Narukana_arc_extract_resource(struct package *pkg,
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

static int Narukana_arc_save_resource(struct resource *res, 
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

static void Narukana_arc_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Narukana_arc_release(struct package *pkg, 
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

static cui_ext_operation Narukana_arc_operation = {
	Narukana_arc_match,					/* match */
	Narukana_arc_extract_directory,		/* extract_directory */
	Narukana_arc_parse_resource_info,	/* parse_resource_info */
	Narukana_arc_extract_resource,		/* extract_resource */
	Narukana_arc_save_resource,			/* save_resource */
	Narukana_arc_release_resource,		/* release_resource */
	Narukana_arc_release				/* release */
};

/********************* bv *********************/

static int Narukana_bv_match(struct package *pkg)
{
	bv_header_t bv_header;
	
	if (!pkg)
		return -CUI_EPARA;

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

static int Narukana_bv_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{		
	bv_entry_t *index_buffer;
	unsigned int index_entries, index_length;
	bv_entry_t bv_entry, *prev;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

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

static int Narukana_bv_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	bv_entry_t *bv_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	bv_entry = (bv_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%05d.ogg", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = bv_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = bv_entry->offset;

	return 0;
}

static int Narukana_bv_extract_resource(struct package *pkg,
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

static int Narukana_bv_save_resource(struct resource *res, 
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

static void Narukana_bv_release_resource(struct package *pkg, 
										 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Narukana_bv_release(struct package *pkg, 
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

static cui_ext_operation Narukana_bv_operation = {
	Narukana_bv_match,					/* match */
	Narukana_bv_extract_directory,		/* extract_directory */
	Narukana_bv_parse_resource_info,	/* parse_resource_info */
	Narukana_bv_extract_resource,		/* extract_resource */
	Narukana_bv_save_resource,			/* save_resource */
	Narukana_bv_release_resource,		/* release_resource */
	Narukana_bv_release					/* release */
};


/********************* xsa *********************/

static int Narukana_xsa_match(struct package *pkg)
{
	bv_header_t bv_header;
	
	if (!pkg)
		return -CUI_EPARA;

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

static cui_ext_operation Narukana_xsa_operation = {
	Narukana_xsa_match,					/* match */
	Narukana_bv_extract_directory,		/* extract_directory */
	Narukana_bv_parse_resource_info,	/* parse_resource_info */
	Narukana_bv_extract_resource,		/* extract_resource */
	Narukana_bv_save_resource,			/* save_resource */
	Narukana_bv_release_resource,		/* release_resource */
	Narukana_bv_release					/* release */
};

/********************* 4ag *********************/

static int Narukana_4ag_match(struct package *pkg)
{
	_4ag_header_t *_4ag_header;
	
	if (!pkg)
		return -CUI_EPARA;

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

	if (memcmp(_4ag_header->magic, "GAF4", 4)) {
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

static int Narukana_4ag_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{		
	_4ag_header_t *_4ag_header;
	char mbcs_4ag_name[260];
	char *_4ag_name_lower;
	BYTE *dec_buf;
	unsigned int dec_buf_len;
	unsigned int len0, len1;
	unsigned int i;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

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

	if (!Narukana_crc_match(check_data, check_data_len)) {
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

	if (strcmp(_4ag_header->desc, "generic") && strcmp(_4ag_header->desc, "NarukanaTrial") && strcmp(_4ag_header->desc, "Narukana")) {
		free(index_offset_dec_buf);
		return -CUI_EMATCH;
	}

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

		if (!Narukana_crc_match((unsigned char *)&dset, sizeof(dset))) {
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

			if (!Narukana_crc_match((unsigned char *)&sub_chunk, sizeof(sub_chunk))) {
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

				if (!Narukana_crc_match((unsigned char *)&pict_parameter, sizeof(pict_parameter))) {
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

				if (!Narukana_crc_match((unsigned char *)buf, sub_chunk.length)) {
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

static int Narukana_4ag_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_4ag_entry_t *my_4ag_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_4ag_entry = (my_4ag_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_4ag_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_4ag_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_4ag_entry->offset;

	return 0;
}

static int Narukana_4ag_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_4ag_private_t *_4ag_private;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

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

static int Narukana_4ag_save_resource(struct resource *res, 
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

static void Narukana_4ag_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Narukana_4ag_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	my_4ag_private_t *_4ag_private;

	if (!pkg || !pkg_dir)
		return;

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

static cui_ext_operation Narukana_4ag_operation = {
	Narukana_4ag_match,					/* match */
	Narukana_4ag_extract_directory,		/* extract_directory */
	Narukana_4ag_parse_resource_info,	/* parse_resource_info */
	Narukana_4ag_extract_resource,		/* extract_resource */
	Narukana_4ag_save_resource,			/* save_resource */
	Narukana_4ag_release_resource,		/* release_resource */
	Narukana_4ag_release				/* release */
};

int CALLBACK Narukana_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &Narukana_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".xarc"), NULL, 
		NULL, &Narukana_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Narukana_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".4ag"), NULL, 
		NULL, &Narukana_4ag_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &Narukana_bv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Narukana_bv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".xsa"), NULL, 
		NULL, &Narukana_xsa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
