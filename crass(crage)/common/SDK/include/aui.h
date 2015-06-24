#ifndef AUI_H
#define AUI_H

#include "acui.h"

struct aui_register_callback;

/* aui必须暴露的API原型 */
typedef int (CALLBACK *register_aui_t)(struct aui_register_callback *callback);
///typedef void (CALLBACK *unregister_aui_t)(struct aui_unregister_callback *callback);
typedef void (CALLBACK *show_aui_info_t)(struct acui_information *info);

struct aui_extension;
/* aui信息（aui代码中不要直接操作以下任何成员） */
struct aui {
	struct aui *prev;		/* aui链表的前指针 */
	struct aui *next;		/* aui链表的后指针 */
	HMODULE module;			/* aui模块句柄 */
	TCHAR *name;			/* aui模块名 */
	struct aui_extension *first_ext;
	struct aui_extension *last_ext;		
	unsigned int ext_count;	/* 支持的扩展名数量 */
};

/* aui扩展名信息（aui代码中不要直接操作以下任何成员） */
struct aui_ext_operation;
struct aui_extension {
	struct aui_ext_operation *op;
	const TCHAR *extension;
	const TCHAR *lst_extension;
	const TCHAR *replace_extension;
	const TCHAR *description;
	struct aui_extension *next_ext;
	struct aui *aui;
	unsigned long flags;
};

/* 每个封包扩展名或特定封包名称的操作函数 */
struct aui_ext_operation {	
	int (*match)(struct resource *);				/* 资源是否符合封包条件 */
	int (*collect_resource_info)(struct resource *,					/* 获取资源文件信息 */
		struct package_resource *);
	int (*repacking_resource)(struct resource *,					/* 封装资源文件 */
		struct package *, struct package_resource *, struct package_directory *);
	int (*repacking)(struct package *, struct package_directory *);	/* 保存封包 */
};

/* aui注册时assage提供的回调函数 */
struct aui_register_callback {
	struct aui *aui;
	/* 添加扩展名以支持相应的封包文件。
	 * @handle: 操作句柄。
	 * @extension: 支持的封包扩展名，以"."开头（大小写不敏感）。
	 * @replace_extension: （可选）替换用的扩展名，以"."开头。
	 * @description: （可选）封包信息。
	 * @operation: 解包过程中使用的各种回调操作。
	 * @flags: 属性位段定义。
	 *
	 * 如果待解的封包是一个包含directory的封包，
	 * 那么rep_ext为空。
	 * 如果待解的封包是一个不包含directory的封包，
	 * 也就说是一个编码资源文件，且含扩展名，那么
	 * ext为文件名，且匹配由match()来作。
	 */
	int (*add_extension)(struct aui *aui, const TCHAR *extension, 
		const TCHAR *replace_extension, const TCHAR *lst_extension, 
		const TCHAR *description, struct aui_ext_operation *operation, unsigned long flags);
/* flags位段定义 */
#define AUI_EXT_FLAG_PKG		0x00000001	/* 封包型 */
#define AUI_EXT_FLAG_RES		0x00000002	/* 资源型 */
#define AUI_EXT_FLAG_NOEXT		0x00010000	/* 无扩展名 */
#define AUI_EXT_FLAG_DIR		0x00020000	/* 含目录 */
#define AUI_EXT_FLAG_LST		0x00040000	/* 需要额外的索引文件 */
};
	
#endif /* AUI_H */
