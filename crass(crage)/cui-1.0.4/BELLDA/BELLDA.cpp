#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information BELLDA_cui_information = {
	_T("BELL-DA"),			/* copyright */
	NULL,					/* system */
	_T(".DAT"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("≥’ùhπ´Ÿ\"),			/* author */
	_T("2009-2-18 22:40"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[8];	// "BLD01\x1a" or "BLD012\x1a" or "BLD03\x1a"
	u16 index_entries;
	u32 index_offset;
	u16 reserved;
} DAT_header_t;

typedef struct {
	s8 name[12];
	u32 length;
} DAT_entry_t;

typedef struct {
	s8 magic[2];	// "D1" or "E1"
	u32 comprlen;
	u32 uncomprlen;
} bmp_header_t;
#pragma pack ()

static u16 wav_conv_table[256] = {
		0x8000, 0x8001, 0x8786, 0x8e99, 0x9542, 0x9b87, 0xa16e, 0xa6fc, 
		0xac37, 0xb123, 0xb5c5, 0xba22, 0xbe3c, 0xc21a, 0xc5bd, 0xc929, 
		0xcc62, 0xcf6b, 0xd246, 0xd4f6, 0xd77e, 0xd9df, 0xdc1d, 0xde39, 
		0xe036, 0xe215, 0xe3d7, 0xe57f, 0xe70e, 0xe886, 0xe9e8, 0xeb35, 
		0xec6e, 0xed95, 0xeeab, 0xefb0, 0xf0a6, 0xf18e, 0xf268, 0xf335, 
		0xf3f6, 0xf4ac, 0xf557, 0xf5f8, 0xf690, 0xf71f, 0xf7a5, 0xf823, 
		0xf89a, 0xf90a, 0xf974, 0xf9d7, 0xfa35, 0xfa8d, 0xfadf, 0xfb2d, 
		0xfb77, 0xfbbc, 0xfbfd, 0xfc3a, 0xfc74, 0xfcaa, 0xfcdd, 0xfd0d, 
		0xfd3a, 0xfd65, 0xfd8d, 0xfdb2, 0xfdd6, 0xfdf7, 0xfe17, 0xfe34, 
		0xfe50, 0xfe6a, 0xfe83, 0xfe9a, 0xfeb0, 0xfec5, 0xfed8, 0xfeea, 
		0xfefc, 0xff0c, 0xff1b, 0xff29, 0xff37, 0xff44, 0xff4f, 0xff5b, 
		0xff65, 0xff6f, 0xff79, 0xff81, 0xff8a, 0xff92, 0xff99, 0xffa0, 
		0xffa6, 0xffad, 0xffb2, 0xffb8, 0xffbd, 0xffc2, 0xffc6, 0xffcb, 
		0xffcf, 0xffd2, 0xffd6, 0xffd9, 0xffdc, 0xffdf, 0xffe2, 0xffe5, 
		0xffe7, 0xffea, 0xffec, 0xffee, 0xfff0, 0xfff2, 0xfff3, 0xfff5, 
		0xfff7, 0xfff8, 0xfff9, 0xfffb, 0xfffc, 0xfffd, 0xfffe, 0xffff, 
		0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0008, 
		0x0009, 0x000b, 0x000d, 0x000e, 0x0010, 0x0012, 0x0014, 0x0016, 
		0x0019, 0x001b, 0x001e, 0x0021, 0x0024, 0x0027, 0x002a, 0x002e, 
		0x0031, 0x0035, 0x003a, 0x003e, 0x0043, 0x0048, 0x004e, 0x0053, 
		0x005a, 0x0060, 0x0067, 0x006e, 0x0076, 0x007f, 0x0087, 0x0091, 
		0x009b, 0x00a5, 0x00b1, 0x00bc, 0x00c9, 0x00d7, 0x00e5, 0x00f4, 
		0x0104, 0x0116, 0x0128, 0x013b, 0x0150, 0x0166, 0x017d, 0x0196, 
		0x01b0, 0x01cc, 0x01e9, 0x0209, 0x022a, 0x024e, 0x0273, 0x029b, 
		0x02c6, 0x02f3, 0x0323, 0x0356, 0x038c, 0x03c6, 0x0403, 0x0444, 
		0x0489, 0x04d3, 0x0521, 0x0573, 0x05cb, 0x0629, 0x068c, 0x06f6, 
		0x0766, 0x07dd, 0x085b, 0x08e1, 0x0970, 0x0a08, 0x0aa9, 0x0b54, 
		0x0c0a, 0x0ccb, 0x0d98, 0x0e72, 0x0f5a, 0x1050, 0x1155, 0x126b, 
		0x1392, 0x14cb, 0x1618, 0x177a, 0x18f2, 0x1a81, 0x1c29, 0x1deb, 
		0x1fca, 0x21c7, 0x23e3, 0x2621, 0x2882, 0x2b0a, 0x2dba, 0x3095, 
		0x339e, 0x36d7, 0x3a43, 0x3de6, 0x41c4, 0x45de, 0x4a3b, 0x4edd, 
		0x53c9, 0x5904, 0x5e92, 0x6479, 0x6abe, 0x7167, 0x787a, 0x7fff, 
};

static void bmp_decompress(BYTE *out, DWORD out_len, BYTE *in)
{
	BYTE *end_out;
	DWORD win_pos;
	BYTE win[4096];

	win_pos = 0xfee;
	end_out = out + out_len;
	memset(win, 0, win_pos);

	while (out < end_out) {
		BYTE flags = *in++;

		for (DWORD k = 0; k < 8; ++k) {
			if (flags & 1) {
				BYTE d = *in++;
				*out++ = d;
				win[win_pos++] = d;
				win_pos &= 0xfff;
			} else {
				DWORD copy_bytes, win_offset;

				win_offset = *in++;
				copy_bytes = *in++;
				win_offset |= (copy_bytes & 0xf0) << 4;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (DWORD i = 0; i < copy_bytes; ++i) {	
					BYTE d;

					d = win[(win_offset + i) & 0xfff];
					*out++ = d;
					win[win_pos++] = d;
					win_pos &= 0xfff;	
				}
			}
			flags >>= 1;
		}
	}
}

/********************* DAT *********************/

static int BELLDA_DAT_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "BLD01\x1a", sizeof(magic))
			&& strncmp(magic, "BLD02\x1a", sizeof(magic))
			&& strncmp(magic, "BLD03\x1a", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int BELLDA_DAT_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	DAT_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->seek(pkg, header.index_offset, IO_SEEK_SET))
		return -CUI_ESEEK;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	DWORD index_len = fsize - header.index_offset;
	DAT_entry_t *index = new DAT_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(DAT_entry_t);
	package_set_private(pkg, sizeof(header));

	return 0;
}

