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

// Q:\儅儅偟傏傝俀

/* 调试C4.exe的方法：
 创建指向C4.exe的快捷方式，然后编辑启动命令，结尾加上" PL"即可脱离启动程序单独运行。
 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information C4_cui_information = {
	_T(""),		/* copyright */
	_T("C4"),			/* system */
	_T(".GD"),				/* package */
	_T("0.5.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "Mp17"
	u16 text_offset;
	u16 text_data_length;
	u16 unknown0;
	u16 unknown_offset;
	u16 unknown1;
} mpx_header_t;

typedef struct {
	s8 magic[4];		// "GD3\x1a"
} gd_header_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int GD_l_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	struct bits bits;
	BYTE window[65536];
	DWORD cur_win_pos = 1;

	bits_init(&bits, compr, comprlen);
	memset(window, 0, sizeof(window));
	while (1) {
		unsigned int flag, data;

		if (bits_get_high(&bits, 1, &flag))
			return -1;

		if (flag) {
			if (bits_get_high(&bits, 8, &data))
				return -1;

			*uncompr++ = data;
			window[cur_win_pos++] = data;
			cur_win_pos &= sizeof(window) - 1;
		} else {
			unsigned int offset, copy_bytes;

			if (bits_get_high(&bits, 16, &offset))
				return -1;

			if (!offset)
				break;

			if (bits_get_high(&bits, 4, &copy_bytes))
				return -1;

			copy_bytes += 3;

			for (DWORD i = 0; i < copy_bytes; i++) {
				data = window[(offset + i) & (sizeof(window) - 1)];
				*uncompr++ = data;
				window[cur_win_pos] = data;
				cur_win_pos++;
				cur_win_pos &= sizeof(window) - 1;
			}
		}
	}

	return 0;
}

static int GD_p_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	struct bits bits;
	BYTE *orig_uncompr = uncompr;
	unsigned int B, G, R;

	bits_init(&bits, compr, comprlen);
	memset(uncompr, 0xff, uncomprlen);
	while (1) {
		unsigned int offset;		

		if (bits_get_high(&bits, 2, &offset))
			return -1;

		if (offset == 2) {
			if (bits_get_high(&bits, 2, &offset))
				return -1;

			offset += 2;
		} else if (offset == 3) {
			unsigned int offset_cnt = 3;

			while (1) {
				unsigned int flag;

				if (bits_get_high(&bits, 1, &flag))
					return -1;

				if (!flag)
					break;

				offset_cnt++;
			}

			if (offset_cnt >= 24)
				break;

			if (bits_get_high(&bits, offset_cnt, &offset))
				return -1;

			offset = offset - 1 + ((1 << offset_cnt) - 1);

			if (offset == -1)
				break;
		}

		uncompr += offset * 3;

		if (bits_get_high(&bits, 8, &B))
			return -1;

		if (bits_get_high(&bits, 8, &G))
			return -1;

		if (bits_get_high(&bits, 8, &R))
			return -1;

		uncompr[0] = B;
		uncompr[1] = G;
		uncompr[2] = R;

		unsigned int jmp;

		if (bits_get_high(&bits, 1, &jmp))
			return -1;

		if (jmp) {
			BYTE *_uncompr = uncompr;

			while (1) {
				unsigned int jmp_type;

				if (bits_get_high(&bits, 2, &jmp_type))
					return -1;

				switch (jmp_type) {
				case 0:
					if (bits_get_high(&bits, 1, &jmp_type))
						return -1;

					if (!jmp_type)
						goto out;

					if (bits_get_high(&bits, 1, &jmp_type))
						return -1;

					if (jmp_type)
						_uncompr += 2406;
					else
						_uncompr += 2394;
					break;
				case 1:
					_uncompr += 2397;
					break;
				case 2:
					_uncompr += 2400;
					break;
				case 3:
					_uncompr += 2403;
					break;
				}
				_uncompr[0] = B;
				_uncompr[1] = G;
				_uncompr[2] = R;
			}
		}
	out:		
		uncompr += 3;
	}

	B = G = R = 0;
	uncompr = orig_uncompr;
	for (DWORD i = 0; i < 800 * 600; i++) {
		if (uncompr[0] == 0xff && uncompr[1] == uncompr[0] && uncompr[2] == uncompr[0]) {
			uncompr[0] = B;
			uncompr[1] = G;
			uncompr[2] = R;
		} else {
			B = uncompr[0];
			G = uncompr[1];
			R = uncompr[2];
		}
		uncompr += 3;
	}

	return 0;
}

