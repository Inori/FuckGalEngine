#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <acui.h>
#include <aui.h>
#include <acui_core.h>
#include <aui_error.h>
#include <crass/locale.h>
#include <utility.h>

struct aui_core {
	struct aui root;	/* 根aui */
	unsigned int count;	/* 已注册的aui个数 */
};

static struct aui_core aui_core;
static struct aui_register_callback aui_register_callback;

static void *aui_import(struct aui *aui, const TCHAR *function);

/*********** aui list **************/

#define aui_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
        	
#define aui_list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)       	
        	
#define aui_list_for_each_safe(pos, safe, head) \
	for (pos = (head)->next, safe = pos->next; pos != (head); \
        	pos = safe, safe = safe->next)   
        	       	
#define aui_list_for_each_safe_reverse(pos, safe, head) \
	for (pos = (head)->prev, safe = pos->prev; pos != (head); \
        	pos = safe, safe = safe->prev)  

static inline void aui_list_init(struct aui *aui)
{
	aui->prev = aui->next = aui;
}

static inline void __aui_list_del(struct aui *prev, struct aui *next)
{
	prev->next = next;
	next->prev = prev;
}

static inline void aui_list_del(struct aui *aui)
{
	__aui_list_del(aui->prev, aui->next);
}

static inline void aui_list_del_init(struct aui *aui)
{
	__aui_list_del(aui->prev, aui->next);
	aui_list_init(aui);
}

static inline void __aui_list_add(struct aui *prev, 
		struct aui *_new, struct aui *next)
{
	_new->prev = prev;
	_new->next = next;	
	prev->next = _new;
	next->prev = _new;
}

static inline void aui_list_add_tail(struct aui *_new, 
		struct aui *head)
{
	__aui_list_add(head->prev, _new, head);
}

static inline void aui_list_add(struct aui *_new, 
		struct aui *head)
{
	__aui_list_add(head, _new, head->next);
}

static inline void aui_list_add_core(struct aui *aui)
{
	aui_list_add_tail(aui, &aui_core.root);
	aui_core.count++;
}

/*********** register_aui API **************/

/* 允许添加具有相同扩展名的extension结构 */
static int aui_add_extension(struct aui *aui, const TCHAR *extension, 
		const TCHAR *replace_extension, const TCHAR *lst_extension, const TCHAR *description,
		struct aui_ext_operation *operation, unsigned long flags)
{
	struct aui_extension *ext;

	if (!aui || !operation) {
		crass_printf(_T("%s: 注册extension时发生错误\n"), aui->name);
	
		return -1;
	}

	if ((flags & AUI_EXT_FLAG_NOEXT) && !extension) {
		crass_printf( _T("%s: 必须指定AUI_EXT_FLAG_NOEXT标志\n"), aui->name);
		return -1;	
	}

	if ((flags & AUI_EXT_FLAG_LST) && !lst_extension) {
		crass_printf(_T("%s: 必须指定lst_extension参数\n"));
		return -1;	
	}

	if (flags & AUI_EXT_FLAG_DIR) {
		if (!operation->collect_resource_info) {
			crass_printf(_T("%s: 目录型封包必须支持collect_resource_info()\n"), aui->name);
			return -1;	
		}
	}

	if (!operation->repacking_resource) {
		crass_printf(_T("%s: 必须支持repacking_resource()\n"), aui->name);
		return -1;
	}

	if (!operation->repacking) {
		crass_printf(_T("%s: 必须支持repacking()\n"), aui->name);
		return -1;
	}

	ext = (struct aui_extension *)GlobalAlloc(GPTR, sizeof(*ext));
	if (!ext) {
		crass_printf(_T("%s: 分配extension时发生错误\n"), aui->name);
		return -1;
	}
	ext->op = operation;
	ext->extension = extension;
	ext->replace_extension = replace_extension;
	ext->lst_extension = lst_extension;
	ext->description = description;
	ext->aui = aui;
	ext->flags = flags;

	if (!aui->first_ext)
		aui->first_ext = ext;
	else
		aui->last_ext->next_ext = ext;
	aui->last_ext = ext;
	aui->ext_count++;

	return 0;
}

/*********** aui core utility **************/

static struct aui *aui_alloc(const TCHAR *aui_name)
{
	struct aui *aui;
	SIZE_T alloc_len;

	alloc_len = (_tcslen(aui_name) + 1) * sizeof(TCHAR) + sizeof(*aui);		
	aui = (struct aui *)GlobalAlloc(GPTR, alloc_len);
	if (!aui) {
		crass_printf(_T("%s: 分配aui时发生错误\n"), aui_name);
		return NULL;		
	}
	aui->name = (TCHAR *)(aui + 1);
	_tcscpy(aui->name, aui_name);
	
	return aui;		
}

static void aui_free(struct aui *aui)
{
	if (aui)
		GlobalFree(aui);
}

