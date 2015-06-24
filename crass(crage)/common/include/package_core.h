#ifndef PACKAGE_CORE_H
#define PACKAGE_CORE_H

#ifdef PACKAGE_CORE_EXPORTS
#define PACKAGE_CORE_API __declspec(dllexport)
#else
#define PACKAGE_CORE_API __declspec(dllimport)
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

extern PACKAGE_CORE_API void package_core_init(void);
extern PACKAGE_CORE_API void package_core_uninstall(void);
extern PACKAGE_CORE_API struct package *package_fio_create(const TCHAR *base_dir, const TCHAR *path);
extern PACKAGE_CORE_API struct package *package_bio_create(const TCHAR *base_dir, const TCHAR *path);
extern PACKAGE_CORE_API void package_release(struct package *pkg);
extern PACKAGE_CORE_API void package_uninstall(struct package *pkg);
extern PACKAGE_CORE_API int package_search_directory(const TCHAR *base_dir, const TCHAR *sub_dir);
extern PACKAGE_CORE_API int package_search_file(const TCHAR *path);
extern PACKAGE_CORE_API struct package *package_walk_each(struct package *pkg);
extern PACKAGE_CORE_API struct package *package_walk_each_safe(struct package **next);

#endif	/* PACKAGE_CORE_H */
