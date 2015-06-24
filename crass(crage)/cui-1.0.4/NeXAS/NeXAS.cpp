#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <zlib.h>
#include <stdio.h>

//NeXAS Ver.0.903

//TODO: Xross Scramble BF-Re-Action & Xross Scramble ゲーム大会用体験版
//bmp.arc: spr, ani, spm(可能用于描绘脸部阴影用的）

//TODO: virsul.arc里的hcg，都是那个样子....orz
// 推测不完整图里的象素值为ff的话，就使用完整图的象素值。
	
struct acui_information NeXAS_cui_information = {
	_T("GIGA"),				/* copyright */
	_T("NeXAS"),			/* system */
	_T(".pac"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-7-30 20:16"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "PAC" or "PACw"
	u32 entries;	// 76 bytes per entry
	u32 cmode;		// 0和4：无压缩？ ；> 4: default(may error)
} pac_header_t;

typedef struct {
	s8 name[64];
	u32 offset;	// from file begining
	u32 uncomprLen;
	u32 comprLen;
} pac_entry_t;

typedef struct {
	s8 magic[3];		/* "GR3" */
	u16 bits_count;
	u32 width;
	u32 height;
	u32 dib_len;
	u32 flag_bits;
} grp_header_t;
#pragma pack ()	


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int huffman_tree_create(struct bits *bits, u16 children[2][255], 
							   unsigned int *index, u16 *retval)
{
	unsigned int bitval;
	u16 child;

	if (bit_get_high(bits, &bitval))
		return -1;
	
	if (bitval) {
		unsigned int parent;

		parent = *index;
		*index = parent + 1;
					
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[0][parent - 256] = child;
		
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[1][parent - 256] = child;

		child = parent;
	} else {
		unsigned int byteval;
		
		if (bits_get_high(bits, 8, &byteval))
			return -1;
		
		child = byteval;			
	}
	*retval = child;
	
	return 0;
}

static int huffman_uncompress(unsigned char *uncompr, unsigned long *uncomprlen,
					 unsigned char *compr, unsigned long comprlen)
{
	struct bits bits;
	u16 children[2][255];
	unsigned int index = 256;
	unsigned long max_uncomprlen;
	unsigned long act_uncomprlen;
	unsigned int bitval;
	u16 retval;	
	
	bits_init(&bits, compr, comprlen);
	if (huffman_tree_create(&bits, children, &index, &retval))
		return -1;
	if (retval != 256)
		return -1;

	index = 0;	/* 从根结点开始遍历 */
	act_uncomprlen = 0;
	max_uncomprlen = *uncomprlen;
	while (!bit_get_high(&bits, &bitval)) {
		if (bitval)
			retval = children[1][index];
		else
			retval = children[0][index];
	
		if (retval >= 256)
			index = retval - 256;
		else {
			if (act_uncomprlen >= max_uncomprlen)
				break;
			uncompr[act_uncomprlen++] = (u8)retval;
			index = 0;	/* 返回到根结点 */
		}
	}
	*uncomprlen = act_uncomprlen;

	return 0;
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, 0, nCurWindowByte);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;

				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= uncomprlen)
					goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;
				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

					if (act_uncomprlen >= uncomprlen)
						goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	return act_uncomprlen;
}

static DWORD grp3_lzss_decompress(unsigned char *uncompr, unsigned long uncomprlen, 
							unsigned char *compr, unsigned int comprlen,
							unsigned char *flag_bitmap, unsigned int flag_bits)
{
	unsigned long act_uncomprLen = 0;
	unsigned int curbyte = 0;

	memset(uncompr, 0, uncomprlen);
	for (unsigned int i = 0; i < flag_bits; i++) {
		BYTE flag = flag_bitmap[i >> 3];

		if (!getbit_le(flag, i & 7)) {
			if (curbyte >= comprlen)
				goto out;
			if (act_uncomprLen >= uncomprlen)
				goto out;
			uncompr[act_uncomprLen++] = compr[curbyte++];
		} else {
			unsigned int copy_bytes, win_offset;

			if (curbyte >= comprlen)
				goto out;
			copy_bytes = compr[curbyte++];

			if (curbyte >= comprlen)
				goto out;
			win_offset = compr[curbyte++] << 8;

			win_offset = (copy_bytes | win_offset) >> 3;
			copy_bytes = copy_bytes & 0x7;
			copy_bytes++;

			for (unsigned int k = 0; k < copy_bytes; k++) {	
				if (act_uncomprLen + k >= uncomprlen)
					goto out;
				
				uncompr[act_uncomprLen + k] = uncompr[act_uncomprLen - win_offset - 1 + k];
			}
			act_uncomprLen += copy_bytes;
		}
	}
out:
	return act_uncomprLen;
}

