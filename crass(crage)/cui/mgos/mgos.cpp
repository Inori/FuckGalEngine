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

/*
O:\p\仮想＝現実 ハーレム_シミュレーター
J:\Program Files\MSH
Q:\saigaku_taiken\嵜柊妛媺懱尡斉
 */
/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information mgos_cui_information = {
	_T(""),		/* copyright */
	_T("μ-GameOperationSystem"),			/* system */
	_T(".det .atm .nme"),		/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 name_offset;
	u32 offset;
	u32 length;
	u32 unknown;		// crc ?
	u32 uncomprlen;
} atm_entry_t;

typedef struct {
	u8 maigc;		// 'F'
	u8 type;		// & 0x5f == 'D' or & 'E'
	u8 bpp;
	u8 pad;
	u16 width;
	u16 height;

} f_headery_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
	u32 unknown;
	u32 uncomprlen;
} det_entry_t;

static int level_delta_4675e8[40] = {
	-1, 0, 1, -1, -2, -2, -2, -1, 
	0, 1, 2, 2, -3, -3, -3, -3, 
	-2,	-1, 0, 1, 2, 3, 3, 3, 
	-4,	-4, -4, -4, -4, -3, -2, -1, 
	0, 1, 2, 3, 4, 4, 4, 4
};

static int level_delta_467688[40] = {
	0, -1, -1, -1, 0, -1, -2, -2,
	-2, -2, -2, -1, 0, -1, -2, -3,
	-3, -3, -3, -3, -3, -3, -2, -1,
	0, -1, -2, -3, -4, -4, -4, -4,
	-4, -4, -4, -4, -4, -3, -2, -1
};

static int dword_50BB08[163];
static int dword_50C1D0[163];
static char byte_50C460[256];
static char byte_50B708[512];
static char byte_50B908[512];

static void init_decode_table(void)
{
	for (char _j = 0; _j < 256; ++_j) {
		char k = 0;
		char j = _j;

		if (j) {
			while (j & 0x80) {
				j <<= 1;
				++k;
			}
			j <<= 1;
			if (!j)
				--k;
		}
		byte_50B708[2 * _j] = k + 1;
		byte_50B708[2 * _j + 1] = j;

		k = 0;
		j = _j;
		if (j) {
			while (j & 0x80) {
				j <<= 1;
				++k;
			}
			j <<= 1;
		}
		byte_50B908[2 * _j] = k;
		byte_50B908[2 * _j + 1] = j + (1 << k);
		k = 0;
		for (j = _j; (j & 0x7F); ++k)
			j <<= 1;
		byte_50C460[_j] = k;
	}
}

static void init_table_424850(void)
{
	dword_50BD98[0] = 0;
	dword_50BD98[1] = 1;
	dword_50BD98[2] = 2;

	DWORD k = 3;
	DWORD j = 44;
	for (DWORD i = 0; i < 40; ++i) {
		dword_50BD98[i + 3] = k + 120;
		dword_50BD98[i + 43] = k;
		dword_50BD98[i + 83] = j - 1;
		dword_50BD98[i + 123] = j;
		++k;
		j += 2;
	}

	for (DWORD i = 0; i < 163; ++i)
		dword_50BB08[dword_50BD98[i]] = i;
}

