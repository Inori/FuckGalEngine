#ifndef LOCALE_H
#define LOCALE_H

#ifdef LOCALE_EXPORTS
#define LOCALE_API __declspec(dllexport)
#else
#define LOCALE_API __declspec(dllimport)
#endif 

LOCALE_API int locale_init(void);
LOCALE_API const TCHAR *locale_load_string(DWORD id);
LOCALE_API int locale_app_register(struct locale_configuration *config, DWORD count);
LOCALE_API const TCHAR *locale_app_load_string(DWORD cid, DWORD id);

// 每个langurage dll注册的数据结构
struct locale_configuration {
	unsigned int acp;
	const TCHAR **string_table;
	unsigned int max_id;
	HMODULE module;
};

#define LOC_ID_CRASS_PROGRAM		0
#define LOC_ID_CRASS_AUTHOR			1
#define LOC_ID_CRASS_REVISION		2
#define LOC_ID_CRASS_DATE			3
#define LOC_ID_CRASS_RELEASE		4
#define LOC_ID_CRASS_SYNTAX			5
#define LOC_ID_CRASS_USAGE			6
#define LOC_ID_CRASS_GET_DIR_INFO	7
#define LOC_ID_CRASS_ASK_PKG		8
#define LOC_ID_CRASS_INVAL_DIR_INFO	9
#define LOC_ID_CRASS_INCOR_DIR_INFO	10
#define LOC_ID_CRASS_START_RIP_PKG	11
#define LOC_ID_CRASS_START_RIP_RPKG	12
#define LOC_ID_CRASS_PARSE_ERR		13
#define LOC_ID_CRASS_ASK_RES		14
#define LOC_ID_CRASS_INVAL_RES_NAME	15
#define LOC_ID_CRASS_INVAL_RES		16
#define LOC_ID_CRASS_RES_NAME_FAIL	17
#define LOC_ID_CRASS_INVAL_RES_FILE	18
#define LOC_ID_CRASS_RIP_RES_FAIL	19
#define LOC_ID_CRASS_LOC_RES_FAIL	20
#define LOC_ID_CRASS_BUILD_PATH		21
#define LOC_ID_CRASS_RIP_RES_ERR	22
#define LOC_ID_CRASS_SAVE_FAIL		23
#define LOC_ID_CRASS_NO_SPACE		24
#define LOC_ID_CRASS_RIP_RES_OK		25
#define LOC_ID_CRASS_RIP_RES_OK_C	26
#define LOC_ID_CRASS_RIP_RES_OK_N	27
#define LOC_ID_CRASS_RIP_RES_N_FAIL	28
#define LOC_ID_CRASS_RIP_PKG_FAIL	29
#define LOC_ID_CRASS_RIP_PKG_OK_C	30
#define LOC_ID_CRASS_RIP_PKG_OK_N	31
#define LOC_ID_CRASS_CUI_NON_EXIST	32
#define LOC_ID_CRASS_INIT_CUI_CORE	33
#define LOC_ID_CRASS_NO_CUI			34
#define LOC_ID_CRASS_LOAD_CUI		35
#define LOC_ID_CRASS_SUPPORT		36
#define LOC_ID_CRASS_INIT_PKG_CORE	37
#define LOC_ID_CRASS_NO_PKG			38
#define LOC_ID_CRASS_LOAD_PKG		39
#define LOC_ID_CRASS_START_CRAGE	40
#define LOC_ID_CRASS_FINISH_CRAGE	41
#define LOC_ID_CRASS_CUI_COPYRIGHT	42
#define LOC_ID_CRASS_CUI_SYSTEM		43
#define LOC_ID_CRASS_CUI_PKG		44
#define LOC_ID_CRASS_CUI_VER		45
#define LOC_ID_CRASS_CUI_AUTHOR		46
#define LOC_ID_CRASS_CUI_TIME		47
#define LOC_ID_CRASS_CUI_NOTICE		48
#define LOC_ID_CRASS_FAIL_FIND_PKG	49
#define LOC_ID_CRASS_FAIL_FIND_PKGS	50

#endif
