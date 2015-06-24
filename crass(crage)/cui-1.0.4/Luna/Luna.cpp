#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information Luna_cui_information = {
	_T("葉迩倭"),			/* copyright */
	_T("DirectX Library \"Luna for DirectX 9.0c\""),	/* system */
	_T(".bin .p .mus"),		/* package */
	_T("1.0.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-1-26 13:04"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PACK"
	u32 index_entries;
} bin_header_t;

typedef struct {
	s8 name[64];
	u32 name_crc;
	u32 data_crc;
	u32 offset;
	u32 length;
} bin_entry_t;

typedef struct {
	s8 magic[4];	// "LZSS"
	u32 uncomprlen;
} lzss_header_t;
#pragma pack ()


static unsigned long crc(unsigned char *data, unsigned long size)
{
	unsigned long r = 0xFFFFFFFF;

	for (unsigned long i = 0; i < size; i++) {
		r ^= (unsigned long)data[i];
		for (unsigned long j = 0; j < 8; j++) {
			if ((r & 1) != 0)
				r = (r >> 1) ^ 0xEDB88320;
			else
				r >>= 1;
		}
	}

	return r ^ 0xFFFFFFFF;
}

/********************* bin *********************/

static int Luna_bin_match(struct package *pkg)
{
	s8 magic[4];
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int Luna_bin_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 index_entries;
	bin_entry_t *index_buffer;
	DWORD index_length;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;
	
	index_length = index_entries * sizeof(bin_entry_t);
	index_buffer = (bin_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_entries; i++) {
		bin_entry_t *entry = &index_buffer[i];
		
		if (crc((BYTE *)entry->name, strlen(entry->name)) != entry->name_crc)
			break;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(bin_entry_t);

	return 0;
}

static int Luna_bin_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	bin_entry_t *bin_entry;

	bin_entry = (bin_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, bin_entry->name, 64);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bin_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = bin_entry->offset;

	return 0;
}

static int Luna_bin_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	bin_entry_t *bin_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;
	
	bin_entry = (bin_entry_t *)pkg_res->actual_index_entry;
	if (crc(compr, comprlen) != bin_entry->data_crc)
		return -CUI_EMATCH;
	
	if (!strncmp((char *)compr, "LZSS", 4)) {
#define LZSS_RING_LENGTH		4096					// 環状バッファの大きさ
#define LZSS_LONGEST_MATCH		16						// 最長一致長
		lzss_header_t *lzss_header = (lzss_header_t *)compr;
		long r = LZSS_RING_LENGTH - LZSS_LONGEST_MATCH;
		unsigned long Flags = 0;
		unsigned char c;
		unsigned char text[LZSS_RING_LENGTH + LZSS_LONGEST_MATCH - 1];	///< テキストデータポインタ
		
		uncomprlen = lzss_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		unsigned char *pDstData = (unsigned char *)uncompr;
		unsigned char *pSrcData = (unsigned char *)(lzss_header + 1);

		memset(text, 0x00, sizeof(text));

		//--------------------------------------------------------------
		// 展開後サイズ取得
		//--------------------------------------------------------------
		unsigned long DstSize = lzss_header->uncomprlen;

		//--------------------------------------------------------------
		// デコード処理
		//--------------------------------------------------------------
		while (1) {
			Flags >>= 1;
			if ((Flags & 256) == 0) {
				c = *pSrcData++;
				Flags = c | 0xff00;
			}

			if (Flags & 1) {
				c = *pSrcData++;
				*pDstData++ = c;
				if (--DstSize == 0)
					break;;

				text[r++] = c;
				r &= (LZSS_RING_LENGTH - 1);
			} else {
				long i = *pSrcData++;
				long j = *pSrcData++;
				i |= ((j & 0xF0) << 4);
				j = (j & 0x0F) + 3;

				for (long k = 0; k < j; k++) {
					c = text[(i + k) & (LZSS_RING_LENGTH - 1)];
					*(pDstData++) = c;
					if (--DstSize == 0)
						break;

					text[r++] = c;
					r &= (LZSS_RING_LENGTH - 1);
				}
				if (k != j)
					break;
			}
		}
	} else if (!strncmp((char *)compr, "LAG", 3)) {
		printf("TODO!\n");
		uncompr = NULL;
		uncomprlen = 0;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int Luna_bin_save_resource(struct resource *res, 
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

static void Luna_bin_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void Luna_bin_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation Luna_bin_operation = {
	Luna_bin_match,					/* match */
	Luna_bin_extract_directory,		/* extract_directory */
	Luna_bin_parse_resource_info,	/* parse_resource_info */
	Luna_bin_extract_resource,		/* extract_resource */
	Luna_bin_save_resource,			/* save_resource */
	Luna_bin_release_resource,		/* release_resource */
	Luna_bin_release				/* release */
};

int CALLBACK Luna_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Luna_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	// AIMS
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &Luna_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	// AIMS
	if (callback->add_extension(callback->cui, _T(".mus"), NULL, 
		NULL, &Luna_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	return 0;
}
