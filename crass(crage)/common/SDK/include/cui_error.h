#ifndef CUI_ERROR_H
#define CUI_ERROR_H

#define CUI_EPARA			1	/* 参数错误 */
#define CUI_EOPEN			2	/* 初始化失败 */
#define CUI_EREAD			3	/* 读操作失败 */
#define CUI_EWRITE			4	/* 写操作失败 */
#define CUI_ESEEK			5	/* 定位操作失败 */
#define CUI_EREADVEC		6
#define CUI_EWRITEVEC		7
#define CUI_EMATCH			8	/* 不支持的封包文件 */
#define CUI_EMEM			9	/* 内存分配失败 */
#define CUI_EUNCOMPR		10	/* 解压缩失败 */
#define CUI_ECOMPR			11	/* 压缩失败 */
#define CUI_ELEN			12
#define CUI_ECREATE			13
#define CUI_ELOC			14
#define CUI_EREADONLY		15
#define CUI_EREADVECONLY	16
#define CUI_EIMPACT			17	/* 发生错误,但继续提取 */
	
#endif	/* CUI_ERROR_H */
	