#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <utility.h>
#include <stdio.h>
#include "tim2.h"

struct acui_information AFS_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".AFS .pak"),		/* package */
	_T("0.5.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-6-18 19:04"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "AFS"
	u32 index_entries;
} afs_header_t;

typedef struct {
	u32 offset;
	u32 length;
} afs_info_t;

typedef struct {
	s8 name[32];
	u16 year;
	u16 month;
	u16 day;
	u16 hour;
	u16 minute;
	u16 second;
	u32 junk;
} afs_entry_t;

//P2T:pal_index(%d) is not the multiple of 256(para is 0x1000)
typedef struct {
	u32 header_size;
	u32 aligned_size;
	u32 index_offset;
	u32 index_entries;	//P2T:It is a larger value than the texture contained(must > 0)
	u32 data_offset;
	u32 reserved[3];
} p2t_header_t;

typedef struct {
	u32 tim2_size;
} klz_header_t;
#pragma pack ()

typedef struct {
	s8 name[33];
	u32 offset;
	u32 length;
} my_afs_entry_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} tm2_entry_t;

#define SWAP16(v)	((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

static DWORD klz_uncompress(BYTE *uncompr, BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	WORD flag = 0;

	if (comprlen == 1)
		return 0;

	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			BYTE copy_bytes;
			u16 offset;

			offset = (compr[curbyte] << 8) | compr[curbyte + 1];
			curbyte += 2;
			copy_bytes = (BYTE)((offset & 0x1f) + 3);
			offset = (u16)act_uncomprlen - (offset >> 5) - 1;
			for (unsigned int k = 0; k < copy_bytes; k++) {
				if (act_uncomprlen >= 2048 || offset <= act_uncomprlen) {
					uncompr[act_uncomprlen] = uncompr[offset];
				} else
					uncompr[act_uncomprlen] = uncompr[0];
				offset++;
				act_uncomprlen++;
			}
		} else
			uncompr[act_uncomprlen++] = compr[curbyte++];
	}
//	printf("curbyte %x VS %x %x\n", curbyte, comprlen,act_uncomprlen);
	return act_uncomprlen;
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void sub_defiltering(BYTE *buf, DWORD width, DWORD depth)
{
	for (DWORD w = 1; w < width; w++) {
		buf += depth;
		for (DWORD p = 0; p < depth; p++)
			buf[p] += buf[p - depth];
	}
}

static void up_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above)
		return;

	for (DWORD w = 0; w < width; w++) {
		for (DWORD p = 0; p < depth; p++)
			buf[p] += above[p];
		buf += depth;
		above += depth;
	}
}

static void average_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above) {
		for (DWORD w = 1; w < width; w++) {
			buf += depth;
			for (DWORD p = 0; p < depth; p++)
				buf[p] += buf[p - depth] / 2;
		}
		return;
	}

	for (DWORD p = 0; p < depth; p++)
		buf[p] += above[p] / 2;

	for (DWORD w = 1; w < width; w++) {
		buf += depth;
		above += depth;
		for (DWORD p = 0; p < depth; p++)
			buf[p] += (buf[p - depth] + above[p]) / 2;
	}
}

static void paeth_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above) {
		for (DWORD w = 1; w < width; w++) {
			buf += depth;
			for (DWORD d = 0; d < depth; d++)
				buf[d] += buf[d - depth];
		}
		return;
	}
	
	for (DWORD p = 0; p < depth; p++)
		buf[p] += above[p];
		
	for (DWORD w = 1; w < width; w++) {
		BYTE a, b, c;	// a = left, b = above, c = upper left
		int p;
		int pa, pb, pc;
		
		buf += depth;
		above += depth;
		for (DWORD d = 0; d < depth; d++) {
			a = buf[d - depth];
			b = above[d];
			c = above[d - depth];
			p = a + b - c;

			pa = abs(p - a);
			pb = abs(p - b);
			pc = abs(p - c);

			if (pa <= pb && pa <= pc) 
				buf[d] += a;
			else if (pb <= pc)
				buf[d] += b;
			else 
				buf[d] += c;
		}
	}
}

