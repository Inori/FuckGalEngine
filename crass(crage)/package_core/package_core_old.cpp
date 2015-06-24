#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <crass/io_request.h>
#include <utility.h>
#include <package.h>
#include <package_core.h>
#include <stdio.h>

struct package_core {
	struct package root;
	unsigned int count;	
};

static struct package_core package_core;

static DWORD virtualalloc_length_aligned;

/*********** package list **************/

#define package_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
        	
#define package_list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)       	
        	
#define package_list_for_each_safe(pos, safe, head) \
	for (pos = (head)->next, safe = pos->next; pos != (head); \
        	pos = safe, safe = safe->next)   
        	       	
#define package_list_for_each_safe_reverse(pos, safe, head) \
	for (pos = (head)->prev, safe = pos->prev; pos != (head); \
        	pos = safe, safe = safe->prev)  

static inline void package_list_init(struct package *pkg)
{
	pkg->prev = pkg->next = pkg;
}

static inline void __package_list_del(struct package *prev, struct package *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void package_list_del(struct package *pkg)
{
	__package_list_del(pkg->prev, pkg->next);
}

static inline void package_list_del_init(struct package *pkg)
{
	__package_list_del(pkg->prev, pkg->next);
	package_list_init(pkg);
}

static inline void __package_list_add(struct package *prev, 
		struct package *_new, struct package *next)
{
	_new->prev = prev;
	_new->next = next;	
	prev->next = _new;
	next->prev = _new;
}

static inline void package_list_add_tail(struct package *_new, 
		struct package *head)
{
	__package_list_add(head->prev, _new, head);
}

static inline void package_list_add(struct package *_new, 
		struct package *head)
{
	__package_list_add(head, _new, head->next);
}

static inline void package_list_add_core(struct package *pkg)
{
	package_list_add_tail(pkg, &package_core.root);
	package_core.count++;
}

/*********** package core utility **************/

static void package_free(struct package *pkg)
{	
	if (!pkg)
		return;

	LocalFree(pkg);
}

static struct package *__package_get(struct package *pkg)
{
	if (!pkg)
		return NULL;

	pkg->use_counts++;
	return pkg;
}

static void __package_put(struct package *pkg)
{
	if (!pkg)
		return;

	if (--pkg->use_counts) {
		if (pkg->release)
			pkg->release(pkg);
	}
}

static struct package *package_alloc(const TCHAR *full_path)
{
	struct package *pkg;
	SIZE_T alloc_len;

	if (!full_path || !full_path[0])
		return NULL;

	alloc_len = sizeof(*pkg) + (_tcslen(full_path) + 1) * sizeof(TCHAR);
	pkg = (struct package *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, alloc_len);
	if (!pkg) {
		_stprintf(fmt_buf, _T("分配package时发生错误\n"));
		wcprintf_error(fmt_buf);		
		return NULL;		
	}
	pkg->release = package_free;
	pkg->full_path = (TCHAR *)(pkg + 1);
	_tcscpy((TCHAR *)pkg->full_path, full_path);	
	return __package_get(pkg);
}

static struct package *__package_create(const TCHAR *base_dir, const TCHAR *path)
{
	struct package *pkg;
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
		//base_len = _stprintf(full_path, _T("%s\\"), base_dir);
	} else
		base_len = 0;
	
	//if (path)
	//	_tcscat(full_path, path);

	if (PathAppend(full_path, path) == FALSE) {
		_stprintf(fmt_buf, _T("%s: 无效的package路径 (%s)\n"), path, base_dir);
		wcprintf_error(fmt_buf);
		return NULL;
	}
		
	pkg = package_alloc(full_path);
	if (!pkg)
		return NULL;
		
	pkg->path = pkg->full_path + base_len;
	pkg->name = PathFindFileName(pkg->full_path);
	if (!pkg->name) {
		_stprintf(fmt_buf, _T("%s: 无效的package路径\n"), pkg->full_path);
		wcprintf_error(fmt_buf);
		__package_put(pkg);
		return NULL;
	}
	pkg->extension = PathFindExtension(pkg->full_path);
	if (!pkg->extension || (pkg->extension[0] != _T('.')))
		pkg->extension = NULL;

	return pkg;
}

