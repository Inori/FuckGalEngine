#include <tchar.h>
#include <windows.h>
#include <crass/locale.h>

static const TCHAR *string_table[] = {
	_T("Prog:\tCrage - a general Galgame resource extractor with cui extensibility\n"),
	_T("Author:\t%s\n"),	
	_T("Rev:\t%s\n"),
	_T("Date:\t%s\n"),
	_T("Rel:\t%s\n"),
	_T("Syntax:\n\t%s %s\n\n"),
	_T("Syntax error, please read FAQ and INSTALL for more help information\n\n"),
	_T("%s: fail to get directory information(%d)\n"),
	_T("Do you want to extract resource from next package? (y/n)\n"),
	_T("%s: invalid directory information\n"),
	_T("%s: incorrect directory information\n"),
	_T("%s: start the extraction (containing %d resources) ...\n"),
	_T("%s: start the extraction for package of resource ...\n"),
	_T("%s: fail to parse resource information(%d)\n"),
	_T("Do you want to extract resources from current package? (y/n)\n"),
	_T("%s: invalid resource name\n"),
	_T("%s: invalid resource\n"),
	_T("%s: fail to convert the resource name\n"),
	_T("%s: invalid resource(%d)\n"),
	_T("%s: fail to extract the resource(%d)\n"),
	_T("%s: fail to locate the resource\n"),
	_T("%s: fail to build the output path for saving\n"),
	_T("%s: error occurs in the process of extracting the resource(%d)\n"),
	_T("%s: fail to save the resource\n"),
 	_T("no enough disk space, retry? (y/n)\n"),
 	_T("%s: the specified resource is extracted successfully\n"),
 	_T("%s: extracting resource %ld / %ld %c\r"),
 	_T("%s: extracted resource %ld / %ld\n"),
 	_T("%s: fail to extract the %dth resource\n"),
	_T("\n%s£ºfail to extract the resource from %s(%d)\n"),
 	_T("%s: extracting package of resource %d %c\r"),
 	_T("%s: extracted %d packages                  \n\n"),
 	_T("%s: the specified cui doesn\'t exist\n"),
 	_T("\n\nInitializing cui core ...\n"),
 	_T("not found any cui\n"),
 	_T("Loading %d cui ...\n"),
 	_T("%s: support"),
	_T("\nInitializing package core ...\n"),
	_T("%s: not found any package\n"),
	_T("Reading %d packages ...\n"),
	_T("\nStart the extraction ...\n\n"),
	_T("\n\n\t\t\t\t\t\t... Finish the extraction\n\n"),
	_T("Copyright£º\t%s\n"),
	_T("System: \t%s\n"),
	_T("Package: \t%s\n"),
	_T("Version: \t%s\n"),
	_T("Author: \t%s\n"),
	_T("Date: \t\t%s\n"),
	_T("Notice: \t%s\n"),
	_T("%s: fail to search the single package\n"),
	_T("%s: fail to search the first package(%d)\n"),
	NULL
};

__declspec(dllexport) struct locale_configuration English_locale_configuration = {
	0,
	string_table
};