static int pngfile_process(BYTE *png, DWORD png_size, BYTE **ret_png, DWORD *ret_png_size, 
						   DWORD width, DWORD height)
{
	DWORD idat_comprlen, zlib_comprlen, zlib_uncomprlen, curbyte, actual_data_len;
	BYTE *idat_compr, *zlib_compr, *zlib_uncompr, *actual_data;
	DWORD i, crc;
	
	/* 第一块IDAT恢复完毕，收集所有IDAT组成完整的zlib stream */
	curbyte = 0;
	zlib_compr = NULL;
	zlib_comprlen = 0;
	idat_compr = &png[0x21];
	idat_comprlen = png_size - 12 /* IEND */ - 0x21;
	do {
		BYTE *sub_idat_compr;
		DWORD sub_idat_comprlen;

		/* 获得IDAT长度 */
		sub_idat_comprlen = SWAP32(*(u32 *)(&idat_compr[curbyte]));
		curbyte += 4;

		/* 非IDAT */
		if (memcmp("IDAT", &idat_compr[curbyte], 4))
			break;

		sub_idat_compr = &idat_compr[curbyte];	/* 指向IDAT标识 */

		/* 检查crc */
		if (crc32(0L, sub_idat_compr, sub_idat_comprlen + 4) != SWAP32(*(u32 *)(&sub_idat_compr[4 + sub_idat_comprlen])))
			break;

		curbyte += 4;	
		sub_idat_compr += 4;
		if (!zlib_compr)
			zlib_compr = (BYTE *)malloc(sub_idat_comprlen);
		else
			zlib_compr = (BYTE *)realloc(zlib_compr, zlib_comprlen + sub_idat_comprlen);
		if (!zlib_compr)
			break;
		memcpy(zlib_compr + zlib_comprlen, sub_idat_compr, sub_idat_comprlen);
		curbyte += sub_idat_comprlen + 4;
		zlib_comprlen += sub_idat_comprlen;
	} while (curbyte < idat_comprlen);
	if (curbyte != idat_comprlen) {
		free(zlib_compr);
		printf("%x %x\n",curbyte, idat_comprlen);
		return -CUI_EUNCOMPR;
	}

	/* 解压zlib stream */
	for (i = 1; ; i++) {	
		zlib_uncomprlen = zlib_comprlen << i;
		zlib_uncompr = (BYTE *)malloc(zlib_uncomprlen);
		if (!zlib_uncompr)
			break;
			
		if (uncompress(zlib_uncompr, &zlib_uncomprlen, zlib_compr, zlib_comprlen) == Z_OK)
			break;
		free(zlib_uncompr);
	}
	free(zlib_compr);
	if (!zlib_uncompr)
		return -CUI_EUNCOMPR;

	for (DWORD h = 0; h < height; h++) {
		BYTE *line, *pre_line, *rgba;

		line = zlib_uncompr + h * (width * 4 + 1);
		pre_line = h ? line - (width * 4 + 1) : (BYTE *)-1;
		rgba = line + 1;

		switch (line[0]) {
		case 0:			
			break;
		case 1:
			sub_defiltering(&line[1], width, 4);
			break;
		case 2:
			up_defiltering(pre_line + 1, &line[1], width, 4);	
			break;
		case 3:
			average_defiltering(pre_line + 1, &line[1], width, 4);	
			break;
		case 4:
			paeth_defiltering(pre_line + 1, &line[1], width, 4);
			break;
		}
	}

	for (h = 0; h < height; h++) {
		BYTE *line, *rgba;

		line = zlib_uncompr + h * (width * 4 + 1);
		rgba = line + 1;

		/* 交换R和B分量 */
		for (DWORD w = 0; w < width; w++) {
			BYTE tmp;

			tmp = rgba[0];			
			rgba[0] = rgba[2];
			rgba[2] = tmp;
			rgba[3] = 255;
			rgba += 4;	
		}
		/* 最终保存的PNG不做filtering处理 */
		line[0] = 0;
	}

	/* 现在得到原始的RGB(A)数据, 然后重新压缩 */
	for (i = 1; ; i++) {
		zlib_comprlen = zlib_uncomprlen << i;			
		zlib_compr = (BYTE *)malloc(zlib_comprlen);
		if (!zlib_compr)
			break;
				
		if (compress(zlib_compr, &zlib_comprlen, zlib_uncompr, zlib_uncomprlen) == Z_OK)
			break;
		free(zlib_compr);
	}
	free(zlib_uncompr);
	if (!zlib_compr)
		return -CUI_EUNCOMPR;

	/* 创建最终经过“修正的”PNG图像缓冲区(非括号部分来自amp文件) */
	actual_data_len = 0x21 + (4 /* IDAT length */ + 4/* "IDAT" */ + zlib_comprlen /* IDAT数据 */ + 4 /* IDAT crc */) + 12;
	actual_data = (BYTE *)malloc(actual_data_len);
	if (!actual_data) {
		free(zlib_compr);
		return -CUI_EMEM;
	}

	memcpy(actual_data, png, 0x21);
	/* 写入IDAT长度 */
	*(u32 *)(&actual_data[33]) = SWAP32(zlib_comprlen);
	/* 写入IDAT标识 */
	actual_data[37] = 'I';
	actual_data[38] = 'D';
	actual_data[39] = 'A';
	actual_data[40] = 'T';
	/* 写入IDAT数据 */
	memcpy(actual_data + 41, zlib_compr, zlib_comprlen);
	free(zlib_compr);
	/* 计算IDAT crc */
	crc = crc32(0L, &actual_data[37], zlib_comprlen + 4);
	free(zlib_compr);
	/* 写入crc */
	*(u32 *)(&actual_data[actual_data_len - 16]) = SWAP32(crc);
	/* 写入IEND信息 */
	memcpy(actual_data + actual_data_len - 12, png + png_size - 12, 12);

	*ret_png = actual_data;
	*ret_png_size = actual_data_len;
	
	return 0;
}