static int package_register(const TCHAR *base_dir, const TCHAR *path)
{
	struct package *pkg;

	pkg = package_fio_create(base_dir, path);
	if (!pkg)
		return -1;

	package_list_init(pkg);
	package_list_add_core(pkg);

	return 0;
}

static void package_unregister(struct package *pkg)
{
	if (!pkg)
		return;
		
	package_list_del(pkg);	
	__package_put(pkg);
}

static void package_unregister_each(void)
{
	struct package *root, *pkg, *safe;

	root = &package_core.root;
	package_list_for_each_safe_reverse(pkg, safe, root)
		package_unregister(pkg);
}

static int __package_search_file(const TCHAR *path)
{
	HANDLE find_file;
	WIN32_FIND_DATA find_data;
	TCHAR tmp_path[MAX_PATH];
	TCHAR *name;

	if (!path || !path[0])
		return -1;

	find_file = FindFirstFile(path, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		_stprintf(fmt_buf, _T("%s: 搜寻单个package时发生错误\n"), 
			path);
		wcprintf_error(fmt_buf);
		return -1;
	}
	FindClose(find_file);

	if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		_stprintf(fmt_buf, _T("%s: 该路径不是文件而是目录\n"), 
			path);
		wcprintf_error(fmt_buf);
		return 0;
	}

	if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
		_stprintf(fmt_buf, _T("%s: package文件长度为0\n"), path);
		wcprintf_error(fmt_buf);
		return -1;
	}

	_stprintf(tmp_path, path);
	name = PathFindFileName(tmp_path);
	*name = _T('');
	return package_register(tmp_path, find_data.cFileName) ? -1 : 1;
}

static int __package_search_directory(const TCHAR *base_dir, 
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
		_stprintf(fmt_buf, _T("%s: 无效的package路径 (%s)\n"), sub_dir, base_dir);
		wcprintf_error(fmt_buf);
		return -1;
	}

	//if (base_dir[0])
	//	_stprintf(path, _T("%s\\%s"), base_dir, sub_dir);
	//else
	//	_stprintf(path, sub_dir);
	
	_stprintf(search, _T("%s\\*"), full_path);
	find_file = FindFirstFile(search, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		_stprintf(fmt_buf, _T("%s: 搜寻第一个package时发生错误 (%d)\n"), full_path, GetLastError());
		wcprintf_error(fmt_buf);
		return -1;
	}
	
	count = 0;	
	if (_tcscmp(find_data.cFileName, _T(".")) 
		&& _tcscmp(find_data.cFileName, _T(".."))) {
			_stprintf(new_sub_path, sub_dir);
			if (PathAppend(new_sub_path, find_data.cFileName) == FALSE) {
				_stprintf(fmt_buf, _T("%s: 无效的package路径 (%s)\n"), find_data.cFileName, sub_dir);
				wcprintf_error(fmt_buf);
				FindClose(find_file);
				return -1;
			}

			//if (sub_dir[0])			
			//	_stprintf(new_sub_path, _T("%s\\%s"), sub_dir, 
			//		find_data.cFileName);
			//else
			//	_stprintf(new_sub_path, find_data.cFileName);				
					
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				err = __package_search_directory(base_dir, new_sub_path);
				if (err < 0) {
					FindClose(find_file);
					return err;
				}
				count += err;
			} else {
				if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
					_stprintf(fmt_buf, _T("%s: 该文件长度为0\n"), find_data.cFileName);
					wcprintf_error(fmt_buf);
					return -1;
				}

				err = package_register(base_dir, new_sub_path);
				if (err) {
					FindClose(find_file);
					return err;
				}
				count++;
			}
	}
