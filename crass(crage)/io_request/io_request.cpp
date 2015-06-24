#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <crass/io_request.h>
#include <stdio.h>
#include <utility.h>

static DWORD virtualalloc_length_aligned;

/********** utility function **********/

static int __MyCreatePath(const TCHAR *path)
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

static HANDLE __MyCreateFile(const TCHAR *name)
{
	HANDLE file;

	__MyCreatePath(name);
	DeleteFile(name);

	file = CreateFile(name, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, NULL);
	return file;
}

/********** fio operations function **********/

IO_REQUEST_API struct fio_request *fio_operation_initialize(const TCHAR *path, unsigned long flags)
{
	struct fio_request *ior;
	HANDLE file, mapping = NULL;
	DWORD file_size_lo, file_size_hi;

	if (!path)
		return NULL;

	if (!(flags & IO_CREATE)) {
		if (flags & IO_READONLY)
			file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, 
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		else
			file = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE)
			return NULL;

		file_size_lo = GetFileSize(file, &file_size_hi);
		if ((file_size_lo == INVALID_FILE_SIZE) && (GetLastError() != NO_ERROR)) {
			CloseHandle(file);
			return NULL;
		}

		if (flags & IO_READONLY) {
			mapping = CreateFileMapping(file, NULL, PAGE_READONLY, file_size_hi, file_size_lo, NULL);
			if (!mapping) {
				CloseHandle(file);
				return NULL;
			}		
		}	
	} else {
		file = __MyCreateFile(path);
		if (file == INVALID_HANDLE_VALUE)
			return NULL;

		file_size_lo = 0;
		file_size_hi = 0;
	}
	ior = (struct fio_request *)malloc(sizeof(*ior));
	if (!ior) {
		if (mapping)
			CloseHandle(mapping);
		CloseHandle(file);
		return NULL;		
	}
	memset(ior, 0, sizeof(*ior));
	ior->file = file;
	ior->mapping = mapping;
//	ior->use_counts = 1;
	ior->base_length = BUILD_QWORD(file_size_lo, file_size_hi);
	ior->flags = flags;	

	return ior;
}

IO_REQUEST_API struct fio_request *fio_operation_finish(struct fio_request *ior)
{
	if (!ior)
		return NULL;

//	if (!--ior->use_counts) {
		if (ior->ro_address) {
			UnmapViewOfFile(ior->ro_address);
			ior->ro_address = NULL;
		}
		if (ior->mapping) {
			CloseHandle(ior->mapping);
			ior->mapping = NULL;
		}
		if (ior->file != INVALID_HANDLE_VALUE) {
			CloseHandle(ior->file);		
			ior->file = INVALID_HANDLE_VALUE;
		}
		free(ior);
		ior = NULL;
//	}
	
	return ior;
}

IO_REQUEST_API int fio_operation_seek(struct fio_request *ior, s32 offset_lo, s32 offset_hi, 
									  unsigned long method)
{
	s64 offset = (s64)BUILD_QWORD(offset_lo, offset_hi);

	if (!ior)
		return -1;
#if 0
	method &= IO_SEEK_METHOD;
	int override = 0;
	if (method == IO_SEEK_SET) {


	} else if () {

	} else if () {

	}
	if (override && !(ior->flags & IO_READONLY))
		ior->base_length = offset;

	if (method & IO_SEEK_METHOD) {
	case IO_SEEK_SET:
		break;
	case IO_SEEK_CUR:
		offset += (s64)ior->current_offset;
		break;
	case IO_SEEK_END:
		offset += (s64)ior->base_length;
		break;
	default:
		return -1;
	}
	if (offset < 0 || (u64)offset > ior->base_length) {
		if (ior->flags & IO_READONLY)
			return -1;
		else 
			ior->base_length = offset;
	}
#else
	switch (method & IO_SEEK_METHOD) {
	case IO_SEEK_SET:
		break;
	case IO_SEEK_CUR:
		offset += (s64)ior->current_offset;
		break;
	case IO_SEEK_END:
		offset += (s64)ior->base_length;
		break;
	default:
		return -1;
	}
	if (offset < 0 || (u64)offset > ior->base_length) {
		if (ior->flags & IO_READONLY)
			return -1;
		else 
			ior->base_length = offset;
	}
#endif	
	ior->current_offset = offset;

	return 0;
}

IO_REQUEST_API int fio_operation_locate(struct fio_request *ior, u32 *location_lo, u32 *location_hi)
{
	if (!ior || !location_lo)
		return -1;
	
	*location_lo = QWORD_LOW(ior->current_offset);
	if (location_hi)
		*location_hi = QWORD_HIGH(ior->current_offset);

	return 0;
}

