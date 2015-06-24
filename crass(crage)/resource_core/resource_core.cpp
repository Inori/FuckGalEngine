#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <crass/io_request.h>
#include <utility.h>
#include <resource.h>
#include <resource_core.h>
#include <stdio.h>

struct resource_core {
	struct resource root;
	unsigned int count;	
};

static struct resource_core resource_core;

static DWORD virtualalloc_length_aligned;

/*********** resource list **************/

#define package_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
        	
#define resource_list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)       	
        	
#define resource_list_for_each_safe(pos, safe, head) \
	for (pos = (head)->next, safe = pos->next; pos != (head); \
        	pos = safe, safe = safe->next)   
        	       	
#define resource_list_for_each_safe_reverse(pos, safe, head) \
	for (pos = (head)->prev, safe = pos->prev; pos != (head); \
        	pos = safe, safe = safe->prev)  

static inline void resource_list_init(struct resource *res)
{
	res->prev = res->next = res;
}

static inline void __resource_list_del(struct resource *prev, struct resource *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void resource_list_del(struct resource *res)
{
	__resource_list_del(res->prev, res->next);
}

static inline void resource_list_del_init(struct resource *res)
{
	__resource_list_del(res->prev, res->next);
	resource_list_init(res);
}

static inline void __resource_list_add(struct resource *prev, 
		struct resource *_new, struct resource *next)
{
	_new->prev = prev;
	_new->next = next;	
	prev->next = _new;
	next->prev = _new;
}

static inline void resource_list_add_tail(struct resource *_new, 
		struct resource *head)
{
	__resource_list_add(head->prev, _new, head);
}

static inline void resource_list_add(struct resource *_new, 
		struct resource *head)
{
	__resource_list_add(head, _new, head->next);
}

static inline void resource_list_add_core(struct resource *res)
{
	resource_list_add_tail(res, &resource_core.root);
	resource_core.count++;
}

/*********** resource core utility **************/

static void resource_free(struct resource *res)
{	
	if (!res)
		return;

	GlobalFree(res);
}

static struct resource *__resource_get(struct resource *res)
{
	if (!res)
		return NULL;

	res->use_counts++;
	return res;
}

static void __resource_put(struct resource *res)
{
	if (!res)
		return;

	if (--res->use_counts) {
		if (res->release)
			res->release(res);
	}
}

static struct resource *resource_alloc(const TCHAR *full_path)
{
	struct resource *res;
	SIZE_T alloc_len;

	if (!full_path || !full_path[0])
		return NULL;

	alloc_len = sizeof(*res) + sizeof(struct package_resource) + (_tcslen(full_path) + 1) * sizeof(TCHAR);		
	res = (struct resource *)GlobalAlloc(GPTR, alloc_len);
	if (!res) {
		crass_printf(_T("分配resource时发生错误\n"));
		return NULL;		
	}
	res->release = resource_free;
	res->pkg_res = (struct package_resource *)(res + 1);
	res->full_path = (TCHAR *)(res->pkg_res + 1);
	_tcscpy((TCHAR *)res->full_path, full_path);
	return __resource_get(res);
}

static int resource_register(const TCHAR *base_dir, const TCHAR *path)
{
	struct resource *res;
	
	res = resource_create(base_dir, path);
	if (!res)
		return -1;

	resource_list_init(res);
	resource_list_add_core(res);

	return 0;
}

static void resource_unregister(struct resource *res)
{
	if (!res)
		return;
		
	resource_list_del(res);	
	__resource_put(res);
}

static void resource_unregister_each(void)
{
	struct resource *root, *res, *safe;

	root = &resource_core.root;
	resource_list_for_each_safe_reverse(res, safe, root)
		resource_unregister(res);
}

static int __resource_search_file(const TCHAR *path)
{
	HANDLE find_file;
	WIN32_FIND_DATA find_data;
	TCHAR tmp_path[MAX_PATH];
	TCHAR *name;

	if (!path || !path[0])
		return -1;

	find_file = FindFirstFile(path, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		crass_printf(_T("%s: 搜寻单个resource时发生错误\n"), path);
		return -1;
	}
	FindClose(find_file);

	if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		crass_printf(_T("%s: 该路径不是文件而是目录\n"), path);
		return 0;
	}

	if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
		crass_printf(_T("%s: resource文件长度为0\n"), path);
		return -1;
	}

	_stprintf(tmp_path, path);
	name = PathFindFileName(tmp_path);
	*name = _T('');
	return resource_register(tmp_path, find_data.cFileName) ? -1 : 1;
}

