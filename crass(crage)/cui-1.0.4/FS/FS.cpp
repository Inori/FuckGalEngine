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

struct acui_information FS_cui_information = {
	_T(""),							/* copyright */
	NULL,							/* system */
	_T(".pd"),						/* package */
	_T("4.0.0"),					/* revision */
	_T("³Õºº¹«Ôô"),					/* author */
	_T("2007-11-17 14:51"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	u8 magic[20];		/* "FlyingShinePDFile" */
	u32 xor_magic;		// u8
	u32 time_stamp;		// stamp [20040514, 20040810]
	u32 index_entries;
} pd_header_t;

typedef struct {
	s8 name[36];
	u32 xor_magic;		// u8
	u32 offset;
	u32 length;
} pd_entry_t;

typedef struct {
	s8 magic[8];		/* "PackPlus" or "PackOnly" */
	u8 reserved[56];
	u32 index_entries;		
	u32 pad;
} pdpack_header_t;

// 0x34 bytes
typedef struct {
	s8 magic[9];		/* "PackPlus" or "PackOnly" */
	u8 reserved[34];
	u8 xor_magic;
	u32 index_entries;	
	u32 index_offset;
} pdpack_header2_t;

typedef struct {
	s8 name[128];
	u32 offset;
	u32 pad0;
	u32 length;
	u32 pad1;
} pdpack_entry_t;

// 48 bytes
typedef struct {
	s8 name[39];		/* "PackPlus" or "PackOnly" */
	s8 sub;
	u32 offset;
	u32 length;
} pdpack_entry2_t;
#pragma pack ()		

static void decode(BYTE *enc_buf, DWORD enc_buf_len, u8 xor_magic)
{
	if (enc_buf_len > 768 * 1024)
		enc_buf_len = 768 * 1024;
	
	for (DWORD i = 0; i < enc_buf_len; i++)
		enc_buf[i] ^= xor_magic;
}	

/********************* pd *********************/

static int PdPack_pd_match(struct package *pkg)
{	
	s8 magic[8];
	u32 index_entries;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (memcmp(magic, "PackPlus", 8) && memcmp(magic, "PackOnly", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, &index_entries, 4, 0x2c, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int PdPack_pd_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{	
	pdpack_header_t pdpack_header; 
	unsigned int index_buffer_len;
	BYTE *index_buffer;
	int packplus;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &pdpack_header, sizeof(pdpack_header)))
		return -CUI_EREAD;

	if (!memcmp(pdpack_header.magic, "PackPlus", 8))
		packplus = 1;
	else if (!memcmp(pdpack_header.magic, "PackOnly", 8))
		packplus = 0;
	else
		return -CUI_EMATCH;

	index_buffer_len = pdpack_header.index_entries * sizeof(pdpack_entry_t);		
	index_buffer = (BYTE *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pdpack_header.index_entries;
	pkg_dir->index_entry_length = sizeof(pdpack_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	package_set_private(pkg, (void *)packplus);

	return 0;
}

static int PdPack_pd_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	pdpack_entry_t *pdpack_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pdpack_entry = (pdpack_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pdpack_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pdpack_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pdpack_entry->offset;

	return 0;
}

static int PdPack_pd_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{	
	BYTE *data;
	int packplus;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	packplus = (int)package_get_private(pkg);

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	if (packplus)
		for (unsigned int k = 0; k < pkg_res->raw_data_length; k++)
			data[k] = ~data[k];

	pkg_res->raw_data = data;

	return 0;
}

static int PdPack_pd_save_resource(struct resource *res, 
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

static void PdPack_pd_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void PdPack_pd_release(struct package *pkg, 
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

static cui_ext_operation PdPack_pd_operation = {
	PdPack_pd_match,				/* match */
	PdPack_pd_extract_directory,	/* extract_directory */
	PdPack_pd_parse_resource_info,	/* parse_resource_info */
	PdPack_pd_extract_resource,		/* extract_resource */
	PdPack_pd_save_resource,		/* save_resource */
	PdPack_pd_release_resource,		/* release_resource */
	PdPack_pd_release				/* release */
};

/********************* pd *********************/

static int PdPack_pd_match2(struct package *pkg)
{	
	s8 magic[9];
	u32 index_entries;
	int packplus;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "PackPlus", 9))
		packplus = 1;
	else if (!memcmp(magic, "PackOnly", 9))
		packplus = 0;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, &index_entries, 4, 0x2c, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	package_set_private(pkg, (void *)packplus);

	return 0;	
}

static int PdPack_pd_extract_directory2(struct package *pkg,
										struct package_directory *pkg_dir)
{	
	pdpack_header2_t pdpack_header2; 
	DWORD index_buffer_len;
	pdpack_entry2_t *index_buffer;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &pdpack_header2, sizeof(pdpack_header2)))
		return -CUI_EREAD;

	index_buffer_len = pdpack_header2.index_entries * sizeof(pdpack_entry2_t);		
	index_buffer = (pdpack_entry2_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_len, 
			pdpack_header2.index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pdpack_header2.index_entries; i++) {
		index_buffer[i].offset = (int)index_buffer[i].offset - (int)index_buffer[i].sub;
		index_buffer[i].length = (int)index_buffer[i].length - (int)index_buffer[i].sub;
		index_buffer[i].sub = pdpack_header2.xor_magic;
	}

	pkg_dir->index_entries = pdpack_header2.index_entries;
	pkg_dir->index_entry_length = sizeof(pdpack_entry2_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int PdPack_pd_parse_resource_info2(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pdpack_entry2_t *pdpack_entry2;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pdpack_entry2 = (pdpack_entry2_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pdpack_entry2->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pdpack_entry2->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pdpack_entry2->offset;

	return 0;
}

static int PdPack_pd_extract_resource2(struct package *pkg,
									   struct package_resource *pkg_res)
{	
	BYTE *data;
	int packplus;
	pdpack_entry2_t *pdpack_entry2;	

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	packplus = (int)package_get_private(pkg);

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	pdpack_entry2 = (pdpack_entry2_t *)pkg_res->actual_index_entry;
	if (packplus) {
		for (unsigned int k = 0; k < pkg_res->raw_data_length; k++)
			data[k] ^= pdpack_entry2->sub;
	} else if (!strstr(pkg_res->name, ".png"))
		data[0x1a] ^= pdpack_entry2->sub;

	pkg_res->raw_data = data;
	return 0;
}

static int PdPack_pd_save_resource2(struct resource *res, 
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

static void PdPack_pd_release_resource2(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void PdPack_pd_release2(struct package *pkg, 
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

static cui_ext_operation PdPack_pd_operation2 = {
	PdPack_pd_match2,				/* match */
	PdPack_pd_extract_directory2,	/* extract_directory */
	PdPack_pd_parse_resource_info2,	/* parse_resource_info */
	PdPack_pd_extract_resource2,	/* extract_resource */
	PdPack_pd_save_resource2,		/* save_resource */
	PdPack_pd_release_resource2,	/* release_resource */
	PdPack_pd_release2				/* release */
};

/********************* pd *********************/

static int FS_pd_match(struct package *pkg)
{	
	pd_header_t pd_header;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &pd_header, sizeof(pd_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (memcmp(pd_header.magic, "FlyingShinePDFile", 18)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pd_header.time_stamp < 20040514 || pd_header.time_stamp > 20040810) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int FS_pd_extract_directory(struct package *pkg,
												  struct package_directory *pkg_dir)
{	
	pd_header_t pd_header;	
	unsigned int index_buffer_len;
	BYTE *index_buffer;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->read(pkg, &pd_header, sizeof(pd_header)))
		return -CUI_EREAD;

	index_buffer_len = pd_header.index_entries * sizeof(pd_entry_t);		
	index_buffer = (BYTE *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (unsigned int i = 0; i < index_buffer_len; i++)
		index_buffer[i] ^= pd_header.xor_magic;

	pkg_dir->index_entries = pd_header.index_entries;
	pkg_dir->index_entry_length = sizeof(pd_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int FS_pd_parse_resource_info(struct package *pkg,
													struct package_resource *pkg_res)
{
	pd_entry_t *pd_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pd_entry = (pd_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pd_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pd_entry->length - pd_entry->xor_magic;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pd_entry->offset - pd_entry->xor_magic;

	return 0;
}

static int FS_pd_extract_resource(struct package *pkg,
												 struct package_resource *pkg_res)
{	
	pd_entry_t *pd_entry;
	BYTE *data;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	pd_entry = (pd_entry_t *)pkg_res->actual_index_entry;

	char *ext = PathFindExtensionA(pkg_res->name);
	if (ext) {	// ¥¯¥é¥¤¥ß¥é¥¤2 trial FSFile: 37235d
		if (!(pkg_res->name[0] == 'e' && pkg_res->name[1] == 'x')) {
			if (!strcmp(ext, ".def") || !strcmp(ext, ".dsf")) {
				decode(data, pkg_res->raw_data_length, pd_entry->xor_magic);
				if (!lstrcmpiA(ext, ".def"))
					pkg_res->replace_extension = _T(".def.txt");
				else if (!lstrcmpiA(ext, ".dsf"))
					pkg_res->replace_extension = _T(".dsf.txt");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else if (!lstrcmpiA(ext, ".ogg"))
				data[0x1a] ^= pd_entry->xor_magic;
		}
	}

	pkg_res->raw_data = data;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = pkg_res->raw_data_length;	

	return 0;
}

static int FS_pd_save_resource(struct resource *res, 
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

static void FS_pd_release_resource(struct package *pkg, 
												  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void FS_pd_release(struct package *pkg, 
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

static cui_ext_operation FS_pd_operation = {
	FS_pd_match,				/* match */
	FS_pd_extract_directory,	/* extract_directory */
	FS_pd_parse_resource_info,	/* parse_resource_info */
	FS_pd_extract_resource,		/* extract_resource */
	FS_pd_save_resource,		/* save_resource */
	FS_pd_release_resource,		/* release_resource */
	FS_pd_release				/* release */
};
	
int CALLBACK FS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pd"), NULL, 
		NULL, &FS_pd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pd"), NULL, 
		NULL, &PdPack_pd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pd"), NULL, 
		NULL, &PdPack_pd_operation2, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