/********************* pac *********************/

static int NeXAS_pac_match(struct package *pkg)
{
	pac_header_t *pac_header;
	pac_entry_t pac_entry;	/* BaldrX */

	pac_header = (pac_header_t *)malloc(sizeof(pac_header_t));
	if (!pac_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(pac_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, pac_header, sizeof(pac_header_t))) {
		pkg->pio->close(pkg);
		free(pac_header);
		return -CUI_EREAD;
	}

	if (strncmp(pac_header->magic, "PAC", 3)) {
		pkg->pio->close(pkg);
		free(pac_header);
		return -CUI_EMATCH;	
	}

	if (!pac_header->entries) {
		pkg->pio->close(pkg);
		free(pac_header);
		return -CUI_EMATCH;	
	}

	/* 针对Xross Scramble的Savor的Update.pac：*/
	if (pkg->pio->read(pkg, &pac_entry, sizeof(pac_entry)))
		goto out;

	/* 和baldrX做互斥 */
	if (pac_entry.offset == sizeof(pac_header_t) + sizeof(pac_entry_t) * pac_header->entries) {
		pkg->pio->close(pkg);
		free(pac_header);
		return -CUI_EMATCH;
	}
out:
	package_set_private(pkg, pac_header);

	return 0;	
}

static int NeXAS_pac_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	pac_header_t *pac_header;
	DWORD comprLen, act_uncomprLen;
	DWORD index_buffer_length;
	BYTE *compr, *uncompr;
	DWORD i;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	if (pkg->pio->seek(pkg, -4, IO_SEEK_END))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &comprLen, 4))
		return -CUI_EREAD;

	compr = (BYTE *)malloc(comprLen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->seek(pkg, -(LONG)(4 + comprLen), IO_SEEK_CUR)) {
		free(compr);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, compr, comprLen)) {
		free(compr);
		return -CUI_EREAD;
	}

	for (i = 0; i < comprLen; i++)
		compr[i] = ~compr[i];

	index_buffer_length = pac_header->entries * sizeof(pac_entry_t);
	uncompr = (BYTE *)malloc(index_buffer_length);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	act_uncomprLen = index_buffer_length;
	if (huffman_uncompress(uncompr, &act_uncomprLen, compr, comprLen)) {
		free(uncompr);
		free(compr);
		return -CUI_EUNCOMPR;
	}
	free(compr);

	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entries = pac_header->entries;
	pkg_dir->index_entry_length= sizeof(pac_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;	

	return 0;
}

static int NeXAS_pac_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	pac_entry_t *pac_entry;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pac_entry->comprLen;
	if (pac_header->cmode && pac_header->cmode != 4)
		pkg_res->actual_data_length = pac_entry->uncomprLen;
	else
		pkg_res->actual_data_length = 0;
	pkg_res->offset = pac_entry->offset;

	return 0;
}

static int NeXAS_pac_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	pac_entry_t *pac_entry;
	DWORD comprlen, uncomprlen, act_uncomprLen = 0;
	BYTE *compr, *uncompr = NULL;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	comprlen = pac_entry->comprLen;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pac_entry->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	uncomprlen = pkg_res->actual_data_length;
	if (uncomprlen) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			return -CUI_EMEM;
		}
		act_uncomprLen = uncomprlen;
	} else {
		uncomprlen = 0;
		uncompr = NULL;
	}

	if (pac_header->cmode == 3) {
		if (!uncompr || !uncomprlen) {
			if (uncompr)
				free(uncompr);
			return -CUI_EPARA;
		}
		/* 有的数据过短，导致返回-3。因此根本不理会返回值 */
		uncompress(uncompr, &act_uncomprLen, compr, comprlen);
	} else if (pac_header->cmode == 1) {
		if (!uncompr || !uncomprlen) {
			if (uncompr)
				free(uncompr);
			return -CUI_EPARA;
		}

		act_uncomprLen = lzss_decompress(uncompr, uncomprlen, 
			compr, comprlen);
		if (act_uncomprLen != uncomprlen) {
			crass_printf(_T("%s: decompress error occured (%d VS %d)\n"), 
				pkg->name, uncomprlen, act_uncomprLen);
			free(uncompr);			
			return -CUI_EUNCOMPR;
		}
	} else if (pac_header->cmode == 2) {
		DWORD act_uncomprLen = uncomprlen;
		if (huffman_uncompress(uncompr, &act_uncomprLen, compr, comprlen)) {
			free(uncompr);			
			return -CUI_EUNCOMPR;			
		}
		uncomprlen = act_uncomprLen;
	} else if (pac_header->cmode && (pac_header->cmode != 4)) {
		crass_printf(_T("%s: unsupported mode %d\n"), 
			pkg->name, pac_header->cmode);
		if (uncompr)
			free(uncompr);
		return -CUI_EMATCH;
	}
	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprLen;

	return 0;
}