//next:	
	err = 0;	
	while (FindNextFile(find_file, &find_data)) {
		if (_tcscmp(find_data.cFileName, _T(".")) 
				&& _tcscmp(find_data.cFileName, _T(".."))) {					
			_stprintf(new_sub_path, sub_dir);
			if (PathAppend(new_sub_path, find_data.cFileName) == FALSE) {
				_stprintf(fmt_buf, _T("%s: 无效的package路径 (%s)\n"), find_data.cFileName, sub_dir);
				wcprintf_error(fmt_buf);
				err = -1;
				break;
			}
			
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				err = __package_search_directory(base_dir, new_sub_path);
				if (err < 0)
					break;
				count += err;
			} else {
				if (!BUILD_QWORD(find_data.nFileSizeLow, find_data.nFileSizeHigh)) {
					_stprintf(fmt_buf, _T("%s: package文件长度为0\n"), find_data.cFileName);
					wcprintf_error(fmt_buf);
					return -1;
				}
				
				err = package_register(base_dir, new_sub_path);
				if (err)
					break;
				count++;
			}
		}		
	}
	FindClose(find_file);
	if (err < 0) {
		package_unregister_each();
		return -1;		
	}
		
	return count;
}

static int package_context_backup(struct package *pkg)
{
	struct package_context *pkg_cxt;

	if (!pkg)
		return -1;

	pkg_cxt = (struct package_context *)malloc(sizeof(*pkg_cxt));
	if (!pkg_cxt)
		return -1;

	pkg_cxt->base_length = pkg->fior->base_length;
	pkg_cxt->base_offset = pkg->fior->base_offset;
	pkg_cxt->current_offset = pkg->fior->current_offset;
	//pkg_cxt->parent = NULL;	
	//if (pkg->pkg_ctx)
	//	pkg_cxt->parent = pkg->pkg_ctx;
	pkg->pkg_context = pkg_cxt;

	return 0;
}

static void package_context_restore(struct package *pkg)
{
	struct package_context *pkg_cxt;

	if (!pkg || !pkg->pkg_context)
		return;

	pkg_cxt = pkg->pkg_context;
	pkg->fior->base_length = pkg_cxt->base_length;
	pkg->fior->base_offset = pkg_cxt->base_offset;
	pkg->fior->current_offset = pkg_cxt->current_offset;
	//pkg->pkg_ctx = pkg_cxt->parent;
	pkg->pkg_context = NULL;
	free(pkg_cxt);
}

/*********** package io operations **************/

static int package_fio_open(struct package *pkg)
{
	if (!pkg)
		return -1;

	if (pkg->parent) {
		struct package_resource *pkg_res;

		pkg->fior = pkg->parent->fior;
		if (package_context_backup(pkg))
			return -1;

		pkg_res = pkg->pkg_res;

//printf("%p before: %x %d %x\n", pkg, pkg->pkg_context->current_offset, pkg->pkg_context->base_length, pkg->pkg_context->base_offset);	

		if (fio_operation_seek(pkg->fior, pkg_res->offset, IO_SEEK_SET)) {
			package_context_restore(pkg);
			return -1;
		}

		pkg->fior->use_counts++;
		pkg->fior->current_offset = 0;
		pkg->fior->base_offset += pkg_res->offset;
		if (pkg_res->actual_data_length)
			pkg->fior->base_length = pkg_res->actual_data_length;
		else
			pkg->fior->base_length = pkg_res->raw_data_length;
	} else {
		pkg->fior = fio_operation_initialize(pkg->full_path, 0);
		if (!pkg->fior)
			return -1;
	}
//printf("%x %d %x\n", pkg->fior->current_offset, pkg->fior->base_length, pkg->fior->base_offset);
	return 0;
}