IO_REQUEST_API int fio_operation_read(struct fio_request *ior, void *buf, 
									  u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);
	u64 offset_align, offset_extra;
	void *addr;
	
	if (!ior || !buf)
		return -1;

	if (ior->current_offset + buf_len > ior->base_length)
		return -1;
	
	if (!buf_len)
		return 0;

	offset_align = (ior->base_offset + ior->current_offset) & ~(virtualalloc_length_aligned - 1);
	offset_extra = (ior->base_offset + ior->current_offset) & (virtualalloc_length_aligned - 1);

	if (ior->mapping)	/* readonly */
		addr = MapViewOfFile(ior->mapping, FILE_MAP_READ, QWORD_HIGH(offset_align), QWORD_LOW(offset_align), (SIZE_T)(offset_extra + buf_len));
	else {
		HANDLE mapping = CreateFileMapping(ior->file, NULL, PAGE_READONLY, 0, 0, NULL);
			//QWORD_HIGH(mapping_size), QWORD_LOW(mapping_size), NULL);	// why ???			
		if (!mapping)
			return -1;
		addr = MapViewOfFile(mapping, FILE_MAP_READ, QWORD_HIGH(offset_align), QWORD_LOW(offset_align), (SIZE_T)(offset_extra + buf_len));
		CloseHandle(mapping);
	}
	if (!addr)
		return -1;
	
	// FIXME
	CopyMemory(buf, (BYTE *)addr + offset_extra, buf_len_lo);
	UnmapViewOfFile(addr);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API void *fio_operation_readonly(struct fio_request *ior, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);
	u64 offset_align, offset_extra;
	
	if (!ior)
		return NULL;

	if (ior->current_offset + buf_len > ior->base_length)
		return NULL;
	
	if (!buf_len)
		return NULL;

	offset_align = (ior->base_offset + ior->current_offset) & ~(virtualalloc_length_aligned - 1);
	offset_extra = (ior->base_offset + ior->current_offset) & (virtualalloc_length_aligned - 1);

	if (ior->ro_address) {
		UnmapViewOfFile(ior->ro_address);
		ior->ro_address = NULL;
	}

	// FIXME
	ior->ro_address = MapViewOfFile(ior->mapping, FILE_MAP_READ, QWORD_HIGH(offset_align), 
		QWORD_LOW(offset_align), (SIZE_T)(offset_extra + buf_len));
	if (!ior->ro_address)
		return NULL;

	ior->current_offset += buf_len;

	return (BYTE *)ior->ro_address + offset_extra;
}

IO_REQUEST_API int fio_operation_write(struct fio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);
	HANDLE mapping;
	u64 mapping_start, mapping_rem, mapping_size, mapping_length;
	void *addr;
	BYTE *mapping_src;

	if (!ior || !buf)
		return -1;

	//if (ior->current_offset + buf_len > ior->base_length)
	//	return -1;
	if (!buf_len)
		return 0;

	mapping = CreateFileMapping(ior->file, NULL, PAGE_READWRITE, QWORD_HIGH(buf_len + ior->current_offset), 
		QWORD_LOW(buf_len + ior->current_offset), NULL);
	if (!mapping)
		return -1;

	mapping_start = (ior->base_offset + ior->current_offset) & ~(virtualalloc_length_aligned - 1);
	mapping_rem = (ior->base_offset + ior->current_offset) & (virtualalloc_length_aligned - 1);
	mapping_src = (BYTE *)buf;

	if (mapping_rem) {	/* 先除掉不对齐的地方 */
		if (mapping_rem + buf_len > virtualalloc_length_aligned)
			mapping_size = virtualalloc_length_aligned;
		else
			mapping_size = mapping_rem + buf_len;

		addr = MapViewOfFile(mapping, FILE_MAP_WRITE, QWORD_HIGH(mapping_start), 
			QWORD_LOW(mapping_start), (u32)mapping_size);

		if (!addr) {
			CloseHandle(mapping);
			return -1;
		}

		CopyMemory((BYTE *)addr + mapping_rem, mapping_src, (u32)(mapping_size - mapping_rem));
		FlushViewOfFile((BYTE *)addr + mapping_rem, (u32)(mapping_size - mapping_rem));	
		UnmapViewOfFile(addr);
		mapping_src += mapping_size - mapping_rem;
		mapping_start += virtualalloc_length_aligned;
		mapping_length = buf_len - (mapping_size - mapping_rem);
	} else
		mapping_length = buf_len;

	mapping_rem = mapping_length & (virtualalloc_length_aligned - 1);
	mapping_length -= mapping_rem;

	while (mapping_length > 0) {
		if (mapping_length > 64 * 1024 * 1024)
			mapping_size = 64 * 1024 * 1024;
		else if (mapping_length > 32 * 1024 * 1024)
			mapping_size = 32 * 1024 * 1024;
		else if (mapping_length > 16 * 1024 * 1024)
			mapping_size = 16 * 1024 * 1024;
		else if (mapping_length > 8 * 1024 * 1024)
			mapping_size = 8 * 1024 * 1024;
		else if (mapping_length > 4 * 1024 * 1024)
			mapping_size = 4 * 1024 * 1024;
		else if (mapping_length > 2 * 1024 * 1024)
			mapping_size = 2 * 1024 * 1024;
		else if (mapping_length > 1 * 1024 * 1024)
			mapping_size = 1 * 1024 * 1024;
		else if (mapping_length > 512 * 1024)
			mapping_size = 512 * 1024;
		else if (mapping_length > 256 * 1024)
			mapping_size = 256 * 1024;
		else if (mapping_length > 128 * 1024)
			mapping_size = 128 * 1024;
		else if (mapping_length > 64 * 1024)
			mapping_size = 64 * 1024;
		else
			mapping_size = virtualalloc_length_aligned;

		DWORD max_blks = DWORD(mapping_length / mapping_size);
		for (DWORD blk = 0; blk < max_blks; blk++) {
			addr = MapViewOfFile(mapping, FILE_MAP_WRITE, QWORD_HIGH(mapping_start), 
				QWORD_LOW(mapping_start), (u32)mapping_size);

			if (!addr)
				break;

			CopyMemory((BYTE *)addr, mapping_src, (u32)mapping_size);
			FlushViewOfFile((BYTE *)addr, (u32)mapping_size);	
			UnmapViewOfFile(addr);
			mapping_start += mapping_size;
			mapping_src += mapping_size;
		}
		if (blk != max_blks) {
			CloseHandle(mapping);
			return -1;
		}
		mapping_length -= mapping_size * max_blks;
	}

	if (mapping_rem) {
		addr = MapViewOfFile(mapping, FILE_MAP_WRITE, QWORD_HIGH(mapping_start), 
			QWORD_LOW(mapping_start), (u32)mapping_rem);

		if (!addr) {
			CloseHandle(mapping);
			return -1;
		}

		CopyMemory((BYTE *)addr, mapping_src, (u32)mapping_rem);
		FlushViewOfFile((BYTE *)addr, (u32)mapping_rem);	
		UnmapViewOfFile(addr);
	}
	CloseHandle(mapping);
	ior->current_offset += buf_len;
	if (ior->current_offset > ior->base_length)
		ior->base_length += ior->current_offset - ior->base_length;

	return 0;
}

