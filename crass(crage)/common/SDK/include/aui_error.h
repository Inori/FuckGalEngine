#ifndef AUI_ERROR_H
#define AUI_ERROR_H
	
#include "cui_error.h"
	
#define AUI_EPARA		CUI_EPARA	/* 参数错误 */
#define AUI_EOPEN		CUI_EOPEN	/* 打开失败 */
#define AUI_ECREATE		CUI_ECREATE	/* 创建失败 */
#define AUI_EREAD		CUI_EREAD	/* 读操作失败 */
#define AUI_EWRITE		CUI_EWRITE	/* 写操作失败 */
#define AUI_ESEEK		CUI_ESEEK	/* 定位操作失败 */
#define AUI_EREADVEC	CUI_EREADVEC
#define AUI_EWRITEVEC	CUI_EWRITEVEC
#define AUI_EMATCH		CUI_EMATCH	/* 不支持的封包文件 */
#define AUI_EMEM		CUI_EMEM	/* 内存分配失败 */
#define AUI_EUNCOMPR	CUI_EUNCOMPR/* 解压缩失败 */
#define AUI_ECOMPR		CUI_ECOMPR	/* 压缩失败 */
#define AUI_ELEN		CUI_ELEN

#endif