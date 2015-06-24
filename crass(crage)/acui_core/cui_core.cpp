#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <acui.h>
#include <cui.h>
#include <acui_core.h>
#include <cui_error.h>
#include <utility.h>
#include <crass/locale.h>
#include <stdio.h>

struct cui_core {
	struct cui root;	/* 根cui */
	unsigned int count;	/* 已注册的cui个数 */
};

static struct cui_core cui_core;
static struct cui_register_callback cui_register_callback;

static void *cui_import(struct cui *cui, const TCHAR *function);

/*********** cui list **************/

#define cui_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
        	
#define cui_list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)       	
        	
#define cui_list_for_each_safe(pos, safe, head) \
	for (pos = (head)->next, safe = pos->next; pos != (head); \
        	pos = safe, safe = safe->next)   
        	       	
#define cui_list_for_each_safe_reverse(pos, safe, head) \
	for (pos = (head)->prev, safe = pos->prev; pos != (head); \
        	pos = safe, safe = safe->prev)  

static inline void cui_list_init(struct cui *cui)
{
	cui->prev = cui->next = cui;
}

static inline void __cui_list_del(struct cui *prev, struct cui *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void cui_list_del(struct cui *cui)
{
	__cui_list_del(cui->prev, cui->next);
}

static inline void cui_list_del_init(struct cui *cui)
{
	__cui_list_del(cui->prev, cui->next);
	cui_list_init(cui);
}

static inline void __cui_list_add(struct cui *prev, 
		struct cui *_new, struct cui *next)
{
	_new->prev = prev;
	_new->next = next;	
	prev->next = _new;
	next->prev = _new;
}

static inline void cui_list_add_tail(struct cui *_new, 
		struct cui *head)
{
	__cui_list_add(head->prev, _new, head);
}

static inline void cui_list_add(struct cui *_new, 
		struct cui *head)
{
	__cui_list_add(head, _new, head->next);
}

static inline void cui_list_add_core(struct cui *cui)
{
	cui_list_add_tail(cui, &cui_core.root);
	cui_core.count++;
}

/*********** register_cui API **************/

/* 允许添加具有相同扩展名的extension结构 */
static int cui_add_extension(struct cui *cui, const TCHAR *extension, 
		const TCHAR *replace_extension, const TCHAR *description,
		struct cui_ext_operation *operation, unsigned long flags)
{
	struct cui_extension *ext;

	if (!cui || !operation) {
		crass_printf(_T("%s: 注册extension时发生错误\n"), cui->name);
		return -1;
	}

	if (!extension && !(flags & CUI_EXT_FLAG_NOEXT)) {
		crass_printf(_T("%s: 必须指定CUI_EXT_FLAG_NOEXT标志\n"), cui->name);	
		return -1;	
	}

	if (flags & CUI_EXT_FLAG_DIR) {
		if (!operation->extract_directory) {
			crass_printf(_T("%s: 目录型封包必须支持extract_directory()\n"), cui->name);
			return -1;	
		}
		if (!operation->parse_resource_info) {
			crass_printf(_T("%s: 目录型封包必须支持parse_resource_info()\n"), cui->name);
			return -1;	
		}

		if (!operation->release) {
			crass_printf(_T("%s: 目录型封包必须支持release()\n"), cui->name);
			return -1;	
		}
	}

	if (!operation->match) {
		crass_printf(_T("%s: 必须支持match()\n"), cui->name);
		return -1;
	}

	if (operation->extract_resource) {
		if (!operation->release_resource) {
			crass_printf(_T("%s: 必须支持release_resource()\n"), cui->name);
			return -1;
		}
	}

	if (operation->release_resource) {
		if (!operation->extract_resource) {
			crass_printf(_T("%s: 必须支持extract_resource()\n"), cui->name);
			return -1;
		}
	}

	ext = (struct cui_extension *)GlobalAlloc(GPTR, sizeof(*ext));
	if (!ext) {
		crass_printf(_T("%s: 分配extension时发生错误\n"), 
			cui->name);
		return -1;
	}
	ext->op = operation;
	ext->extension = extension;
	ext->replace_extension = replace_extension;
	ext->description = description;
	ext->cui = cui;
	ext->flags = flags;

	if (!cui->first_ext)
		cui->first_ext = ext;
	else
		cui->last_ext->next_ext = ext;
	cui->last_ext = ext;
	cui->ext_count++;

	return 0;
}

/*********** cui core utility **************/

static struct cui *cui_alloc(const TCHAR *cui_name)
{
	struct cui *cui;
	SIZE_T alloc_len;

	alloc_len = (_tcslen(cui_name) + 1) * sizeof(TCHAR) + sizeof(*cui);		
	cui = (struct cui *)GlobalAlloc(GPTR, alloc_len);
	if (!cui) {
		crass_printf(_T("%s: 分配cui时发生错误\n"), cui_name);
		return NULL;		
	}
	cui->name = (TCHAR *)(cui + 1);
	_tcscpy(cui->name, cui_name);
	
	return cui;		
}

static void cui_free(struct cui *cui)
{
	if (cui)
		GlobalFree(cui);
}

static int cui_register(const TCHAR *path)
{
	struct cui *cui;
	TCHAR *name;
	TCHAR cui_name[MAX_PATH];
	register_cui_t reg_cui;

	name = PathFindFileName(path);
	if (!name) {
		crass_printf(_T("%s: 无效的cui路径\n"), path);
		return -1;
	}
	_tcscpy(cui_name, name);
	if (!PathRenameExtension(cui_name, _T(""))) {
		crass_printf(_T("%s: 无效的cui名称\n"), cui_name);
		return -1;
	}

	cui = cui_alloc(cui_name);
	if (!cui)
		return -CUI_EMEM;

	cui->module = LoadLibrary(path);
	if (!cui->module) {		
		crass_printf(_T("%s: 加载cui时发生错误\n"), cui_name);
		cui_free(cui);
		return -1;
	}
	cui_list_init(cui);

	reg_cui = (register_cui_t)cui_import(cui, _T("_register_cui"));
	if (!reg_cui) {
		crass_printf(_T("%s: cui不支持register_cui()\n"), cui_name);
		FreeLibrary(cui->module);
		cui_free(cui);
		return -1;
	}

	cui_register_callback.cui = cui;
	cui_register_callback.add_extension = cui_add_extension;
	if (reg_cui(&cui_register_callback)) {
		crass_printf(_T("%s: cui注册失败\n"), cui_name);
		FreeLibrary(cui->module);
		cui_free(cui);
		return -1;		
	}	
	cui_list_add_core(cui);

	return 0;
}

static void cui_unregister(struct cui *cui)
{
	if (!cui)
		return;
		
	cui_list_del(cui);	
	if (cui->module != INVALID_HANDLE_VALUE)
		FreeLibrary(cui->module);
	cui_free(cui);
}

static void cui_unregister_each(void)
{
	struct cui *root, *cui, *safe;

	root = &cui_core.root;
	cui_list_for_each_safe_reverse(cui, safe, root)
		cui_unregister(cui);
}

static int cui_load(const TCHAR *dir)
{
	HANDLE find_file;
	WIN32_FIND_DATA find_data;
	int count;
	TCHAR search[MAX_PATH];
	TCHAR path[MAX_PATH];

	_stprintf(search, _T("%s\\*.cui"), dir);	
	find_file = FindFirstFile(search, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		crass_printf(_T("%s: 搜寻cui时发生错误\n"), dir);
		return -1;
	}

	count = 0;
	_stprintf(path, _T("%s\\%s"), dir, find_data.cFileName);
	if (!cui_register(path)) 
		count++;
		/* 忽略注册失败的cui */

	while (FindNextFile(find_file, &find_data)) {
		_stprintf(path, _T("%s\\%s"), dir, find_data.cFileName);
		if (cui_register(path)) {
			crass_printf(_T("%s: 注册失败\n"), find_data.cFileName);
		} else
			count++;
	}
	FindClose(find_file);
	
	return count;			
}

static void *cui_import(struct cui *cui, const TCHAR *function)
{
	TCHAR tchar_func_name[MAX_PATH];
	char mbcs_name[MAX_PATH];
	int cui_name_len = _tcslen(cui->name) + 1;

	for (int i = 0, j = 0; i < cui_name_len; ++i, ++j) {
		if (cui->name[i] == _T(' ') || cui->name[i] == _T('-'))
			tchar_func_name[j] = _T('_');
		else if (cui->name[i] == _T('+'))
			tchar_func_name[j] = _T('p');
		else if (!i && (cui->name[0] >= '0' && cui->name[0] <= '9')) {
			tchar_func_name[0] = _T('_');
			tchar_func_name[1] = cui->name[i];
			j = 1;
		} else
			tchar_func_name[j] = cui->name[i];
	}
	_tcscat(tchar_func_name, function);

#ifdef UNICODE
	wcstombs(mbcs_name, tchar_func_name, sizeof(mbcs_name));
	return GetProcAddress(cui->module, mbcs_name);
#else
	return GetProcAddress(cui->module, tchar_func_name);	
#endif
}

/**************** cui core API *****************/

ACUI_CORE_API void cui_uninstall(struct cui *cui)
{
	if (!cui)
		return;

	cui_unregister(cui);
}

ACUI_CORE_API struct cui *cui_walk_each(struct cui *cui)
{
	struct cui *root = &cui_core.root;
	
	if (!cui)
		cui = root;
		
	if (cui->next == root)
		return NULL;	
	
	return cui->next;
}

ACUI_CORE_API struct cui_extension *cui_extension_walk_each(struct cui *cui,
		struct cui_extension *extension, unsigned long flags)
{
	struct cui_extension *ext;

	if (!extension)
		extension = cui->first_ext;
	else
		extension = extension->next_ext;

	for (ext = extension; ext; ext = ext->next_ext) {
		if (ext->flags & flags)
			return ext;
	}

	return NULL;
}

ACUI_CORE_API int cui_core_init(const TCHAR *cui_dir)
{	
	struct cui *root_cui;

	if (!cui_dir) {
		crass_printf(_T("%s: 无效的cui目录\n"), cui_dir);
		return -1;
	}
	memset(&cui_core, 0, sizeof(cui_core));
	/* 初始化根cui */
	root_cui = &cui_core.root;
	cui_list_init(root_cui);
	root_cui->name = _T("root");

	return cui_load(cui_dir);
}

ACUI_CORE_API void cui_core_uninstall(void)
{
	cui_unregister_each();
}

ACUI_CORE_API void cui_print_information(struct cui *cui)
{
	struct acui_information *info;
	
	info = (struct acui_information *)cui_import(cui, _T("_cui_information"));
	if (!info)
		return;

	crass_printf(_T("****** %s ******\n"), cui->name);

	if (info->copyright)
		locale_printf(LOC_ID_CRASS_CUI_COPYRIGHT, info->copyright);
	if (info->system)
		locale_printf(LOC_ID_CRASS_CUI_SYSTEM, info->system);
	if (info->package)
		locale_printf(LOC_ID_CRASS_CUI_PKG, info->package);
	if (info->revision)
		locale_printf(LOC_ID_CRASS_CUI_VER, info->revision);
	if (info->author)
		locale_printf(LOC_ID_CRASS_CUI_AUTHOR, info->author);
	if (info->date)
		locale_printf(LOC_ID_CRASS_CUI_TIME, info->date);
	if (info->notion)
		locale_printf(LOC_ID_CRASS_CUI_NOTICE, info->notion);
}