IO_REQUEST_API int fio_operation_length_of(struct fio_request *ior, u32 *len_lo, u32 *len_hi)
{
	if (!ior || !len_lo)
		return -1;

	*len_lo = QWORD_LOW(ior->base_length);
	if (len_hi)
		*len_hi = QWORD_HIGH(ior->base_length);

	return 0;	
}

/********** bio operations function **********/

IO_REQUEST_API struct bio_request *bio_operation_initialize(void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);
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

IO_REQUEST_API int bio_operation_seek(struct bio_request *ior, s32 offset_lo, s32 offset_hi, unsigned long method)
{
	s64 offset = (s64)BUILD_QWORD(offset_lo, offset_hi);

	switch (method) {
	case IO_SEEK_SET:
		break;
	case IO_SEEK_CUR:
		offset += (s64)ior->current_offset;
		break;
	case IO_SEEK_END:
		offset += (s64)ior->base_length;
		break;
	default:
		return -1;
	}	
	if (offset < 0 || (u64)offset > ior->base_length)
		return -1;
	
	ior->current_offset = offset;

	return 0;
}

IO_REQUEST_API int bio_operation_locate(struct bio_request *ior, u32 *location_lo, u32 *location_hi)
{
	if (!ior || !location_lo)
		return -1;
	
	*location_lo = QWORD_LOW(ior->current_offset);
	if (location_hi)
		*location_hi = QWORD_HIGH(ior->current_offset);

	return 0;
}

IO_REQUEST_API int bio_operation_read(struct bio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);

	if (!ior || !buf)
		return -1;
	
	if (!buf_len)
		return 0;

	if (ior->current_offset + buf_len > ior->base_length)
		return -1;	

	// FIXME
	CopyMemory(buf, (char *)ior->buffer + ior->current_offset, buf_len_lo);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API void *bio_operation_readonly(struct bio_request *ior, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = BUILD_QWORD(buf_len_lo, buf_len_hi);
	void *ret;
	
	if (!ior || !buf_len)
		return NULL;
	
	if (ior->current_offset + buf_len > ior->base_length)
		return NULL;	

	ret = (char *)ior->buffer + ior->current_offset;
	ior->current_offset += buf_len;

	return ret;
}

IO_REQUEST_API int bio_operation_write(struct bio_request *ior, void *buf, u32 buf_len_lo, u32 buf_len_hi)
{
	u64 buf_len = (u64)BUILD_QWORD(buf_len_lo, buf_len_hi);
	
	if (!ior || !buf)
		return -1;
	
	if (!buf_len)
		return 0;
	
	if (ior->current_offset + buf_len > ior->base_length)
		return -1;	
	
	// FIXME
	CopyMemory((char *)ior->buffer + ior->current_offset, buf, buf_len_lo);
	ior->current_offset += buf_len;

	return 0;
}

IO_REQUEST_API int bio_operation_length_of(struct bio_request *ior, u32 *len_lo, u32 *len_hi)
{
	if (!ior || !len_lo)
		return -1;

	*len_lo = QWORD_HIGH(ior->base_length);
	if (len_hi)
		*len_hi = QWORD_HIGH(ior->base_length);

	return 0;	
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
 	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);
	virtualalloc_length_aligned = sys_info.dwAllocationGranularity;

	return TRUE;
}
