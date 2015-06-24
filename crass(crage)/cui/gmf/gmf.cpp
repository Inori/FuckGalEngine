#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information gmf_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".mma .pak .dat .arc .nsa .ypf .fpk .odn .alm .ngs .mpk .bin .yukke .wav .dow .TCD .hxp .mp_ .sfd .hum"),		/* package */
	_T("0.3.8"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-27 23:14"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

/********************* mpg *********************/

/* 封包匹配回调函数 */
static int gmf_mpg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	// MPEG-1 PS
	if (memcmp(magic, "\x00\x00\x01\xba", 4) && memcmp(magic, "\x00\x00\x01\xb3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int gmf_extract_resource(struct package *pkg,
								struct package_resource *pkg_res)
{
	u32 size;
	if (pkg->pio->length_of(pkg, &size))
		return -CUI_ELEN;

	BYTE *data = new BYTE[size];
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, size, 0, IO_SEEK_SET)) {
		delete [] data;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = data;

	return 0;
}

/* 资源保存函数 */
static int gmf_save_resource(struct resource *res, 
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
static void gmf_release_resource(struct package *pkg, 
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
static void gmf_release(struct package *pkg, 
						struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_mpg_operation = {
	gmf_mpg_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	gmf_extract_resource,		/* extract_resource */
	gmf_save_resource,			/* save_resource */
	gmf_release_resource,		/* release_resource */
	gmf_release					/* release */
};

/********************* wmv *********************/

/* 封包匹配回调函数 */
static int gmf_wmv_match(struct package *pkg)
{
	s8 magic[10];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "\x30\x26\xb2\x75\x8e\x66\xcf\x11\xa6\xd9", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_wmv_operation = {
	gmf_wmv_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	gmf_extract_resource,		/* extract_resource */
	gmf_save_resource,			/* save_resource */
	gmf_release_resource,		/* release_resource */
	gmf_release					/* release */
};

/********************* ogg *********************/

/* 封包匹配回调函数 */
static int gmf_ogg_match(struct package *pkg)
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

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_ogg_operation = {
	gmf_ogg_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/********************* wav *********************/

/* 封包匹配回调函数 */
static int gmf_wav_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_wav_operation = {
	gmf_wav_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/********************* dds *********************/

/* 封包匹配回调函数 */
static int gmf_dds_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DDS ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_dds_operation = {
	gmf_dds_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/********************* png *********************/

/* 封包匹配回调函数 */
static int gmf_png_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "\x89PNG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_png_operation = {
	gmf_png_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/********************* avi *********************/

/* 封包匹配回调函数 */
static int gmf_avi_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RIFF", 4) || strncmp(magic+8, "AVI ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_avi_operation = {
	gmf_avi_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/********************* mpga *********************/

/* 封包匹配回调函数 */
static int gmf_mpga_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if ((magic & 0xffffff) != 0x90fbff) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation gmf_mpga_operation = {
	gmf_mpga_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	gmf_extract_resource,	/* extract_resource */
	gmf_save_resource,		/* save_resource */
	gmf_release_resource,	/* release_resource */
	gmf_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK gmf_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".hum"), _T(".ogg"), 
		NULL, &gmf_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".png"), 
		NULL, &gmf_png_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// for NekoSDK & N2System
	if (callback->add_extension(callback->cui, NULL, _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	// for NekoSDK
	if (callback->add_extension(callback->cui, _T(".dat"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// for BGI
	if (callback->add_extension(callback->cui, _T(".arc"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// NScripter
	if (callback->add_extension(callback->cui, _T(".nsa"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// YU-RIS
	if (callback->add_extension(callback->cui, _T(".ypf"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// SystemC
	if (callback->add_extension(callback->cui, _T(".fpk"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// System4
	if (callback->add_extension(callback->cui, _T(".alm"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// VALKYRIA
	if (callback->add_extension(callback->cui, _T(".odn"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// N2System
	if (callback->add_extension(callback->cui, _T(".ngs"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// M45
	if (callback->add_extension(callback->cui, _T(".mpk"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// Xuse
	if (callback->add_extension(callback->cui, _T(".bin"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// System 4
	if (callback->add_extension(callback->cui, _T(".yukke"), _T(".wmv"), 
		NULL, &gmf_wmv_operation, CUI_EXT_FLAG_PKG))
			return -1;
	
	// Seraphim & Kaguya
	if (callback->add_extension(callback->cui, _T(".wav"), _T(".ogg"), 
		NULL, &gmf_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// old YU-RIS
	if (callback->add_extension(callback->cui, _T(".dat"), _T(".ogg"), 
		NULL, &gmf_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// Gamesys
	if (callback->add_extension(callback->cui, _T(".dow"), _T(".ogg"), 
		NULL, &gmf_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// Gamesys
	if (callback->add_extension(callback->cui, _T(".dow"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// Gamesys
	if (callback->add_extension(callback->cui, _T(".dow"), _T(".wav"), 
		NULL, &gmf_wav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// TCD
	if (callback->add_extension(callback->cui, _T(".TCD"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// SHSystem
	if (callback->add_extension(callback->cui, _T(".hxp"), _T(".wmv"), 
		NULL, &gmf_wmv_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// (同人ゲーム)[081227][梅麻呂3D]双子の小悪魔
	if (callback->add_extension(callback->cui, _T(".mp_"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// ヴァルキリーコンプレックス
	if (callback->add_extension(callback->cui, _T(".pak"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// Hypatia
	if (callback->add_extension(callback->cui, _T(".sfd"), _T(".mpg"), 
		NULL, &gmf_mpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// (C75)(同人)爆裂生徒会 体験版 ver.1.02
	if (callback->add_extension(callback->cui, _T(".arc"), _T(".dds"), 
		NULL, &gmf_dds_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, NULL, _T(".wav"), 
		NULL, &gmf_wav_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mma"), _T(".avi"), 
		NULL, &gmf_avi_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nsa"), _T(".mpga"), 
		NULL, &gmf_mpga_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
