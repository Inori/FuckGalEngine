#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information FORIS_cui_information = {
	_T("STACK"),			/* copyright */
	_T(""),					/* system */
	_T(".GPK .GPK.*"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-21 10:16"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 sig0[12];
	u32 pidx_length;
	s8 sig1[16];
} GPK_sig_t;

// gpk pidx format:
typedef struct {
	unsigned short unilen;			// unicode string length
	unsigned short *unistring;		// unicode string exclude NULL
	unsigned short sub_version;		// same as script.gpk.* suffix
	unsigned short version;			// major version(always 1) 
	unsigned short zero;			// always 0
	unsigned int offset;			// pidx data file offset
	unsigned int comprlen;			// compressed pidx data length
	unsigned char dflt[4];			// magic "DFLT" or "    "
	unsigned int uncomprlen;		// raw pidx data length(if magic isn't DFLT, then this filed always zero)
	unsigned char comprheadlen;		// pidx data header length
	unsigned char *comprheaddata;	// pidx data
} GPK_pidx_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

static int no_exe_parameter = 1;
static u8 ciphercode[16];

static void GPK_decode(BYTE *buffer, DWORD buffer_len)
{
	unsigned int i = 0, k = 0;
	
	while (i < buffer_len) {
		buffer[i++] ^= ciphercode[k++];	
		if (k >= 16)
			k = 0;
	}
}

/********************* GPK *********************/

/* 封包匹配回调函数 */
static int FORIS_GPK_match(struct package *pkg)
{
	if (no_exe_parameter)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 offset;
	pkg->pio->length_of(pkg, &offset);

	offset -= sizeof(GPK_sig_t);
	if (pkg->pio->seek(pkg, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	GPK_sig_t GPK_sig;
	if (pkg->pio->read(pkg, &GPK_sig, sizeof(GPK_sig))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

#define GPK_TAILER_IDENT0	"STKFile0PIDX"
#define GPK_TAILER_IDENT1	"STKFile0PACKFILE"

	if (strncmp(GPK_TAILER_IDENT0, GPK_sig.sig0, strlen(GPK_TAILER_IDENT0))
			|| strncmp(GPK_TAILER_IDENT1, GPK_sig.sig1, strlen(GPK_TAILER_IDENT1))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int FORIS_GPK_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	u32 offset;
	pkg->pio->length_of(pkg, &offset);

	offset -= sizeof(GPK_sig_t);
	GPK_sig_t GPK_sig;
	if (pkg->pio->readvec(pkg, &GPK_sig, sizeof(GPK_sig), offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 comprlen = GPK_sig.pidx_length;
	offset -= comprlen;	/* point to pidx segment */ 
	if (pkg->pio->seek(pkg, offset, IO_SEEK_SET))
		return -CUI_ESEEK;

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, comprlen)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	GPK_decode(compr, comprlen);

	u32 uncomprlen = *(u32 *)compr;	
	comprlen -= 4;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr + 4, comprlen) != Z_OK) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;
	}
	delete [] compr;

	BYTE *p = uncompr;
	pkg_dir->index_entries = 0;
	while (1) {
		u16 nlen = *(u16 *)p;
		if (!nlen)
			break;
		p += 2 + nlen * 2 + 22;
		p += *p + 1;
		pkg_dir->index_entries++;
	}

	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = act_uncomprlen;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

/* 封包索引项解析函数 */
static int FORIS_GPK_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *entry = (BYTE *)pkg_res->actual_index_entry;
	pkg_res->name_length = *(u16 *)entry;
	entry += 2;
	wcsncpy((WCHAR *)pkg_res->name, (WCHAR *)entry, pkg_res->name_length);
	entry += pkg_res->name_length * 2 + 6;
	pkg_res->offset = *(u32 *)entry;
	entry += 4;
	pkg_res->raw_data_length = *(u32 *)entry;
	entry += 4;
	/* magic "DFLT" or "    ",
	 * if magic isn't DFLT, this filed always zero
	 */
	entry += 4;
	pkg_res->actual_data_length = *(u32 *)entry;
	entry += 4;
	BYTE compr_head_len = *entry;
	pkg_res->actual_index_entry_length = 2 + pkg_res->name_length * 2 
		+ 22 + compr_head_len + 1;
	pkg_res->flags = PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int FORIS_GPK_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *entry = (BYTE *)pkg_res->actual_index_entry 
		+ 2 + pkg_res->name_length * 2 + 22;
	BYTE compr_head_len = *entry++;
	DWORD comprlen = compr_head_len + pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	memcpy(compr, entry, compr_head_len);
	if (pkg->pio->readvec(pkg, compr + compr_head_len, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	BYTE *uncompr;
	if (pkg_res->actual_data_length) {
		DWORD uncomprlen = pkg_res->actual_data_length;
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		if (uncompress(uncompr, &uncomprlen, compr, comprlen) != Z_OK) {
			delete [] uncompr;
			delete [] compr;
			return -CUI_EUNCOMPR;
		}
	} else
		uncompr = NULL;

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int FORIS_GPK_save_resource(struct resource *res, 
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
static void FORIS_GPK_release_resource(struct package *pkg, 
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
static void FORIS_GPK_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation FORIS_GPK_operation = {
	FORIS_GPK_match,				/* match */
	FORIS_GPK_extract_directory,	/* extract_directory */
	FORIS_GPK_parse_resource_info,	/* parse_resource_info */
	FORIS_GPK_extract_resource,		/* extract_resource */
	FORIS_GPK_save_resource,		/* save_resource */
	FORIS_GPK_release_resource,		/* release_resource */
	FORIS_GPK_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK FORIS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".GPK"), NULL, 
		NULL, &FORIS_GPK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".GPK.*"), NULL, 
		NULL, &FORIS_GPK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	const char *exe_file = get_options("exe");
	no_exe_parameter = 1;
	if (exe_file) {
		HMODULE exe = LoadLibraryA(exe_file);
		if ((DWORD)exe > 31) {
			HRSRC code = FindResourceA(exe, "CIPHERCODE", "CODE");
			if (code) {				
				DWORD sz = SizeofResource(exe, code); 
				if (sz == 16) {
					HGLOBAL hsrc = LoadResource(exe, code); 
					if (hsrc) {
						LPVOID cipher = LockResource(hsrc); 
						if (cipher) {
							memcpy(ciphercode, cipher, 16);
							no_exe_parameter = 0;
						}
						FreeResource(hsrc);
					}
				} else if (sz == 20) {	// SchoolDays
					HGLOBAL hsrc = LoadResource(exe, code); 
					if (hsrc) {
						LPVOID cipher = LockResource(hsrc); 
						if (cipher && *(u32 *)cipher == 16) {
							memcpy(ciphercode, (BYTE *)cipher + 4, 16);
							no_exe_parameter = 0;
						}
						FreeResource(hsrc);
					}				
				}
			}
			FreeLibrary(exe);
		}
	}

	return 0;
}