static int aui_register(const TCHAR *path)
{
	struct aui *aui;
	TCHAR *name;
	TCHAR aui_name[MAX_PATH];
	register_aui_t reg_aui;

	name = PathFindFileName(path);
	if (!name) {
		crass_printf(_T("%s: 无效的aui路径\n"), path);
		return -1;
	}
	_tcscpy(aui_name, name);
	if (!PathRenameExtension(aui_name, _T(""))) {
		crass_printf(_T("%s: 无效的aui名称\n"), aui_name);
		return -1;
	}

	aui = aui_alloc(aui_name);
	if (!aui)
		return -CUI_EMEM;

	aui->module = LoadLibrary(path);
	if (!aui->module) {		
		crass_printf(_T("%s: 加载aui时发生错误\n"), aui_name);
		aui_free(aui);
		return -1;
	}
	aui_list_init(aui);

	reg_aui = (register_aui_t)aui_import(aui, _T("register_aui"));
	if (!reg_aui) {
		crass_printf(_T("%s: aui不支持register_aui()\n"), aui_name);
		FreeLibrary(aui->module);
		aui_free(aui);
		return -1;
	}

	aui_register_callback.aui = aui;
	aui_register_callback.add_extension = aui_add_extension;
	if (reg_aui(&aui_register_callback)) {
		crass_printf(_T("%s: aui注册失败\n"), aui_name);
		FreeLibrary(aui->module);
		aui_free(aui);
		return -1;		
	}	
	aui_list_add_core(aui);

	return 0;
}

static void aui_unregister(struct aui *aui)
{
	if (!aui)
		return;
		
	aui_list_del(aui);	
	if (aui->module != INVALID_HANDLE_VALUE)
		FreeLibrary(aui->module);
	aui_free(aui);
}

static void aui_unregister_each(void)
{
	struct aui *root, *aui, *safe;

	root = &aui_core.root;
	aui_list_for_each_safe_reverse(aui, safe, root)
		aui_unregister(aui);
}

static int aui_load(const TCHAR *dir)
{
	HANDLE find_file;
	WIN32_FIND_DATA find_data;
	int count;
	TCHAR search[MAX_PATH];
	TCHAR path[MAX_PATH];

	_stprintf(search, _T("%s\\*.aui"), dir);	
	find_file = FindFirstFile(search, &find_data);
	if (find_file == INVALID_HANDLE_VALUE) {
		crass_printf(_T("%s: 搜寻aui时发生错误\n"), dir);
		return -1;
	}

	count = 0;
	_stprintf(path, _T("%s\\%s"), dir, find_data.cFileName);
	if (!aui_register(path))
		count++;
	/* 忽略注册失败的aui */

	while (FindNextFile(find_file, &find_data)) {
		_stprintf(path, _T("%s\\%s"), dir, find_data.cFileName);
		if (aui_register(path)) {
			crass_printf(_T("%s: 注册失败\n"), find_data.cFileName);
		} else
			count++;
	}
	FindClose(find_file);
	
	return count;			
}

static void *aui_import(struct aui *aui, const TCHAR *function)
{
	TCHAR tchar_func_name[32];
	char mbcs_name[32];

	_stprintf(tchar_func_name, _T("%s_%s"), aui->name, function);
#ifdef UNICODE
	wcstombs(mbcs_name, tchar_func_name, sizeof(mbcs_name));
	return GetProcAddress(aui->module, mbcs_name);
#else
	return GetProcAddress(aui->module, tchar_func_name);	
#endif
}

/**************** aui core API *****************/

ACUI_CORE_API struct aui *aui_walk_each(struct aui *aui)
{
	struct aui *root = &aui_core.root;
	
	if (!aui)
		aui = root;
		
	if (aui->next == root)
		return NULL;	
	
	return aui->next;
}

ACUI_CORE_API void aui_uninstall(struct aui *aui)
{
	if (!aui)
		return;

	aui_unregister(aui);
}

ACUI_CORE_API struct aui_extension *aui_extension_walk_each(struct aui *aui,
		struct aui_extension *extension, unsigned long flags)
{
	struct aui_extension *ext;

	if (!extension)
		extension = aui->first_ext;
	else
		extension = extension->next_ext;

	for (ext = extension; ext; ext = ext->next_ext) {
		if (ext->flags & flags)
			break;
	}
		
	return ext;
}

ACUI_CORE_API int aui_core_init(const TCHAR *aui_dir)
{	
	struct aui *root_aui;

	if (!aui_dir) {
		crass_printf(_T("%s: 无效的aui目录\n"), aui_dir);
		return -1;
	}
	memset(&aui_core, 0, sizeof(aui_core));
	/* 初始化根aui */
	root_aui = &aui_core.root;
	aui_list_init(root_aui);
	root_aui->name = _T("root");

	return aui_load(aui_dir);
}

ACUI_CORE_API void aui_core_uninstall(void)
{
	aui_unregister_each();
}

ACUI_CORE_API void aui_print_information(struct aui *aui)
{
	struct acui_information *info;
	
	info = (struct acui_information *)aui_import(aui, _T("aui_information"));
	if (!info)
		return;

	crass_printf(_T("****** %s ******\n"), aui->name);

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