static void package_fio_close(struct package *pkg)
{
	if (!pkg)
		return;

	if (pkg->fior->use_counts > 1) {
		package_context_restore(pkg);
//printf("%p after: %x %d %x\n", pkg, pkg->fior->current_offset, pkg->fior->base_length, pkg->fior->base_offset);	
		fio_operation_seek(pkg->fior, pkg->fior->current_offset, IO_SEEK_SET);
	}
	fio_operation_finish(pkg->fior);
}

static int package_fio_seek(struct package *pkg, long offset, unsigned long method)
{
	if (!pkg)
		return -1;
	
	return fio_operation_seek(pkg->fior, offset, method);
}

static int package_fio_locate(struct package *pkg, SIZE_T *offset)
{
	if (!pkg || !offset)
		return -1;
	
	return fio_operation_locate(pkg->fior, offset);
}

static int package_fio_read(struct package *pkg, void *buf, SIZE_T buf_len)
{
	if (!pkg || !buf)
		return -1;
	
	return fio_operation_read(pkg->fior, buf, buf_len);
}

static int package_fio_write(struct package *pkg, void *buf, SIZE_T buf_len)
{
	if (!pkg || !buf)
		return -1;
	
	return fio_operation_write(pkg->fior, buf, buf_len);
}

static int package_fio_readvec(struct package *pkg, void *buf, SIZE_T buf_len, long offset, unsigned long method)
{
	if (!pkg || !buf)
		return -1;
	
	if (!buf_len)
		return 0;

	if (fio_operation_seek(pkg->fior,offset, method))
		return -1;
	
	return fio_operation_read(pkg->fior, buf, buf_len);
}

static int package_fio_writevec(struct package *pkg, void *buf, SIZE_T buf_len, long offset, unsigned long method)
{
	if (!pkg || !buf)
		return -1;
	
	if (!buf_len)
		return 0;

	if (fio_operation_seek(pkg->fior, offset, method))
		return -1;
	
	return fio_operation_write(pkg->fior, buf, buf_len);
}

static int package_fio_length_of(struct package *pkg, SIZE_T *len)
{
	if (!pkg || !len || !pkg->fior)
		return -1;
	
	*len = pkg->fior->base_length;
	
	return 0;
}

static int package_fio_create(struct package *pkg)
{
	unsigned long flags;
	
	if (!pkg)
		return -1;

	flags = PathFileExisted() ? 0 : IO_INIT_FLAG_CREATE;
	pkg->fior = fio_operation_initialize(pkg->full_path, flags);
	if (!pkg->fior)
		return -1;

	return 0;
}

static struct package_io_operations package_fio_operations = {
	package_fio_open,
	package_fio_close,
	package_fio_seek,
	package_fio_locate,
	package_fio_read,
	package_fio_write,
	package_fio_readvec,
	package_fio_writevec,
	package_fio_length_of,
	package_fio_create,
};

/*********** package bio operations **************/

static int package_bio_initialize(struct package *pkg)
{
	struct package_resource *pkg_res = pkg->pkg_res;
	
	if (!pkg || !pkg_res)
		return -1;

	if (pkg->bior) {
		int ret;
	
		ret = bio_operation_seek(pkg->bior, 0, IO_SEEK_SET);
		if (ret)
			return -1;
	} else {
		if (pkg_res->actual_data && pkg_res->actual_data_length)
			pkg->bior = bio_operation_initialize(pkg_res->actual_data, pkg_res->actual_data_length);
		else if (pkg_res->raw_data && pkg_res->raw_data_length)	
			pkg->bior = bio_operation_initialize(pkg_res->raw_data, pkg_res->raw_data_length);
		else
			return -1;
	}

	return 0;
}

static void package_bio_release(struct package *pkg)
{
	if (!pkg)
		return;
	
	pkg->bior = bio_operation_finish(pkg->bior);
}

static int package_bio_seek(struct package *pkg, long offset, unsigned long method)
{
	if (!pkg)
		return -1;
	
	return bio_operation_seek(pkg->bior, offset, method);
}

