#include <tchar.h>
#include <windows.h>
#include <crass/locale.h>

static const TCHAR *string_table[] = {
	_T("程序：\tCrage - 使用cui插件扩展的通用游戏资源提取器\n"),
	_T("作者：\t%s\n"),	
	_T("版本：\t%s\n"),
	_T("日期：\t%s\n"),
	_T("发布：\t%s\n"),
	_T("语法：\n\t%s %s\n\n"),
	_T("语法错误，请阅读FAQ和INSTALL获得更多的帮助信息\n\n"),
	_T("%s: 获取directory信息失败(%d)\n"),
	_T("要继续提取下一个封包吗？(y/n)\n"),
	_T("%s：无效的directory信息\n"),
	_T("%s：不正确的directory信息\n"),
	_T("%s: 准备提取封包文件（含%d个资源文件） ...\n"),
	_T("%s: 准备提取资源封包文件 ...\n"),	// 12
	_T("%s: 解析resource信息失败(%d)\n"),
	_T("要继续提取封包内的其他资源吗？(y/n)\n"),
	_T("%s: 无效的resource名称\n"),	// 15
	_T("%s: 无效的resource\n"),
	_T("%s: 资源文件名转换失败\n"),
	_T("%s: 无效的资源文件(%d)\n"),
	_T("%s: 提取资源文件失败(%d)\n"),
	_T("%s: 定位资源文件失败\n"),
	_T("%s: 构造资源文件存储路径失败\n"),
	_T("%s: 提取资源文件时发生错误(%d)\n"),
	_T("%s: 保存失败\n"),
 	_T("磁盘空间不足，要重试吗？(y/n)\n"),
 	_T("%s: 指定的资源提取成功\n"),
 	_T("%s: 成功提取%ld / %ld 个资源文件 %c\r"),
 	_T("%s: 成功提取%ld / %ld 个资源文件\n"),
 	_T("%s：提取第%d个时失败\n"),
	_T("\n%s：提取封包%s时发生错误(%d)\n"),
 	_T("%s：成功提取%d个%s资源封包文件 %c\r"),
 	_T("%s：成功提取%d个%s封包文件          \n\n"),
 	_T("%s: 指定的cui不存在\n"),
 	_T("\n\n初始化cui core ...\n"),
 	_T("没有找到任何cui\n"),
 	_T("加载%d个cui ...\n"),
 	_T("%s: 支持"),
	_T("\n初始化package core ...\n"),
	_T("%s: 没有找到任何package\n"),
	_T("读取%d个封包文件信息 ...\n"),
	_T("\n开始执行解包操作 ...\n\n"),
	_T("\n\n\t\t\t\t\t\t\t... 解包操作执行完成\n\n"),
	_T("版权：\t%s\n"),
	_T("系统：\t%s\n"),
	_T("封包：\t%s\n"),
	_T("版本：\t%s\n"),
	_T("作者：\t%s\n"),
	_T("时间：\t%s\n"),
	_T("注意：\t%s\n"),
	_T("%s: 搜寻单个package时发生错误\n"),
	_T("%s: 搜寻第一个package时发生错误 (%d)\n"),
	NULL
};

__declspec(dllexport) struct locale_configuration SimplifiedChinese_locale_configuration = {
	936,
	string_table
};
