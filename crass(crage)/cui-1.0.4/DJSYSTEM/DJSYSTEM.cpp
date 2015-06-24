#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information DJSYSTEM_cui_information = {
	NULL,					/* copyright */
	_T("DJSYSTEM"),			/* system */
	_T(".dat"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-4-20 21:32"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)	
typedef struct {
	s8 name[128];
	u32 offset;
	u32 length;
} dat_entry_t;
#pragma pack ()

static int CodeFileReadOneLine(struct package *pkg, char *out)
{
	unsigned char chr;
	int len = 0;
	
	while (1) {
		if (pkg->pio->read(pkg, &chr, 1))
			return -CUI_EREAD;

		*out++ = chr;
		len++;
		if (chr == '\n')
			break;
	}

	return len;
}

static int CodeFileParseOneLine(struct package *pkg, dat_entry_t *dat_entry)
{
	int line_len;
	char line[512];

	memset(line, 0, sizeof(line));
	line_len = CodeFileReadOneLine(pkg, line);
	if (line_len < 0)
		return line_len;

	if (!strcmp(line, "LIST-END\n"))
		return 1;

	if (sscanf(line, "%s\t%d\t%d\n", dat_entry->name, 
		&dat_entry->offset, &dat_entry->length) != 3)
			return -1;
	dat_entry->length -= dat_entry->offset;

	return 0;
}

/********************* dat *********************/

static int DJSYSTEM_dat_match(struct package *pkg)
{
	s8 magic[21];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp(magic, "FILECMB-DATA-LIST-IN\n", 21)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int DJSYSTEM_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	char entry_buffer[512];
	unsigned int index_buffer_length;	
	unsigned int i;

	pkg_dir->index_entries = 0;	
	while (1) {
		int err;

		memset(entry_buffer, 0, sizeof(entry_buffer));
		err = CodeFileReadOneLine(pkg, entry_buffer);
		if (err < 0)
			return err;

		if (!strcmp(entry_buffer, "LIST-END\n"))
			break;

		pkg_dir->index_entries++;
	}

	if (pkg->pio->seek(pkg, 21, IO_SEEK_SET))
		return -CUI_ESEEK;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (CodeFileParseOneLine(pkg, &index_buffer[i]))
			break;
	}
	if (i != pkg_dir->index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

static int DJSYSTEM_dat_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

static int DJSYSTEM_dat_extract_resource(struct package *pkg,
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

	if (!strncmp((char *)raw, "DJCODE NLINE-ENCODE\n", 20)) {
		pkg_res->actual_data_length = pkg_res->raw_data_length - 20;
		BYTE *act = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!act) {
			free(raw);
			return -CUI_EMEM;
		}

		for (DWORD i = 0; i < pkg_res->actual_data_length; ++i)
			act[i] = ~raw[i + 20];

		pkg_res->actual_data = act;
	} else if (!strncmp((char *)raw, "DJCODE NLINE-NO-ENCODE\n", 23)) {
		pkg_res->actual_data_length = pkg_res->raw_data_length - 23;
		BYTE *act = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!act) {
			free(raw);
			return -CUI_EMEM;
		}

		for (DWORD i = 0; i < pkg_res->actual_data_length; ++i) {
			if (raw[i + 23] != '\r' && raw[i + 23] != '\n')
				act[i] = ~raw[i + 23];
			else
				act[i] = raw[i + 23];
		}

		pkg_res->actual_data = act;
	}
	pkg_res->raw_data = raw;

	return 0;
}

static int DJSYSTEM_dat_save_resource(struct resource *res, 
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

static void DJSYSTEM_dat_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void DJSYSTEM_dat_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation DJSYSTEM_dat_operation = {
	DJSYSTEM_dat_match,					/* match */
	DJSYSTEM_dat_extract_directory,		/* extract_directory */
	DJSYSTEM_dat_parse_resource_info,	/* parse_resource_info */
	DJSYSTEM_dat_extract_resource,		/* extract_resource */
	DJSYSTEM_dat_save_resource,			/* save_resource */
	DJSYSTEM_dat_release_resource,		/* release_resource */
	DJSYSTEM_dat_release				/* release */
};

int CALLBACK DJSYSTEM_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DJSYSTEM_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