static int __resource_search_directory(const TCHAR *base_dir, 
								   const TCHAR *sub_dir)
{
	HANDLE find_file;
	WIN32_FIND_DATA find_data;
	unsigned int count;
	TCHAR search[MAX_PATH];
	TCHAR full_path[MAX_PATH];
	TCHAR new_sub_path[MAX_PATH];
	int err;

	if (!base_dir || !sub_dir)
		return -1;

	if (!base_dir[0] && !sub_dir[0])
		return -1;

	_stprintf(full_path, base_dir);
	if (PathAppend(full_path, sub_dir) == FALSE) {
		crass_printf(_T("%s: 无效的resource路径 (%s)\n"), sub_dir, base_dir);
		return -1;
	}


	_stprintf(search, _T("%s\\*"), full_path);
	find_file = FindFirstFile(search, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		crass_printf(_T("%s: 搜寻第一个package时发生错误 (%d)\n"), full_path, GetLastError());
		return -1;
	}
	
	count = 0;	
	if (_tcscmp(find_data.cFileName, _T(".")) 
		&& _tcscmp(find_data.cFileName, _T(".."))) {
			_stprintf(new_sub_path, sub_dir);
			if (PathAppend(new_sub_path, find_data.cFileName) == FALSE) {
				crass_printf(_T("%s: 无效的package路径 (%s)\n"), find_data.cFileName, sub_dir);
				FindClose(find_file);
				return -1;
			}
				
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				err = __resource_search_directory(base_dir, new_sub_path);
				if (err < 0) {
					FindClose(find_file);
					return err;
				}
				count += err;
			} else {
				if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
					crass_printf(_T("%s: 该文件长度为0\n"), find_data.cFileName);
					return -1;
				}
				err = resource_register(base_dir, new_sub_path);
				if (err) {
					FindClose(find_file);
					return err;
				}
				count++;
			}
	}
	
	err = 0;	
	while (FindNextFile(find_file, &find_data)) {
		if (_tcscmp(find_data.cFileName, _T(".")) 
				&& _tcscmp(find_data.cFileName, _T(".."))) {					
			_stprintf(new_sub_path, sub_dir);
			if (PathAppend(new_sub_path, find_data.cFileName) == FALSE) {
				crass_printf(_T("%s: 无效的package路径 (%s)\n"), find_data.cFileName, sub_dir);
				err = -1;
				break;
			}
				
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				err = __resource_search_directory(base_dir, new_sub_path);
				if (err < 0)
					break;
				count += err;
			} else {				
				if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
					crass_printf(_T("%s: resource文件长度为0\n"), find_data.cFileName);
					return -1;
				}
				
				err = resource_register(base_dir, new_sub_path);
				if (err)
					break;
				count++;
			}
		}		
	}
	FindClose(find_file);
	if (err < 0) {
		resource_unregister_each();
		return -1;		
	}
		
	return count;
}

/*********** resource io operations **************/

static int resource_fio_create(struct resource *res)
{
	if (!res || !res->pkg_res)
		return -1;

	if (res->ior) {
		int ret;

		ret = fio_operation_seek(res->ior, 0, 0, IO_SEEK_SET);
		if (ret)
			return -1;

		res->ior->current_offset = 0;
	} else {
		res->ior = fio_operation_initialize(res->full_path, IO_CREATE | IO_READWRITE);
		if (!res->ior)
			return -1;
		if (res->pkg_res->actual_data_length)
			res->ior->base_length = res->pkg_res->actual_data_length;
		else
			res->ior->base_length = res->pkg_res->raw_data_length;
	}	

	return 0;
}

