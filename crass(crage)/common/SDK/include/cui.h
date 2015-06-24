#ifndef CUI_H
#define CUI_H

struct cui_register_callback;

/* cui必须暴露的API原型 */
typedef int (CALLBACK *register_cui_t)(struct cui_register_callback *callback);
//typedef void (CALLBACK *unregister_cui_t)(struct cui_unregister_callback *callback);
typedef void (CALLBACK *show_cui_info_t)(struct acui_information *info);

struct cui_extension;
/* cui信息（cui代码中不要直接操作以下任何成员） */
struct cui {
	struct cui *prev;		/* cui链表的前指针 */
	struct cui *next;		/* cui链表的后指针 */
	HMODULE module;			/* cui模块句柄 */
	TCHAR *name;			/* cui模块名 */
	struct cui_extension *first_ext;
	struct cui_extension *last_ext;		
	unsigned int ext_count;	/* 支持的扩展名数量 */
};

/* cui扩展名信息（cui代码中不要直接操作以下任何成员） */
struct cui_extension {
	struct cui_ext_operation *op;
	const TCHAR *extension;
	const TCHAR *replace_extension;
	const TCHAR *description;
	struct cui_extension *next_ext;
	struct cui *cui;
	unsigned long flags;
};

/* 每个封包扩展名或特定封包名称的操作函数 */
struct cui_ext_operation {
	int (*match)(struct package *);				/* 是否支持该封包 */
	int (*extract_directory)(struct package *,	/* 提取directory数据 */
		struct package_directory *);
	int (*parse_resource_info)(struct package *,/* 解析resource信息 */
		struct package_resource *);	
	int (*extract_resource)(struct package *,	/* 提取资源文件数据 */
		struct package_resource *);	
	int (*save_resource)(struct resource *,		/* 保存资源数据 */
		struct package_resource *);	
	void (*release_resource)(struct package *,	/* 释放相关资源 */
		struct package_resource *);
	void (*release)(struct package *,			/* 释放相关资源 */
		struct package_directory *);
};

/* cui注册时crage提供的回调函数 */
struct cui_register_callback {
	struct cui *cui;
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
	int (*add_extension)(struct cui *cui, const TCHAR *extension, 
		const TCHAR *replace_extension, const TCHAR *description,
		struct cui_ext_operation *operation, unsigned long flags);
/* flags位段定义 */
#define CUI_EXT_FLAG_PKG		0x00000001	/* 封包型 */
#define CUI_EXT_FLAG_RES		0x00000002	/* 资源型 */
#define CUI_EXT_FLAG_NOEXT		0x00010000	/* 无扩展名 */
#define CUI_EXT_FLAG_DIR		0x00020000	/* 含目录 */
#define CUI_EXT_FLAG_LST		0x00040000	/* 需要额外的索引文件 */
#define CUI_EXT_FLAG_OPTION		0x00080000	/* 需要获得额外的配置参数 */
#define CUI_EXT_FLAG_NO_MAGIC	0x00100000	/* 没有magic匹配 */
#define CUI_EXT_FLAG_WEAK_MAGIC	0x00200000	/* 尽管没有magic匹配，但是扩展名相对独特 */
//#define CUI_EXT_FLAG_IMPACT		0x00400000	/* 对于magic相同的cui，在extract_dir和extract_resource阶段都不算错误 */
#define CUI_EXT_FLAG_SUFFIX_SENSE	0x00400000	/* 后缀名大小写敏感 */
#define CUI_EXT_FLAG_RECURSION		0x00800000	/* 资源递归处理 */
};

#endif /* CUI_H */