static int package_bio_locate(struct package *pkg, SIZE_T *offset)
{
	if (!pkg || !offset)
		return -1;
	
	return bio_operation_locate(pkg->bior, offset);
}

static int package_bio_read(struct package *pkg, void *buf, SIZE_T buf_len)
{
	if (!pkg || !buf)
		return -1;
	
	return bio_operation_read(pkg->bior, buf, buf_len);
}

static int package_bio_write(struct package *pkg, void *buf, SIZE_T buf_len)
{
	if (!pkg || !buf)
		return -1;
	
	return bio_operation_write(pkg->bior, buf, buf_len);
}

static int package_bio_readvec(struct package *pkg, void *buf, SIZE_T buf_len, long offset, unsigned long method)
{
	if (!pkg || !buf)
		return -1;
	
	if (!buf_len)
		return 0;

	if (bio_operation_seek(pkg->bior,offset, method))
		return -1;
	
	return bio_operation_read(pkg->bior, buf, buf_len);
}

static int package_bio_writevec(struct package *pkg, void *buf, SIZE_T buf_len, long offset, unsigned long method)
{
	if (!pkg || !buf)
		return -1;
	
	if (!buf_len)
		return 0;

	if (bio_operation_seek(pkg->bior, offset, method))
		return -1;
	
	return bio_operation_write(pkg->bior, buf, buf_len);
}

static int package_bio_length_of(struct package *pkg, SIZE_T *len)
{
	if (!pkg || !len || !pkg->bior)
		return -1;
	
	*len = pkg->bior->base_length;
	
	return 0;
}

static struct package_io_operations package_bio_operations = {
	package_bio_initialize,
	package_bio_release,
	package_bio_seek,
	package_bio_locate,
	package_bio_read,
	package_bio_write,
	package_bio_readvec,
	package_bio_writevec,
	package_bio_length_of
};
	
/*********** package core API **************/

PACKAGE_CORE_API void package_core_init(void)
{	
	memset(&package_core, 0, sizeof(package_core));	
	package_list_init(&package_core.root);
}

PACKAGE_CORE_API void package_core_uninstall(void)
{
	package_unregister_each();
}

PACKAGE_CORE_API struct package *package_walk_each(struct package *pkg)
{
	struct package *root = &package_core.root;
	
	if (!pkg)
		pkg = root;

	if (pkg->next == root)
		return NULL;	
	
	return pkg->next;
}

PACKAGE_CORE_API struct package *package_walk_each_safe(struct package **next)
{
	struct package *root = &package_core.root;
	struct package *pkg;

	pkg = *next;	
	if (!pkg) {
		pkg = root;
		if (pkg->next == root)
			return NULL;
		pkg = pkg->next;
	} else if (pkg == root)
		return NULL;	

	*next = pkg->next;

	return pkg;
}

PACKAGE_CORE_API struct package *package_fio_create(const TCHAR *base_dir, const TCHAR *path)
{
	struct package *pkg;
	
	pkg = __package_create(base_dir, path);
	if (!pkg)
		return NULL;
	
	pkg->pio = &package_fio_operations;

	return pkg;
}

PACKAGE_CORE_API struct package *package_bio_create(const TCHAR *base_dir, const TCHAR *path)
{
	struct package *pkg;
	
	pkg = __package_create(base_dir, path);
	if (!pkg)
		return NULL;
	
	pkg->pio = &package_bio_operations;

	return pkg;
}

PACKAGE_CORE_API void package_release(struct package *pkg)
{	
	if (!pkg)
		return;

	__package_put(pkg->lst);
	__package_put(pkg);
}

PACKAGE_CORE_API void package_uninstall(struct package *pkg)
{
	package_unregister(pkg);
}

PACKAGE_CORE_API int package_search_file(const TCHAR *path)
{
	return __package_search_file(path);
}

PACKAGE_CORE_API int package_search_directory(const TCHAR *base_dir, const TCHAR *sub_dir)
{
	return __package_search_directory(base_dir, sub_dir);
}