/* 资源保存函数 */
static int C4_save_resource(struct resource *res, 
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
static void C4_release_resource(struct package *pkg, 
									struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void C4_release(struct package *pkg, 
						   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

#if 0
/********************* MPX *********************/

/* 封包匹配回调函数 */
static int C4_MPX_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "Mp17", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int C4_MPX_extract_resource(struct package *pkg,
								   struct package_resource *pkg_res)
{
	u32 mpx_size;
	BYTE *mpx_data, *mpx_text;
	mpx_header_t *mpx_header;

	if (pkg->pio->length_of(pkg, &mpx_size))
		return -CUI_ELEN;

	mpx_data = (BYTE *)malloc(mpx_size);
	if (!mpx_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, mpx_data, mpx_size, 0, IO_SEEK_SET)) {
		free(mpx_data);
		return -CUI_EREADVEC;
	}

	mpx_header = (mpx_header_t *)mpx_data;
	mpx_text = mpx_data + mpx_header->text_offset;
	
	for (DWORD i = 0; i < mpx_header->text_data_length; i++)
		mpx_text[i] ^= 0x24;

	lowcase(pkg_res->name);
	for (DWORD key = 0; *name; ) { 
		chr = *name++;
		if (isalpha(chr))
			chr |= 0x20;
		key = chr ^ (i * 2);
	}


	pkg_res->raw_data = mpx_data;
	pkg_res->raw_data_length = mpx_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation C4_MPX_operation = {
	C4_MPX_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	C4_MPX_extract_resource,/* extract_resource */
	C4_MPX_save_resource,	/* save_resource */
	C4_MPX_release_resource,/* release_resource */
	C4_MPX_release			/* release */
};
#endif 


/********************* GD *********************/

/* 封包匹配回调函数 */
static int C4_GD_match(struct package *pkg)
{
	s8 magic[3];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "GD3", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int C4_GD_extract_resource(struct package *pkg,
								  struct package_resource *pkg_res)
{
	u32 gd_size;
	BYTE *compr, *uncompr;
	DWORD uncomprlen;

	if (pkg->pio->length_of(pkg, &gd_size))
		return -CUI_ELEN;

	compr = (BYTE *)malloc(0x163900);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, gd_size, 0, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	memset(compr + gd_size, 0, 0x163900 - gd_size);

	uncomprlen = 800 * 600 * 3;
	if (compr[14164] == 'p') {
		BYTE *bmp;
		DWORD bmp_size;

		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (GD_p_uncompress(uncompr, uncomprlen, compr + 14166, 0x163900 - 14166)) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}

		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, 800, 600, 24,
				&bmp, &bmp_size, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
		free(uncompr);
		uncompr = bmp;
		uncomprlen = bmp_size;
	} else if (compr[14164] == 'l') {
		BYTE *bmp;
		DWORD bmp_size;

		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (GD_l_uncompress(uncompr, uncomprlen, compr + 14166, 0x163900 - 14166)) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}

		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, 800, 600, 24,
				&bmp, &bmp_size, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
		free(uncompr);
		uncompr = bmp;
		uncomprlen = bmp_size;
	} else if (compr[14164] == 'b') {
		if (MyBuildBMPFile(compr + 14166, uncomprlen, NULL, 0, 
				800, 600, 24, &uncompr, &uncomprlen, my_malloc)) {
			free(compr);
			return -CUI_EMEM;
		}
	} else {
		free(compr);
		return -CUI_EMATCH;
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = gd_size;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation C4_GD_operation = {
	C4_GD_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	C4_GD_extract_resource,	/* extract_resource */
	C4_save_resource,	/* save_resource */
	C4_release_resource,/* release_resource */
	C4_release			/* release */
};

/********************* VMD *********************/

/* 封包匹配回调函数 */
static int C4_VMD_match(struct package *pkg)
{
	u32 vmd_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &vmd_size))
		return -CUI_ELEN;

	if (vmd_size <= 3)
		return -CUI_EMATCH;

#if 0
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "GD3", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
#endif
	return 0;	
}

/* 封包资源提取函数 */
static int C4_VMD_extract_resource(struct package *pkg,
								   struct package_resource *pkg_res)
{
	u32 vmd_size;
	BYTE *raw_vmd;

	if (pkg->pio->length_of(pkg, &vmd_size))
		return -CUI_ELEN;

	raw_vmd = (BYTE *)malloc(vmd_size);
	if (!raw_vmd)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw_vmd, vmd_size, 0, IO_SEEK_SET)) {
		free(raw_vmd);
		return -CUI_EREADVEC;
	}
	
	for (DWORD i = 0; i < 1024; i++)
		raw_vmd[i] ^= 0xe5;

	pkg_res->raw_data = raw_vmd;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation C4_VMD_operation = {
	C4_VMD_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	C4_VMD_extract_resource,/* extract_resource */
	C4_save_resource,	/* save_resource */
	C4_release_resource,/* release_resource */
	C4_release			/* release */
};

int CALLBACK C4_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".GD"), _T(".bmp"), 
		NULL, &C4_GD_operation, CUI_EXT_FLAG_PKG))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".VMD"), _T(".wav"), 
//		NULL, &C4_VMD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
//			return -1;

//	if (callback->add_extension(callback->cui, _T(".MPX"), NULL, 
//		NULL, &C4_MPX_operation, CUI_EXT_FLAG_PKG))
//			return -1;

	return 0;
}