static int BELLDA_DAT_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	DAT_entry_t *DAT_entry;

	DAT_entry = (DAT_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, DAT_entry->name, 12);
	pkg_res->name_length = 12;
	pkg_res->raw_data_length = DAT_entry->length;
	pkg_res->actual_data_length = 0;
	DWORD offset = (DWORD)package_get_private(pkg);
	pkg_res->offset = offset;
	package_set_private(pkg, offset + DAT_entry->length);

	return 0;
}

static int BELLDA_DAT_extract_resource(struct package *pkg,
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

	if (!strncmp((char *)raw, "PW10", 4)) {
		memcpy(raw, "RIFF", 4);
		memcpy(raw + 8, "WAVE", 4);
		BYTE *data = raw + 0x2a;
		u32 data_len = *(u32 *)data;
		BYTE *dec = new BYTE[data_len * 2 + 0x2e];
		if (!dec) {
			delete [] raw;
			return -CUI_EMEM;		
		}

		u16 *p = (u16 *)(dec + 0x2e);
		for (DWORD i = 0; i < data_len; ++i)
			*p++ = wav_conv_table[*data++];
		memcpy(dec, raw, 0x2a);
		*(u32 *)(dec + 0x1c) = *(u32 *)(dec + 0x18) * 2;
		*(u32 *)(dec + 0x2a) = data_len * 2;
		*(u16 *)(dec + 0x20) = 2;	// 16 bits
		*(u16 *)(dec + 0x22) = 16;
		*(u32 *)(dec + 4) = data_len * 2 + 0x2e - 8;
		pkg_res->actual_data = dec;
		pkg_res->actual_data_length = data_len * 2 + 0x2e;
	} else if (!strncmp((char *)raw, "D1", 2) || !strncmp((char *)raw, "E1", 2)) {
		bmp_header_t *header = (bmp_header_t *)raw;
		BYTE *compr = (BYTE *)(header + 1);
		BYTE *uncompr = new BYTE[header->uncomprlen + 4096];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		bmp_decompress(uncompr, header->uncomprlen, compr);

		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = header->uncomprlen;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static int BELLDA_DAT_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data, pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data, pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

static void BELLDA_DAT_release_resource(struct package *pkg, 
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

static void BELLDA_DAT_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation BELLDA_DAT_operation = {
	BELLDA_DAT_match,				/* match */
	BELLDA_DAT_extract_directory,	/* extract_directory */
	BELLDA_DAT_parse_resource_info,	/* parse_resource_info */
	BELLDA_DAT_extract_resource,	/* extract_resource */
	BELLDA_DAT_save_resource,		/* save_resource */
	BELLDA_DAT_release_resource,	/* release_resource */
	BELLDA_DAT_release				/* release */
};

int CALLBACK BELLDA_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".DAT"), NULL, 
		NULL, &BELLDA_DAT_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