static int resource_fio_open(struct resource *res)
{
	if (!res)
		return -1;

	res->ior = fio_operation_initialize(res->full_path, 0);
	if (!res->ior)
		return -1;

	return 0;
}

static void resource_fio_close(struct resource *res)
{
	if (!res)
		return;
	
	res->ior = fio_operation_finish(res->ior);
}

static int __resource_fio_seek(struct resource *res, s32 offset_lo, s32 offset_hi, unsigned long method)
{
	if (!res)
		return -1;
	
	return fio_operation_seek(res->ior, offset_lo, offset_hi, method);
}

static int resource_fio_seek(struct resource *res, s32 offset, unsigned long method)
{
	return __resource_fio_seek(res, offset, 0, method);
}

static int resource_fio_seek64(struct resource *res, s32 offset_lo, s32 offset_hi, unsigned long method)
{
	return __resource_fio_seek(res, offset_lo, offset_hi, method);
}

static int __resource_fio_locate(struct resource *res, u32 *offset_lo, u32 *offset_hi)
{
	if (!res || !offset_lo)
		return -1;
	
	return fio_operation_locate(res->ior, offset_lo, offset_hi);
}

static int resource_fio_locate(struct resource *res, u32 *offset)
{
	return __resource_fio_locate(res, offset, NULL);
}

static int resource_fio_locate64(struct resource *res, u32 *offset_lo, u32 *offset_hi)
{
	return __resource_fio_locate(res, offset_lo, offset_hi);
}

static int __resource_fio_read(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	if (!res || !buf)
		return -1;
	
	return fio_operation_read(res->ior, buf, buf_len_lo, buf_len_hi);
}

static int resource_fio_read(struct resource *res, void *buf, u32 buf_len)
{
	return __resource_fio_read(res, buf, buf_len, 0);
}

static int resource_fio_read64(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	return __resource_fio_read(res, buf, buf_len_lo, buf_len_hi);
}

static int __resource_fio_write(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	if (!res || !buf)
		return -1;

	return fio_operation_write(res->ior, buf, buf_len_lo, buf_len_hi);
}

static int resource_fio_write(struct resource *res, void *buf, u32 buf_len)
{
	return __resource_fio_write(res, buf, buf_len, 0);
}

static int resource_fio_write64(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	return __resource_fio_write(res, buf, buf_len_lo, buf_len_hi);
}

static int __resource_fio_readvec(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi, 
								s32 offset_lo, s32 offset_hi, unsigned long method)
{

	if (!res || !buf)
		return -1;


	if (fio_operation_seek(res->ior, offset_lo, offset_hi, method))
		return -1;
	
	return fio_operation_read(res->ior, buf, buf_len_lo, buf_len_hi);
}

static int resource_fio_readvec(struct resource *res, void *buf, u32 buf_len, 
								s32 offset, unsigned long method)
{
	return __resource_fio_readvec(res, buf, buf_len, 0, offset, 0, method);
}

static int resource_fio_readvec64(struct resource *res, void *buf, u32 buf_len_lo, u32 buf_len_hi, 
								s32 offset_lo, s32 offset_hi, unsigned long method)
{
	return __resource_fio_readvec(res, buf, buf_len_lo, buf_len_hi, offset_lo, offset_hi, method);
}

static int __resource_fio_writevec(struct resource *res, void *buf, 
								 u32 buf_len_lo, u32 buf_len_hi,
								 s32 offset_lo, s32 offset_hi,
								 unsigned long method)
{
	if (!res || !buf)
		return -1;

	if (fio_operation_seek(res->ior, offset_lo, offset_hi, method))
		return -1;
	
	return fio_operation_write(res->ior, buf, buf_len_lo, buf_len_hi);
}

static int resource_fio_writevec(struct resource *res, void *buf, 
								 u32 buf_len, s32 offset,
								 unsigned long method)
{
	return __resource_fio_writevec(res, buf, buf_len, 0, offset, 0, method);
}

