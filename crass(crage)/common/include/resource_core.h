#ifndef RESOURCE_CORE_H
#define RESOURCE_CORE_H

#ifdef RESOURCE_CORE_EXPORTS
#define RESOURCE_CORE_API __declspec(dllexport)
#else
#define RESOURCE_CORE_API __declspec(dllimport)
#endif 

#if 0
enum {
	PKGRES_PKG,
	PKGRES_RES,
};

struct pkgres {
	struct pkgres_io *pio;
	HANDLE file;
	BYTE *data;
	DWORD current_position_lo;
	DWORD current_position_hi;
	//DWORD relative_offset_lo;
	//DWORD relative_offset_hi;
	//DWORD relative_length_lo;
	//DWORD relative_length_hi;
	struct pkgres *prev;
	struct pkgres *next;
	TCHAR *full_path;
	TCHAR *path;
	TCHAR *name;
	TCHAR *extension;
	DWORD length_lo;
	DWORD length_hi;
	//int processed;				/* 是否已经被处理过的标志 */
	//unsigned long flags;
	//int use_count;
	void *priv;						/* 封包私有信息 */
};
#endif

extern RESOURCE_CORE_API void resource_core_init(void);
extern RESOURCE_CORE_API void resource_core_uninstall(void);
extern RESOURCE_CORE_API struct resource *resource_create(const TCHAR *base_dir, const TCHAR *path);
extern RESOURCE_CORE_API void resource_release(struct resource *res);
extern RESOURCE_CORE_API void resource_uninstall(struct resource *res);
extern RESOURCE_CORE_API int resource_search_directory(const TCHAR *base_dir, const TCHAR *sub_dir);
extern RESOURCE_CORE_API int resource_search_file(const TCHAR *path);
extern RESOURCE_CORE_API struct resource *resource_walk_each(struct resource *res);
extern RESOURCE_CORE_API struct resource *resource_walk_each_safe(struct resource **next);

#endif	/* RESOURCE_CORE_H */
