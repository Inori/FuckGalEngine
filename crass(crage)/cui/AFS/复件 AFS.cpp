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
	_T(".AFS"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-10-22 18:07"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
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
				if (act_uncomprlen >= 2048 || offset <= act_uncomprlen)
					uncompr[act_uncomprlen++] = uncompr[offset++];
				else
					uncompr[act_uncomprlen++] = uncompr[0];
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

/********************* TM2 *********************/

static int AFS_tm2_match(struct package *pkg)
{
	TIM2_FILEHEADER TimHdr;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &TimHdr, sizeof(TimHdr))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)TimHdr.FileId, "TIM2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if ((TimHdr.FormatVersion != 4) || (TimHdr.FormatId != 0 && TimHdr.FormatId != 1)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
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

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->readvec(pkg, &TimHdr, sizeof(TimHdr), 0, IO_SEEK_SET)) 
		return -CUI_EREAD;
	
	offset = sizeof(TimHdr);
	
	if (TimHdr.FormatId == 1) {
		if (pkg->pio->seek(pkg, 112, IO_SEEK_CUR))
			return -CUI_ESEEK;	
		offset += 112;
	}
	
	index_buffer_length = sizeof(tm2_entry_t) * TimHdr.Pictures;
	index_buffer = (tm2_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	int ret = 0;
	for (DWORD i = 0; i < TimHdr.Pictures; i++) {
		TIM2_PICTUREHEADER PictHdr;

		if (pkg->pio->read(pkg, &PictHdr, sizeof(PictHdr))) {
			ret = -CUI_EREAD;
			break;
		}

		if (pkg->pio->seek(pkg, PictHdr.ImageSize + PictHdr.ClutSize, IO_SEEK_CUR)) {
			ret = -CUI_ESEEK;
			break;
		}

		sprintf(index_buffer[i].name, "%08d.bmp", i);
		index_buffer[i].length = sizeof(TIM2_PICTUREHEADER) + PictHdr.ImageSize + PictHdr.ClutSize;
		index_buffer[i].offset = offset;
		offset += index_buffer[i].length;
	}
	if (i != TimHdr.Pictures) {
		free(index_buffer);
		return ret;
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

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

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
	TIM2_PICTUREHEADER PictHdr;
	BYTE *Clut, *Image;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;
	
	if (pkg->pio->readvec(pkg, &PictHdr, sizeof(PictHdr), pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	if (PictHdr.MipMapTextures) {
		int bpl, bpp;

		if (PictHdr.ClutSize) {
			DWORD csize;

			Clut = (BYTE *)malloc(PictHdr.ClutSize);
			if (!Clut)
				return -CUI_EMEM;
			
			if (pkg->pio->readvec(pkg, Clut, PictHdr.ClutSize, pkg_res->offset + sizeof(PictHdr) 
					+ PictHdr.ImageSize, IO_SEEK_SET)) {
				free(Clut);
				return -CUI_EREADVEC;
			}
			
			for (DWORD p = 0; p < PictHdr.ClutColors; p++) {
				if (PictHdr.ImageType == 5) {
					BYTE r;
					r = Clut[p * 4 + 0];
					Clut[p * 4 + 0] = Clut[p * 4 + 2];	// swap R and B
					Clut[p * 4 + 2] = r;
				}
			}
			
			csize = (PictHdr.ClutType & 0x1F) + 1;

			if (PictHdr.ImageType > 3) {	// 4bpp or 8bpp
			//	if (csize == 2)
			//		Convert555ToRGB(ACT);
	        }
		} else
			Clut = NULL;

		switch (PictHdr.ImageType) {
		case 1:	// 16bpp(实际上是BGR555，最高位为1表示α为0xff，为0表示α为0)
			bpp = 16;
			break;
		case 2: // 24bpp
			bpp = 24;
			break;
		case 3: // 32bpp
			bpp = 32;
			break;
		case 4: // 4bpp
			bpp = 4;
			break;
		case 5:	// 8bpp
			bpp = 8;
			break;
		};
		bpl = PictHdr.ImageWidth * bpp / 8;
printf("ImageType %d, ClutType %d, ClutSize %d ClutColors %d bpp %d\n", PictHdr.ImageType, PictHdr.ImageType, 
	   PictHdr.ClutSize, PictHdr.ClutColors, bpp);		
		Image = (BYTE *)malloc(PictHdr.ImageSize);
		if (!Image) {
			free(Clut);
			return -CUI_EMEM;
		}
		
		if (pkg->pio->readvec(pkg, Image, PictHdr.ImageSize, pkg_res->offset + sizeof(PictHdr), IO_SEEK_SET)) {
			free(Image);
			free(Clut);
			return -CUI_EREADVEC;
		}

		if (PictHdr.ImageType == 4) {	// 4bpp
			for (int y = 0; y < PictHdr.ImageHeight; y++) {
				//Swap4Bit(Ansector->ScanLine[y],bpl);
				;
	           // Possible RGB conversation for 16,24 and 32 bit
			}
	    }

	    if (PictHdr.ImageType != 1) {
	    	if (MyBuildBMPFile(Image, PictHdr.ImageSize, Clut, PictHdr.ClutSize, PictHdr.ImageWidth, 0 - PictHdr.ImageHeight,
	    			bpp, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(Image);
				free(Clut);
	    		return -CUI_EMEM;
	    	}
	    } else {
	    	if (MyBuildBMP16File(Image, PictHdr.ImageSize, PictHdr.ImageWidth, 0 - PictHdr.ImageHeight, 
	    			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, RGB555, NULL, my_malloc)) {
				free(Image);
				free(Clut);
	    		return -CUI_EMEM;
	    	}
	    }
		free(Image);
		free(Clut);
	}
	return 0;
}

static int AFS_tm2_save_resource(struct resource *res, 
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

static void AFS_tm2_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

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
	if (!pkg || !pkg_dir)
		return;

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

	if (!pkg)
		return -CUI_EPARA;

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

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

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

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

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
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = 0;	
	if (!memcmp(compr, "OggS", 4)) {
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
	//		printf("%x VS %x {%x VS %x}\n",i, tim2_size, blk_size , copy_bytes);
		}	
		
		if (!strncmp((char *)tim2, "TIM2", 4)) {
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

static void AFS_afs_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void AFS_afs_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

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
	
	if (callback->add_extension(callback->cui, _T(".TM2"), NULL, 
		NULL, &AFS_tm2_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;
	
	return 0;
}