static int sub_4248F0(DWORD width, DWORD height, DWORD bpp, BYTE *in, BYTE *out)
{
	init_table_424850();
	memcpy(dword_50C1D0, dword_50BB08, sizeof(dword_50C1D0));

	for (DWORD i = 0; i < 40; ++i)
		dword_50C130[i] = level_delta_4675e8[i] + width * level_delta_467688[i];

	BYTE *out_end = out + 4 * width * height;
	DWORD flag = 0x80;
	in += sizeof(FE_header_t);
	while (out < out_end) {
		BYTE tmp = byte_50B708[2 * flag + 1];
		for (int i = byte_50B708[2 * flag]; !tmp; ) {
			BYTE idx = *in++;
			BYTE step = byte_50B908[2 * idx];
			tmp = byte_50B908[2 * idx + 1];
			++in;
			i += step;
		}

		int v14 = byte_50C460[tmp];
		int v13 = tmp + 256;
		if (i > v14) {
			int v16 = i - v14 - 1;
			int v15 = *in++ + ((v13 << v14) & 0xFFFFFF00);
			if (v16 >= 8) {
				int v17 = v16 >> 3;
				v16 += -8 * (v16 >> 3);
				do {
					v15 = *in++ + (v15 << 8);
					--v17;
				} while (v17);
			}
			v13 = 2 * v15 + 1;
			i = v16;
		}

		int v101 = v13 << i;
		int v19 = v101 >> 8;
		int v18 = (_BYTE)v101;
		if (v19 == 2) {
			v20 = dword_50C1D0[0];
		} else if (v19 == 3) {
			v20 = dword_50C1D0[1];
			dword_50C1D4[1] = dword_50C1D0[0];
			dword_50C1D0[0] = v20;
        } else {
			v102 = dword_50C130[37 + v19];
			v20 = dword_50C130[38 + v19];
			v103 = v19 - 2;
			dword_50C1D0[v103] = v102;
			dword_50C130[39 + v103] = dword_50C130[38 + v103];
			dword_50C130[38 + v103] = v20;
		}

		if (v20 == 2) {
			v21 = byte_50B708[2 * v18 + 1];
			for (j = byte_50B708[2 * v18]; !v21; ++in) {
				v104 = *in;
				j += byte_50B908[2 * v104];
				v21 = byte_50B908[2 * v104 + 1];
			}
			v24 = byte_50C460[v21];
			v23 = v21 + 256;
        if ( j - v24 > 0 )
        {
          v25 = j - v24 - 1;
          v26 = *data_in++ + ((v23 << v24) & 0xFFFFFF00);
          if ( (signed int)v25 >= 8 )
          {
            v27 = v25 >> 3;
            v25 += -8 * (v25 >> 3);
            do
            {
              v26 = *data_in++ + (v26 << 8);
              --v27;
            }
            while ( v27 );
          }
          v23 = 2 * v26 + 1;
          LOBYTE(j) = v25;
        }
        v105 = v23 << j;
        v28 = v105 >> 8;
        v105 = (_BYTE)v105;
        v30 = byte_50B709[2 * v105];
        for ( k = byte_50B708[2 * v105]; !v30; ++data_in )
        {
          v106 = *data_in;
          k += byte_50B908[2 * v106];
          v30 = byte_50B909[2 * v106];
        }
        v31 = byte_50C460[v30];
        v32 = v30 + 256;
        if ( k - v31 > 0 )
        {
          v33 = k - v31 - 1;
          v34 = *data_in++ + ((v32 << v31) & 0xFFFFFF00);
          if ( (signed int)v33 >= 8 )
          {
            v35 = v33 >> 3;
            v33 += -8 * (v33 >> 3);
            do
            {
              v34 = *data_in++ + (v34 << 8);
              --v35;
            }
            while ( v35 );
          }
          v32 = 2 * v34 + 1;
          LOBYTE(k) = v33;
        }
        v107 = v32 << k;
        v108 = *(_DWORD *)&in[4 * ((((v28 & 1) - 1) ^ ((v107 >> 8) - 2)) - width * ((v28 & 1) - 1 + v28 / 2))];
        flag = (_BYTE)v107;
        *(_DWORD *)in = v108;
        in += 4;
      }







	}


}


static int FE_uncompress(fe_header_t *fe_header)
{
	DWORD uncomprlen = fe_header->width * fe_header->height * 4 + 64;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr)
		return -CUI_EMEM;

	sub_4248F0(fe_header->width, fe_header->height, fe_header->bpp, fe_header, uncompr);

}

static DWORD det_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;

	while (curbyte < comprlen) {
		BYTE flag = compr[curbyte++];
		if (flag != 0xff)
			uncompr[act_uncomprlen++] = flag;
		else {
			flag = compr[curbyte++];
			if (flag != 0xff) {
				DWORD offset = (flag >> 2) + 1;
				DWORD count = (flag & 3) + 3;
				BYTE *src = uncompr + act_uncomprlen - offset;
				for (DWORD i = 0; i < count; ++i)
					uncompr[act_uncomprlen++] = *src++;
			} else
				uncompr[act_uncomprlen++] = 0xff;
		}
	}

	return act_uncomprlen;
}