static int fxt5_process(BYTE *fxt5, DWORD fxt5_size, BYTE *palette, DWORD palette_size, 
						DWORD width, DWORD height, BYTE **ret_bmp, DWORD *ret_bmp_size)
{
	DWORD act_uncomprlen, uncomprlen;
	BYTE *uncompr;

	uncomprlen = width * height;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, fxt5, fxt5_size) != Z_OK) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, 
			width, 0 - height, 8, ret_bmp, ret_bmp_size, my_malloc)) {
		free(uncompr);
		return -CUI_EMEM;
	}
	free(uncompr);
	return 0;
}

/********************* TM2 *********************/

static int AFS_tm2_match(struct package *pkg)
{
	TIM2_FILEHEADER TimHdr;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &TimHdr, sizeof(TimHdr))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (Tim2CheckFileHeaer(&TimHdr) != 1) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;
}

static int AFS_tm2_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	TIM2_FILEHEADER TimHdr;
	tm2_entry_t *index_buffer;
	DWORD index_buffer_length;
	DWORD offset;
	
	if (pkg->pio->readvec(pkg, &TimHdr, sizeof(TimHdr), 0, IO_SEEK_SET)) 
		return -CUI_EREAD;

	if (TimHdr.FormatId == 0x00)
		offset = sizeof(TIM2_FILEHEADER);
	else {
		if (pkg->pio->seek(pkg, 0x80, IO_SEEK_SET))
			return -CUI_ESEEK;	
		offset = 128;
	}
	
	index_buffer_length = sizeof(tm2_entry_t) * TimHdr.Pictures;
	index_buffer = (tm2_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	int ret = 0;
	for (DWORD i = 0; i < TimHdr.Pictures; i++) {
		TIM2_PICTUREHEADER PictHdr;

		if (pkg->pio->readvec(pkg, &PictHdr, sizeof(PictHdr), offset, IO_SEEK_SET))
			break;

		sprintf(index_buffer[i].name, "%08d", i);
		index_buffer[i].length = PictHdr.TotalSize + PictHdr.HeaderSize;
		index_buffer[i].offset = offset;
		offset += index_buffer[i].length;
	}
	if (i != TimHdr.Pictures) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = TimHdr.Pictures;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(tm2_entry_t);

	return 0;
}

static int AFS_tm2_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	tm2_entry_t *tm2_entry;

	tm2_entry = (tm2_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, tm2_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = tm2_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = tm2_entry->offset;

	return 0;
}