static NeXAS_pac_save_resource(struct resource *res, 
							   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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

static void NeXAS_pac_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data && pkg_res->raw_data_length)
		pkg_res->raw_data = NULL;
}

static void NeXAS_pac_release(struct package *pkg, 
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

static cui_ext_operation NeXAS_pac_operation = {
	NeXAS_pac_match,				/* match */
	NeXAS_pac_extract_directory,	/* extract_directory */
	NeXAS_pac_parse_resource_info,	/* parse_resource_info */
	NeXAS_pac_extract_resource,		/* extract_resource */
	NeXAS_pac_save_resource,		/* save_resource */
	NeXAS_pac_release_resource,		/* release_resource */
	NeXAS_pac_release				/* release */
};

/********************** grp ************************/

static int NeXAS_grp_match(struct package *pkg)
{
	grp_header_t grp_header;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &grp_header, sizeof(grp_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(grp_header.magic, "GR3", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int NeXAS_grp_extract_resource(struct package *pkg,
							 		  struct package_resource *pkg_res)
{
	grp_header_t *grp_header;
	unsigned int flag_bitmap_len;
	BYTE *flag_bitmap, *compr, *uncompr;
	DWORD comprLen;
	u32 grp_len;

	if (pkg->pio->length_of(pkg, &grp_len))
		return -CUI_ELEN;

	grp_header = (grp_header_t *)malloc(grp_len);
	if (!grp_header)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, grp_header, grp_len, 0, IO_SEEK_SET)) {
		free(grp_header);
		return -CUI_EREADVEC;
	}
	
	flag_bitmap_len = (grp_header->flag_bits + 7) >> 3;
	flag_bitmap = (BYTE *)(grp_header + 1);
	comprLen = *(DWORD *)&flag_bitmap[flag_bitmap_len];
	compr = &flag_bitmap[flag_bitmap_len] + 4;
	
	uncompr = (BYTE *)malloc(grp_header->dib_len); 
	if (!uncompr) {
		free(grp_header);
    	return -CUI_EMEM;
	}
	
	DWORD act_uncomprlen = grp3_lzss_decompress(uncompr, grp_header->dib_len, 
		compr, comprLen, flag_bitmap, grp_header->flag_bits);
	if (act_uncomprlen != grp_header->dib_len) {
		crass_printf(_T("%s: grp decompress error occured (%d VS %d)\n"),
			pkg->name, act_uncomprlen, grp_header->dib_len);
		free(uncompr);
		free(grp_header);
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, grp_header->width, 
			0 - grp_header->height, grp_header->bits_count, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(grp_header);
		return -CUI_EUNCOMPR;
	}
	free(uncompr);
	free(grp_header);

	return 0;
}

static int NeXAS_grp_save_resource(struct resource *res, 
								   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void NeXAS_grp_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void NeXAS_grp_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation NeXAS_grp_operation = {
	NeXAS_grp_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* extract_index */
	NeXAS_grp_extract_resource,	/* extract_resource */
	NeXAS_grp_save_resource,	/* save_resource */
	NeXAS_grp_release_resource,	/* release_resource */
	NeXAS_grp_release
};

/********************** zlib ************************/

static int NeXAS_zlib_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "\x78\x9c", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int NeXAS_zlib_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	u32 comprlen;

	if (pkg->pio->length_of(pkg, &comprlen))
		return -CUI_ELEN;

	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 0, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	DWORD uncomprlen = comprlen * 2;
	BYTE *uncompr = NULL;
	int ret = 0;
	while (1) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			ret = -CUI_EMEM;
			break;
		}

		if (uncompress(uncompr, &uncomprlen, compr, comprlen) == Z_OK)
			break;
		free(uncompr);
		uncomprlen *= 2;
	}
	free(compr);
	if (ret) {
		free(uncompr);
		return ret;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	
	return 0;
}

static cui_ext_operation NeXAS_zlib_operation = {
	NeXAS_zlib_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	NeXAS_zlib_extract_resource,	/* extract_resource */
	NeXAS_grp_save_resource,		/* save_resource */
	NeXAS_grp_release_resource,		/* release_resource */
	NeXAS_grp_release				/* release */
};

int CALLBACK NeXAS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &NeXAS_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".grp"), _T(".bmp"), 
		NULL, &NeXAS_grp_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".spm"), _T("._spm"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".map"), _T("._map"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T("._dat"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mek"), _T("._mek"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".grp"), _T("._grp"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".waz"), _T("._waz"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T("._bin"), 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), NULL, 
		NULL, &NeXAS_zlib_operation, CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}

