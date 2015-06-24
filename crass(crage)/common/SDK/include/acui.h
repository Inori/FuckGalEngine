#ifndef ACUI_H
#define ACUI_H

/* aui/cui导出信息 */
struct acui_information {
	const TCHAR *copyright;	/* 封包系统的版权描述信息 */
	const TCHAR *system;	/* 封包系统的描述信息 */
	const TCHAR *package;	/* 封包扩展名 */
	const TCHAR *revision;	/* 版本 */
	const TCHAR *author;	/* 作者 */
	const TCHAR *date;		/* 完成日期 */
	const TCHAR *notion;	/* 其他信息 */
/* 属性字段定义 */
#define ACUI_ATTRIBUTE_LEVEL_UNKNOWN	0x00000000	/* 未定义 */
#define ACUI_ATTRIBUTE_LEVEL_DEVELOP	0x00000001	/* 功能不完善，尚在开发中 */
#define ACUI_ATTRIBUTE_LEVEL_UNSTABLE	0x00000002	/* 功能虽然完善，但是不稳定 */
#define ACUI_ATTRIBUTE_LEVEL_STABLE		0x00000003	/* 功能完善且稳定 */
#define ACUI_ATTRIBUTE_SUPPORT64		0x40000000	/* 支持64位操作 */
#define ACUI_ATTRIBUTE_MT_SAFE			0x80000000	/* 多线程安全 */
#define ACUI_ATTRIBUTE_PRELOAD			0x01000000	/* 支持资源预读 */
	unsigned long attribute;/* 属性字段 */	
};

#endif	/* ACUI_H */