static int AFS_tm2_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	TIM2_PICTUREHEADER *PictHdr;
	BYTE *tim2, *MipMapPicture, *Image, *palette;
	int MipMapPictureSize;
	int Width, Height, Pitch, bpp;
	DWORD palette_size;

	tim2 = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!tim2)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, tim2, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(tim2);
		return -CUI_EREADVEC;
	}

	PictHdr = (TIM2_PICTUREHEADER *)tim2;
	Image = (BYTE *)Tim2GetImage(PictHdr, 0);
	if (!strncmp((char *)Image, "PNGFILE3", 8)) {
		int ret;

		if (!strncmp((char *)Image + 0x64, "FXT5", 4)) {
			ret = fxt5_process(Image + 0x7c, PictHdr->ImageSize - 0x7c, 
				Image + PictHdr->ImageSize, PictHdr->ClutSize, 
				*(u32 *)(Image + 8), *(u32 *)(Image + 12), 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);		
			pkg_res->replace_extension = _T(".bmp");
		} else {
			ret = pngfile_process(Image + 0x7c, PictHdr->ImageSize - 0x7c, 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length,
				*(u32 *)(Image + 8), *(u32 *)(Image + 12));
			pkg_res->replace_extension = _T(".png");
		}
		if (ret) {
			free(tim2);
			return -CUI_EMEM;
		}
		pkg_res->raw_data = tim2;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;	
		return 0;
	} else {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	MipMapPictureSize = Tim2GetMipMapPictureSize(PictHdr, 0, &Width, &Height);
	MipMapPicture = (BYTE *)malloc(MipMapPictureSize);
	if (!MipMapPicture) {
		free(tim2);
		return -CUI_EMATCH;
	}	
	memcpy(MipMapPicture, Image, MipMapPictureSize);

	switch (PictHdr->ImageType) {
	case TIM2_RGB16:	// 实际上是BGR555，最高位为1表示α为0xff，为0表示α为0
		bpp = 16;
		break;
	case TIM2_RGB24:
		bpp = 24;
		break;
	case TIM2_RGB32:
		bpp = 32;
		break;
	case TIM2_IDTEX4:
		bpp = 4;
		break;
	case TIM2_IDTEX8:
		bpp = 8;
		break;
	};
	Pitch = Width * bpp / 8;

	if (!Tim2GetClut(PictHdr)) {
		memset(MipMapPicture, 0, MipMapPictureSize);
		palette_size = 0;
		palette = NULL;
		for (int y = 0; y < Height; y++) {
			for (int x  =0; x < Width; x++) {
				BYTE r, g, b;
				int rgba;
				
				rgba = Tim2GetTexel(PictHdr, 0, x, y);
				switch(PictHdr->ImageType) {
				case TIM2_RGB16:
					r = ((rgba << 3) & 0xF8);
					g = ((rgba >> 2) & 0xF8);
					b = ((rgba >> 7) & 0xF8);
					MipMapPicture[y * Pitch + 0] = b | (g << 5);
					MipMapPicture[y * Pitch + 1] = (g >> 3) | (r << 3);
					break;
				case TIM2_RGB24:
					MipMapPicture[y * Pitch + 0] = (BYTE)(rgba >> 16);
					MipMapPicture[y * Pitch + 1] = (BYTE)(rgba >> 8);
					MipMapPicture[y * Pitch + 2] = (BYTE)rgba;
					break;
				case TIM2_RGB32:
					MipMapPicture[y * Pitch + 0] = (BYTE)(rgba >> 16);
					MipMapPicture[y * Pitch + 1] = (BYTE)(rgba >> 8);
					MipMapPicture[y * Pitch + 2] = (BYTE)rgba;
					MipMapPicture[y * Pitch + 3] = (BYTE)(rgba >> 24);
					break;
				}
			}
		}
	} else {
		Image = (BYTE *)Tim2GetImage(PictHdr, 0);
		memcpy(MipMapPicture, Image, MipMapPictureSize);
		palette_size = PictHdr->ClutColors * 4;
		palette = (BYTE *)malloc(palette_size);
		if (!palette) {
			free(MipMapPicture);
			free(tim2);
			return -CUI_EMEM;
		}
		
		for (int n = 0; n < PictHdr->ClutColors; n++) {
			int rbga = Tim2GetClutColor(PictHdr, 0, n);			
			palette[n * 4 + 0] = (BYTE)(rbga >> 8);
			palette[n * 4 + 1] = (BYTE)(rbga >> 16);
			palette[n * 4 + 2] = (BYTE)rbga;
			palette[n * 4 + 3] = (BYTE)(rbga >> 24);
		}
	}
//printf("Width %d, Height %d, bpp %d, ImageType %d, ClutType %d, ClutSize %d, ClutColors %d, MipMapPictureSize %d\n", 
//	Width, Height, bpp, PictHdr->ImageType, PictHdr->ClutType, PictHdr->ClutSize, PictHdr->ClutColors, MipMapPictureSize);	

    if (PictHdr->ImageType != TIM2_RGB16) {
	   	if (MyBuildBMPFile(MipMapPicture, MipMapPictureSize, palette, palette_size, Width, 0 - Height,
	    		bpp, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(MipMapPicture);
			free(tim2);
	    	return -CUI_EMEM;
		}
    } else {
	    if (MyBuildBMP16File(MipMapPicture, MipMapPictureSize, Width, 0 - Height, 
	    		(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, RGB555, NULL, my_malloc)) {
			free(MipMapPicture);
			free(tim2);
	    	return -CUI_EMEM;
	   	}
    }
   	free(MipMapPicture);
    pkg_res->raw_data = tim2;
	return 0;
}

static int AFS_tm2_save_resource(struct resource *res, 
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

static void AFS_tm2_release_resource(struct package *pkg, 
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

static void AFS_tm2_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation AFS_tm2_operation = {
	AFS_tm2_match,					/* match */
	AFS_tm2_extract_directory,		/* extract_directory */
	AFS_tm2_parse_resource_info,	/* parse_resource_info */
	AFS_tm2_extract_resource,		/* extract_resource */
	AFS_tm2_save_resource,			/* save_resource */
	AFS_tm2_release_resource,		/* release_resource */
	AFS_tm2_release					/* release */
};

/********************* afs *********************/

static int AFS_afs_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AFS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int AFS_afs_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	afs_info_t *info_index_buffer;
	afs_entry_t *index_buffer;
	my_afs_entry_t *my_index_buffer;
	DWORD my_index_buffer_length, info_index_buffer_length;
	u32 index_entries;
	afs_info_t dir_info;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	info_index_buffer_length = index_entries * sizeof(afs_info_t);
	info_index_buffer = (afs_info_t *)malloc(info_index_buffer_length);
	if (!info_index_buffer) 
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, info_index_buffer, info_index_buffer_length)) {
		free(info_index_buffer);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &dir_info, sizeof(dir_info))) {
		free(info_index_buffer);
		return -CUI_EREAD;
	}
	
	index_buffer = (afs_entry_t *)malloc(dir_info.length);
	if (!index_buffer) {
		free(info_index_buffer);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, index_buffer, dir_info.length, dir_info.offset, IO_SEEK_SET)) {
		free(index_buffer);
		free(info_index_buffer);
		return -CUI_EREAD;
	}
	
	my_index_buffer_length = index_entries * sizeof(my_afs_entry_t);
	my_index_buffer = (my_afs_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		free(info_index_buffer);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < index_entries; i++) {
		memset(my_index_buffer[i].name, 0, sizeof(my_index_buffer[i].name));
		strncpy(my_index_buffer[i].name, index_buffer[i].name, sizeof(index_buffer[i].name));
		my_index_buffer[i].length = info_index_buffer[i].length;
		my_index_buffer[i].offset = info_index_buffer[i].offset;
	}
	free(index_buffer);
	free(info_index_buffer);
	
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_afs_entry_t);

	return 0;
}