static int resource_fio_writevec64(struct resource *res, void *buf, 
								 u32 buf_len_lo, u32 buf_len_hi,
								 s32 offset_lo, s32 offset_hi,
								 unsigned long method)
{
	return __resource_fio_writevec(res, buf, buf_len_lo, buf_len_hi, offset_lo, offset_hi, method);
}

static int __resource_fio_length_of(struct resource *res, 
								  u32 *len_lo, u32 *len_hi)
{
	if (!res || !len_lo || !res->ior)
		return -1;
	
	*len_lo = QWORD_LOW(res->ior->base_length);
	if (len_hi)
		*len_hi = QWORD_HIGH(res->ior->base_length);
	
	return 0;
}

static int resource_fio_length_of(struct resource *res, 
								  u32 *len)
{
	return __resource_fio_length_of(res, len, 0);
}

static int resource_fio_length_of64(struct resource *res, 
								  u32 *len_lo, u32 *len_hi)
{
	return __resource_fio_length_of(res, len_lo, len_hi);
}

static struct resource_io_operations resource_fio_operations = {
	resource_fio_create,
	resource_fio_close,
	resource_fio_seek,
	resource_fio_locate,
	resource_fio_read,
	resource_fio_write,
	resource_fio_readvec,
	resource_fio_writevec,
	resource_fio_length_of,
	resource_fio_open,
	NULL,
	NULL,
	resource_fio_seek64,
	resource_fio_locate64,
	resource_fio_read64,
	resource_fio_write64,
	resource_fio_readvec64,
	resource_fio_writevec64,
	resource_fio_length_of64,
};

#if 0
/*********** resource file mapping io operations **************/

static int resource_file_mapping_io_create(struct io_request *ior, unsigned long flags)
{
	int ret;
	
	if (!ior)
		return -1;

	ior->fio->handle = MyCreateFile(ior->fio->path);
	ret = ior->fio->handle == INVALID_HANDLE_VALUE ? -1 : 0;
	if (!ret)
		ior->fio->use_counts = 1;

	return ret;
}

static void resource_file_mapping_io_close(struct io_request *ior)
{
	if (!ior)
		return;

	if (!--ior->fio->use_counts) {	
		if (ior->fio->handle != INVALID_HANDLE_VALUE) {
			MyCloseFile(ior->fio->handle);		
			ior->fio->handle = INVALID_HANDLE_VALUE;
		}
	}
}

static int resource_file_mapping_io_seek(struct io_request *ior, 
								u64 offset, unsigned long posision)
{
	s32 offset_hi = 0;
	
	if (!ior)
		return -1;

	switch (posision) {
	case IO_SEEK_SET:
		posision = FILE_BEGIN;
		offset += ior->base_offset;
		break;
	case IO_SEEK_CUR:
		posision = FILE_CURRENT;
		break;
	case IO_SEEK_END:
		posision = FILE_BEGIN;
		offset = (long)ior->base_offset + ior->base_length + offset;
		break;
	default:
		return -1;
	}

	return MySetFilePosition(ior->fio->handle, offset, NULL, posision);
}

static int resource_file_mapping_io_get_position(struct io_request *ior, u64 *offset)
{
	int err;
	
	if (!ior || !offset)
		return -1;

	err = MyGetFilePosition(ior->fio->handle, offset, NULL);
	if (err)
		return err;

	*offset -= ior->base_offset;

	return 0;
}

static int resource_file_mapping_io_read(struct io_request *ior, void *buf, 
								u64 buf_len)
{
	if (!ior || !buf || !buf_len)
		return -1;
	
	return MyReadFile(ior->fio->handle, buf, buf_len);
}

static int resource_file_mapping_io_readvec(struct io_request *ior, void *buf, 
								   u64 buf_len, u64 offset, 
								   unsigned long method)
{
	if (!ior || !buf || !buf_len)
		return -1;

	if (resource_file_io_seek(ior, offset, method))
		return -1;
	
	return MyReadFile(ior->fio->handle, buf, buf_len);
}

