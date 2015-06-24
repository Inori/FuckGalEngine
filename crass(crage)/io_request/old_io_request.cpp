#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <crass/io_request.h>
#include <stdio.h>

static DWORD virtualalloc_length_aligned;

/********** utilitu function **********/

static int MyCreatePath(const TCHAR *path)
{
	TCHAR *compt;
	TCHAR *tmp_path, *name, *p;
	DWORD len;

	if (PathIsRoot(path))
		return 0;

	len = _tcslen(path);	
	tmp_path = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
	if (!tmp_path)
		return -1;
	tmp_path[len] = 0;
	_tcsncpy(tmp_path, path, len);

	name = PathFindFileName(tmp_path);
	if (name)
		tmp_path[name - tmp_path] = 0;
	
	if (!*tmp_path || PathIsRoot(tmp_path)) {
		free(tmp_path);
		return 0;
	}

	p = tmp_path;
	while (p = _tcsstr(p, _T("/")))
		*p = _T('\\');

	p = tmp_path;
	while (compt = _tcsstr(p, _T("\\"))) {
		TCHAR bak = *(compt + 1);
		*(compt + 1) = 0;

		if (!PathIsRoot(tmp_path)) {
			if (!CreateDirectory(tmp_path, NULL)) {
				if (GetLastError() != ERROR_ALREADY_EXISTS) {
					free(tmp_path);
					return -1;
				}
			}
		}
		*(compt + 1) = bak;
		p = compt + 1;	/* may NULL */
	}
	free(tmp_path);
	
	return 0;
}

static HANDLE MyCreateFile(const TCHAR *name)
{
	HANDLE file;

	MyCreatePath(name);
	DeleteFile(name);

	file = CreateFile(name, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	return file;
}

/********** fio operations function **********/

IO_REQUEST_API struct fio_request *fio_operation_initialize(const TCHAR *path, unsigned long flags)
{
	struct fio_request *ior;
	HANDLE file, mapping;
	DWORD file_size_lo, file_size_hi;
	
	if (!path)
		return NULL;
	
	if (!(flags & IO_INIT_FLAG_CREATE)) {
		file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, /*FILE_ATTRIBUTE_READONLY*/FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
			return NULL;
	
		file_size_lo = GetFileSize(file, &file_size_hi);
		if ((file_size_lo == INVALID_FILE_SIZE) && (GetLastError() != NO_ERROR)) {
			CloseHandle(file);
			return NULL;
		}

		mapping = CreateFileMapping(file, NULL, PAGE_READONLY, file_size_hi, file_size_lo, NULL);
		if (!mapping) {
			CloseHandle(file);
			return NULL;
		}		
	} else {
		file = MyCreateFile(path);
		if (file == INVALID_HANDLE_VALUE)
			return NULL;

		file_size_lo = 0;
		file_size_hi = 0;
	}

	ior = (struct fio_request *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*ior));
	if (!ior) {
		if (mapping)
			CloseHandle(mapping);
		CloseHandle(file);
		return NULL;		
	}

	ior->file = file;
	ior->mapping = mapping;
	ior->use_counts = 1;
	ior->base_length = BUILD_QWORD(file_size_lo, file_size_hi);

	return ior;
}

IO_REQUEST_API struct fio_request *fio_operation_finish(struct fio_request *ior)
{
	if (!ior)
		return NULL;

	if (!--ior->use_counts) {
		if (ior->mapping) {
			CloseHandle(ior->mapping);
			ior->mapping = NULL;
		}
		if (ior->file != INVALID_HANDLE_VALUE) {
			CloseHandle(ior->file);		
			ior->file = INVALID_HANDLE_VALUE;
		}
		LocalFree(ior);
		ior = NULL;
	}
	
	return ior;
}

IO_REQUEST_API int fio_operation_seek(struct fio_request *ior, long offset, unsigned long method)
{
	if (!ior)
		return -1;

	switch (method & IO_SEEK_METHOD) {
	case IO_SEEK_SET:
		break;
	case IO_SEEK_CUR:
		offset = ior->current_offset + offset;
		break;
	case IO_SEEK_END:
		offset = ior->base_length + offset;
		break;
	default:
		return -1;
	}
	if ((offset < 0) || ((unsigned long)offset > ior->base_length))
		return -1;
	
	ior->current_offset = offset;

	return 0;
}

IO_REQUEST_API int fio_operation_locate(struct fio_request *ior, SIZE_T *location)
{
	if (!ior || !location)
		return -1;
	
	*location = ior->current_offset;

	return 0;
}

