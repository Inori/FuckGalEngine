#ifndef IO_REQUEST_H
#define IO_REQUEST_H

#ifdef IO_REQUEST_EXPORTS
#define IO_REQUEST_API __declspec(dllexport)
#else
#define IO_REQUEST_API __declspec(dllimport)
#endif 

/* initialize()的flags位段定义 */
#define IO_READWRITE	0x00000000UL
#define IO_READONLY		0x00000001UL
#define IO_CREATE		0x00000002UL

/* seek()的method参数定义 */
#define IO_SEEK_SET		0x00000000UL
#define IO_SEEK_CUR		0x00000001UL
#define IO_SEEK_END		0x00000002UL
#define IO_SEEK_METHOD	0x00000003UL

/* fio请求描述符 */
struct fio_request {
	HANDLE file;			/* [int]文件打开句柄 */
	HANDLE mapping;			/* [int]文件内存映射句柄 */
	void *ro_address;		/* [int]只读时的映射地址 */
//	int use_counts;			/* [int]文件打开计数 */
	u64 base_offset;		/* [int]文件的资源基址偏移（绝对值） */
	u64 base_length;		/* [int]文件的文件长度 */
	u64 current_offset;	/* [int]当前封包文件的资源位置偏移（相对值） */
	unsigned long flags;	/* [int]标志位段 */
};
	
/* bio请求描述符 */
struct bio_request {
	void *buffer;			/* （绝对值） */
	u64 base_offset;		/* [int]资源基址偏移（绝对值） */
	u64 base_length;		/* [int]文件长度 */
	u64 current_offset;	/* [int]当前文件的资源位置偏移（相对值） */
};

/* fio operations */
extern IO_REQUEST_API struct fio_request *fio_operation_initialize(const TCHAR *path, unsigned long flags);
extern IO_REQUEST_API struct fio_request *fio_operation_finish(struct fio_request *ior);
extern IO_REQUEST_API int fio_operation_seek(struct fio_request *ior, s32 offset_lo, s32 offset_hi, unsigned long flags);
extern IO_REQUEST_API int fio_operation_locate(struct fio_request *ior, u32 *location_lo, u32 *location_hi);
extern IO_REQUEST_API int fio_operation_read(struct fio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API void *fio_operation_readonly(struct fio_request *ior, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API int fio_operation_write(struct fio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API int fio_operation_length_of(struct fio_request *ior, u32 *len_lo, u32 *len_hi);
/* bio operations */
extern IO_REQUEST_API struct bio_request *bio_operation_initialize(void *buf, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API struct bio_request *bio_operation_finish(struct bio_request *ior);
extern IO_REQUEST_API int bio_operation_seek(struct bio_request *ior, s32 offset_lo, s32 offset_hi, unsigned long method);
extern IO_REQUEST_API int bio_operation_locate(struct bio_request *ior, u32 *location_lo, u32 *location_hi);
extern IO_REQUEST_API int bio_operation_read(struct bio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API void *bio_operation_readonly(struct bio_request *ior, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API int bio_operation_write(struct bio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi);
extern IO_REQUEST_API int bio_operation_length_of(struct bio_request *ior, u32 *len_lo, u32 *len_hi);

#endif	/* IO_REQUEST_H */
