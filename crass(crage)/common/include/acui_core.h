#ifndef ACUI_CORE_H
#define ACUI_CORE_H

#ifdef ACUI_CORE_EXPORTS
#define ACUI_CORE_API __declspec(dllexport)
#else
#define ACUI_CORE_API __declspec(dllimport)
#endif 

extern ACUI_CORE_API int cui_core_init(const TCHAR *cui_dir);
extern ACUI_CORE_API void cui_core_uninstall(void);
extern ACUI_CORE_API struct cui *cui_walk_each(struct cui *cui);
extern ACUI_CORE_API void cui_print_information(struct cui *cui);
extern ACUI_CORE_API struct cui_extension *cui_extension_walk_each(struct cui *cui,
		struct cui_extension *extension, unsigned long flags);
extern ACUI_CORE_API void cui_uninstall(struct cui *cui);
extern ACUI_CORE_API int aui_core_init(const TCHAR *aui_dir);
extern ACUI_CORE_API void aui_core_uninstall(void);
extern ACUI_CORE_API struct aui *aui_walk_each(struct aui *aui);
extern ACUI_CORE_API void aui_print_information(struct aui *aui);
extern ACUI_CORE_API struct aui_extension *aui_extension_walk_each(struct aui *aui,
		struct aui_extension *extension, unsigned long flags);
extern ACUI_CORE_API void aui_uninstall(struct aui *aui);

#endif	/* ACUI_CORE_H */
