#ifndef PACKAGE_H
#define PACKAGE_H

#include <crass_types.h>
#include <crass/io_request.h>

/* io操作 */
struct package_io_operations {
	int (*open)(struct package *pkg, unsigned long flags);
	void (*close)(struct package *pkg);
	int (*seek)(struct package *pkg, s32 offset, unsigned long method);
	int (*locate)(struct package *pkg, u32 *offset);
	int (*read)(struct package *pkg, void *buf, u32 buf_len);
	int (*write)(struct package *pkg, void *buf, u32 buf_len);
	int (*readvec)(struct package *pkg, void *buf, u32 buf_len, s32 offset, unsigned long method);
	int (*writevec)(struct package *pkg, void *buf, u32 buf_len, s32 offset, unsigned long method);
	int (*length_of)(struct package *pkg, u32 *len);
	int (*create)(struct package *pkg);
	void *(*read_only)(struct package *pkg, u32 buf_len);
	void *(*readvec_only)(struct package *pkg, u32 buf_len, s32 offset, unsigned long method);
	int (*seek64)(struct package *pkg, s32 offset_lo, s32 offset_hi, unsigned long method);
	int (*locate64)(struct package *pkg, u32 *offset_lo, u32 *offset_hi);
	int (*read64)(struct package *pkg, void *buf, u32 buf_len_lo, u32 buf_len_hi);
	int (*write64)(struct package *pkg, void *buf, u32 buf_len_lo, u32 buf_len_hi);
	int (*readvec64)(struct package *pkg, void *buf, u32 buf_len_lo, u32 buf_len_hi, s32 offset_lo, s32 offset_hi, unsigned long method);
	int (*writevec64)(struct package *pkg, void *buf, u32 buf_len_lo, u32 buf_len_hi, s32 offset_lo, s32 offset_hi, unsigned long method);
	int (*length_of64)(struct package *pkg, u32 *len_lo, u32 *len_hi);
	void *(*read_only64)(struct package *pkg, u32 buf_len_lo, u32 buf_len_hi);
	void *(*readvec_only64)(struct package *pkg, u32 buf_len_lo, u32 buf_len_hi, s32 offset_lo, s32 offset_hi, unsigned long method);
};

struct package_context {
	u64 base_offset;		/* [int]封包文件的资源基址偏移（绝对值） */
	u64 base_length;		/* [int]封包文件的文件长度 */
	u64 current_offset;		/* [int]当前封包文件的资源位置偏移（相对值） */
	//struct package_context *parent;
};

/* 封包描述符 */
struct package {
	struct package *lst;					/* [in] 外部索引文件 */
	const TCHAR *name;						/* [in] 封包名称 */
	struct package_io_operations *pio;		/* [in]  */
	unsigned long context;					/* [out,in]封包私有信息 */
	union {
		struct fio_request *fior;			/* [int]fio请求描述符 */
		struct bio_request *bior;			/* [int]bio请求描述符 */	
	};
	/* 以下成员信息对cui/aui代码不透明 */
	const TCHAR *full_path;					/* [int]封包全路径 */
	const TCHAR *path;						/* [int]封包路径 */
	const TCHAR *extension;					/* [int]封包扩展名 */
	int use_counts;
	void (*release)(struct package *);	
	struct package *prev;					/* [int]前一个封包信息 */
	struct package *next;					/* [int]下一个封包信息 */
	struct package_resource *pkg_res;
	struct package *parent;
	struct package_context *pkg_context;
};

#define package_get_private(pkg)	((pkg)->context)
#define package_set_private(pkg, v)	((pkg)->context = (unsigned long)v)

/* 封包目录信息 */
struct package_directory {
	void *directory;						/* [out][out]索引段数据 */
	unsigned long directory_length;			/* [out][out]索引段数据长度 */
	unsigned long index_entries;			/* [in] [out]资源文件的个数 */
	unsigned long index_entry_length;		/* [out][out]索引项的长度 */
//	const TCHAR *lst_filename;				/* [in] 外部索引文件名 */
//	struct io_request *lst;					/* [in] 外部索引文件名io操作符 */
/* 标志位段定义 */
#define PKG_DIR_FLAG_VARLEN				0x00000001	/* 索引项信息变长 */
#define PKG_DIR_FLAG_SKIP0				0x00000002	/* 忽略长度为0的数据 */
#define PKG_DIR_FLAG_PRIVATE			0xff000000	/* cui私用位段 */
	unsigned long flags;					/* [out][out]标志位段 */
};

struct package_resource {
	unsigned int index_number;
	void *actual_index_entry;				/* [out][in] 指向当前索引项 */
	unsigned long actual_index_entry_length;/* [out][out]当前索引项的长度（仅变长索引段的情况） */
	char name[256];							/* [in] [out]资源文件名称 */
	long name_length;						/* [in] [out]资源文件名称长度，-1表示以NULL结尾 */
	void *raw_data;							/* [out][out]原始资源文件数据 */
	unsigned long raw_data_length;			/* [out][out]原始资源文件数据长度 */
	unsigned long offset;					/* [in] [out]偏移位置 */
	void *actual_data;						/* [out][out]实际的数据 */
	unsigned long actual_data_length;		/* [in] [out]实际的数据长度 */
	const TCHAR *replace_extension;
/* 标志位段定义 */
#define PKG_RES_FLAG_RAW		0x00000001	/* 只提取原始数据 */
#define PKG_RES_FLAG_LAST		0x00000002	/* 最后一项资源 */
#define PKG_RES_FLAG_REEXT		0x00010000	/* 重新定义资源文件扩展名 */
#define PKG_RES_FLAG_UNICODE	0x00020000	/* 资源文件名称是UNICODE编码 */
#define PKG_RES_FLAG_FIO		0x00040000	/* 以FIO资源类型提取(比如RES是目录型封包) */
#define PKG_RES_FLAG_APEXT		0x00080000	/* 附件资源文件扩展名 */
#define PKG_RES_FLAG_PRIVATE	0xff000000	/* cui私用位段掩码 */
	unsigned long flags;					/* [in][out]标志位段 */
};

#if 0
struct package_index_header {
	char cui_name[32];
	unsigned int entries;
	unsigned int index_length;
};

struct package_index_entry {
	char name[128];	
	LONG offset;
	DWORD length;
	DWORD actual_length;
};
#endif

#endif	/* PACKAGE_H */