int resource_file_mapping_io_write(struct io_request *ior, void *buf, 
						  u64 buf_len)
{
	HANDLE mapping;

	if (!ior || !buf || !buf_len)
		return -1;

	mapping = CreateFileMapping(ior->fio->handle, NULL, PAGE_READWRITE, 0, buf_len, NULL);
	if (!mapping)
		return -1;

	void *addr = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, buf_len);
	if (!addr) {
		CloseHandle(mapping);
		return -1;
	}
	memcpy(addr, buf, buf_len);
	FlushViewOfFile(addr, buf_len);
	UnmapViewOfFile(addr);
	CloseHandle(mapping);

	return 0;
}

static int resource_file_mapping_io_writevec(struct io_request *ior, void *buf, 
								   u64 buf_len, u64 offset, 
								   unsigned long method)
{
	if (!ior || !buf || !buf_len)
		return -1;

	if (resource_file_io_seek(ior, offset, method))
		return -1;
	
	return MyWriteFile(ior->fio->handle, buf, buf_len);
}

struct io_request_operations resource_file_mapping_io = {
	resource_file_mapping_io_create,
	resource_file_mapping_io_close,
	resource_file_mapping_io_seek,
	resource_file_mapping_io_get_position,
	resource_file_mapping_io_read,
	resource_file_mapping_io_readvec,
	resource_file_mapping_io_write,
	resource_file_mapping_io_writevec
};
#endif
	
/*********** resource core API **************/

RESOURCE_CORE_API void resource_core_init(void)
{	
	memset(&resource_core, 0, sizeof(resource_core));	
	resource_list_init(&resource_core.root);
}

RESOURCE_CORE_API void resource_core_uninstall(void)
{
	resource_unregister_each();
}

RESOURCE_CORE_API struct resource *resource_walk_each(struct resource *res)
{
	struct resource *root = &resource_core.root;
	
	if (!res)
		res = root;

	if (res->next == root)
		return NULL;	
	
	return res->next;
}

RESOURCE_CORE_API struct resource *resource_walk_each_safe(struct resource **next)
{
	struct resource *root = &resource_core.root;
	struct resource *res;

	res = *next;	
	if (!res) {
		res = root;
		if (res->next == root)
			return NULL;
		res = res->next;
	} else if (res == root)
		return NULL;	

	*next = res->next;

	return res;
}

RESOURCE_CORE_API struct resource *resource_create(const TCHAR *base_dir, const TCHAR *path)
{
	struct resource *res;
	TCHAR full_path[MAX_PATH];
	int base_len;
		
	if (!base_dir || !path)
		return NULL;

	if (!base_dir[0] && !path[0])
		return NULL;
		
	_stprintf(full_path, base_dir);
	PathRemoveBackslash(full_path);
	if (base_dir[0]) {
		base_len = _tcslen(full_path);
		if (!PathIsRoot(full_path))
			base_len++;
	} else
		base_len = 0;
		
	if (PathAppend(full_path, path) == FALSE) {
		crass_printf( _T("%s: 无效的package路径 (%s)\n"), path, base_dir);
		return NULL;
	}

	res = resource_alloc(full_path);
	if (!res)
		return NULL;
		
	res->path = res->full_path + base_len;
	res->name = PathFindFileName(res->full_path);
	if (!res->name) {
		crass_printf(_T("%s: 无效的resource路径\n"), res->full_path);
		__resource_put(res);
		return NULL;
	}
	res->extension = PathFindExtension(res->full_path);
	if (!res->extension || (res->extension[0] != _T('.')))
		res->extension = NULL;

	res->rio = &resource_fio_operations;

	return res;
}

RESOURCE_CORE_API void resource_release(struct resource *res)
{	
	if (!res)
		return;

	__resource_put(res);
}

RESOURCE_CORE_API void resource_uninstall(struct resource *res)
{
	resource_unregister(res);
}

RESOURCE_CORE_API int resource_search_file(const TCHAR *path)
{
	return __resource_search_file(path);
}

RESOURCE_CORE_API int resource_search_directory(const TCHAR *base_dir, const TCHAR *sub_dir)
{
	return __resource_search_directory(base_dir, sub_dir);
}