static int AFS_afs_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_afs_entry_t *my_afs_entry;

	my_afs_entry = (my_afs_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_afs_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_afs_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_afs_entry->offset;

	return 0;
}

static int AFS_afs_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = 0;	
	if (!strncmp((char *)compr, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (strstr(pkg_res->name, ".KLZ")) {
		BYTE *klz, *tim2;
		u32 tim2_size;

		klz = compr;
		tim2_size = SWAP32(*(u32 *)klz);
		klz += 4;
		tim2 = (BYTE *)malloc(tim2_size);
		if (!tim2)
			return -CUI_EMEM;

		uncompr = tim2;
		uncomprlen = tim2_size;
		for (DWORD i = 0; i < tim2_size;) {
			u16 blk_size;
			DWORD copy_bytes;

			blk_size = SWAP16(*(u16 *)klz);
			klz += 2;

			if (blk_size & 0x8000) {
				copy_bytes = blk_size & 0x7fff;
				memcpy(tim2, klz, copy_bytes);
				klz += copy_bytes;
			} else {
				copy_bytes = klz_uncompress(tim2, klz, blk_size);
				klz += blk_size;
			}
			i += copy_bytes;
			tim2 += copy_bytes;
		}	
		if (!strncmp((char *)uncompr, "TIM2", 4)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".TM2");
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int AFS_afs_save_resource(struct resource *res, 
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

static void AFS_afs_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void AFS_afs_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation AFS_afs_operation = {
	AFS_afs_match,					/* match */
	AFS_afs_extract_directory,		/* extract_directory */
	AFS_afs_parse_resource_info,	/* parse_resource_info */
	AFS_afs_extract_resource,		/* extract_resource */
	AFS_afs_save_resource,			/* save_resource */
	AFS_afs_release_resource,		/* release_resource */
	AFS_afs_release					/* release */
};

int CALLBACK AFS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".AFS"), NULL, 
		NULL, &AFS_afs_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &AFS_afs_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".TM2"), NULL, 
		NULL, &AFS_tm2_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;
	
	return 0;
}