IO_REQUEST_API int fio_operation_read(struct fio_request *ior, void *buf, SIZE_T buf_len)
{
	SIZE_T offset_align, offset_extra;
	void *addr;
	
	if (!ior || !buf)
		return -1;

	if (ior->current_offset + buf_len > ior->base_length)
		return -1;
	
	if (!buf_len)
		return 0;

	offset_align = (ior->base_offset + ior->current_offset) & ~(virtualalloc_length_aligned - 1);
	offset_extra = (ior->base_offset + ior->current_offset) & (virtualalloc_length_aligned - 1);
	addr = MapViewOfFile(ior->mapping, FILE_MAP_READ, QWORD_HIGH(offset_align), QWORD_LOW(offset_align), (SIZE_T)(offset_extra + buf_len));
	if (!addr)
		return -1;
	
	CopyMemory(buf, (BYTE *)addr + offset_extra, buf_len);
	UnmapViewOfFile(addr);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API int fio_operation_write(struct fio_request *ior, void *buf, DWORD buf_len)
{
	HANDLE mapping;
	SIZE_T offset_align, offset_extra;
	void *addr;

	if (!ior || !buf)
		return -1;

	if (ior->current_offset + buf_len > ior->base_length)
		return -1;

	if (!buf_len)
		return 0;

	mapping = CreateFileMapping(ior->file, NULL, PAGE_READWRITE, QWORD_HIGH(buf_len), QWORD_LOW(buf_len), NULL);
	if (!mapping)
		return -1;

	offset_align = (ior->base_offset + ior->current_offset) & ~(virtualalloc_length_aligned - 1);
	offset_extra = (ior->base_offset + ior->current_offset) & (virtualalloc_length_aligned - 1);
	addr = MapViewOfFile(mapping, FILE_MAP_WRITE, QWORD_HIGH(offset_align), QWORD_LOW(offset_extra), (SIZE_T)(offset_extra + buf_len));
	if (!addr) {
		CloseHandle(mapping);
		return -1;
	}

	CopyMemory((BYTE *)addr + offset_extra, buf, (SIZE_T)buf_len);
	FlushViewOfFile((BYTE *)addr + offset_extra, (SIZE_T)buf_len);	
	UnmapViewOfFile(addr);
	CloseHandle(mapping);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API int fio_operation_length_of(struct fio_request *ior, SIZE_T *len)
{
	if (!ior || !len)
		return -1;

	*len = ior->base_length;

	return 0;	
}

/********** bio operations function **********/

IO_REQUEST_API struct bio_request *bio_operation_initialize(void *buf, SIZE_T buf_len)
{
	struct bio_request *ior;
	
	if (!buf || !buf_len)
		return NULL;
	
	ior = (struct bio_request *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*ior));
	if (!ior)
		return NULL;
	
	ior->buffer = buf;
	ior->base_length = buf_len;
	
	return ior;
}

IO_REQUEST_API struct bio_request *bio_operation_finish(struct bio_request *ior)
{
	if (!ior)
		return NULL;
	
	LocalFree(ior);
	return NULL;
}

IO_REQUEST_API int bio_operation_seek(struct bio_request *ior, long offset, unsigned long method)
{
	switch (method) {
	case IO_SEEK_SET:
		break;
	case IO_SEEK_CUR:
		offset = ior->current_offset + offset;
		break;
	case IO_SEEK_END:
		offset = ior->base_length + offset;
		break;
	default:
		return -1;
	}	
	if ((offset < 0) || ((unsigned long)offset > ior->base_length))
		return -1;
	
	ior->current_offset = offset;

	return 0;
}

IO_REQUEST_API int bio_operation_locate(struct bio_request *ior, SIZE_T *location)
{
	if (!ior || !location)
		return -1;
	
	*location = ior->current_offset;

	return 0;
}

IO_REQUEST_API int bio_operation_read(struct bio_request *ior, void *buf, SIZE_T buf_len)
{
	if (!ior || !buf)
		return -1;
	
	if (!buf_len)
		return 0;
	
	if (ior->current_offset + buf_len > ior->base_length)
		return -1;	
	
	CopyMemory(buf, (char *)ior->buffer + ior->current_offset, (SIZE_T)buf_len);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API int bio_operation_write(struct bio_request *ior, void *buf, SIZE_T buf_len)
{
	if (!ior || !buf)
		return -1;
	
	if (!buf_len)
		return 0;
	
	if (ior->current_offset + buf_len > ior->base_length)
		return -1;	
	
	CopyMemory((char *)ior->buffer + ior->current_offset, buf, (SIZE_T)buf_len);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API int bio_operation_length_of(struct bio_request *ior, SIZE_T *len)
{
	if (!ior || !len)
		return -1;

	*len = ior->base_length;

	return 0;	
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
 	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);
	virtualalloc_length_aligned = sys_info.dwAllocationGranularity;

	return TRUE;
}