/********************* det *********************/

/* 封包匹配回调函数 */
static int mgos_det_match(struct package *pkg)
{
	const char *nme_list = get_options("nme");
	if (!nme_list)
		return -CUI_EMATCH;

	TCHAR nme_name[MAX_PATH];

	acp2unicode(nme_list, -1, nme_name, MAX_PATH);
	HANDLE nme = MyOpenFile(nme_name);
	if (nme == INVALID_HANDLE_VALUE)
		return -CUI_EOPEN;

	DWORD size;			
	MyGetFileSize(nme, &size);
	BYTE *nme_index = new BYTE[size];
	if (!nme_index) {
		MyCloseFile(nme);
		return -CUI_EMEM;
	}

	if (MyReadFile(nme, nme_index, size)) {
		delete [] nme_index;
		MyCloseFile(nme);
		return -CUI_EREAD;
	}
	MyCloseFile(nme);

	if (pkg->pio->open(pkg, IO_READONLY)) {
		delete [] nme_index;
		return -CUI_EOPEN;
	}

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		delete [] nme_index;
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}
	package_set_private(pkg, nme_index);

	return 0;	
}

/* 封包索引目录提取函数 */
static int mgos_det_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 atm_index_length;
	
	if (pkg->pio->length_of(pkg->lst, &atm_index_length))
		return -CUI_ELEN;

	atm_index_length -= 4;	// 最后4字节无意义
	atm_entry_t *atm_index = (atm_entry_t *)new BYTE[atm_index_length];
	if (!atm_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, atm_index, atm_index_length)) {
		delete [] atm_index;
		return -CUI_ELEN;
	}

	pkg_dir->index_entries = atm_index_length / sizeof(atm_entry_t);

	BYTE *nme_index = (BYTE *)package_get_private(pkg);
	det_entry_t *det_index = new det_entry_t[pkg_dir->index_entries];
	if (!det_index) {
		delete [] atm_index;
		return -CUI_EMEM;
	}

	char *p = (char *)nme_index;
	for (DWORD i = 0; i < pkg_dir->index_entries; ++i) {
		char *p = (char *)nme_index + atm_index[i].name_offset;

		strcpy(det_index[i].name, p);
		det_index[i].offset = atm_index[i].offset;
		det_index[i].length = atm_index[i].length;
		det_index[i].unknown = atm_index[i].unknown;
		det_index[i].uncomprlen = atm_index[i].uncomprlen;
	}
	delete [] nme_index;
	delete [] atm_index;

	pkg_dir->directory = det_index;
	pkg_dir->directory_length = pkg_dir->index_entries * sizeof(det_entry_t);
	pkg_dir->index_entry_length = sizeof(det_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int mgos_det_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	det_entry_t *det_entry = (det_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, det_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = det_entry->length;
	pkg_res->actual_data_length = det_entry->uncomprlen;
	pkg_res->offset = det_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int mgos_det_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = det_uncompress(uncompr, pkg_res->actual_data_length, 
		compr, pkg_res->raw_data_length);
	if (act_uncomprlen != pkg_res->actual_data_length) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;		
	}

	if (uncompr[0] == 'F') {
		if ((uncompr[1] & 0x5f) == 'E') {
			sub_425AF0((BYTE *)_pic_buf, v8, a3, a4, v6, v7)
		} else if ((uncompr[1] & 0x5f)== 'D') {
			sub_4246F0(v8, a3, a4, v6, v7);
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int mgos_det_save_resource(struct resource *res, 
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
static void mgos_det_release_resource(struct package *pkg, 
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
static void mgos_det_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation mgos_det_operation = {
	mgos_det_match,					/* match */
	mgos_det_extract_directory,		/* extract_directory */
	mgos_det_parse_resource_info,	/* parse_resource_info */
	mgos_det_extract_resource,		/* extract_resource */
	mgos_det_save_resource,			/* save_resource */
	mgos_det_release_resource,		/* release_resource */
	mgos_det_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK mgos_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".det"), NULL, 
		NULL, &mgos_det_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_LST | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
