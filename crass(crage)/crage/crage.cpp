#include <tchar.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <crass/crage_compile.h>
#include <crass/locale.h>
#include <crass_types.h>
#include <crass/io_request.h>
#include <crass/crass.h>
#include <cui.h>
#include <acui_core.h>
#include <cui_error.h>
#include <package.h>
#include <resource.h>
#include <package_core.h>
#include <resource_core.h>
#include <utility.h>
#include <locale.h>
#include <stdio.h>

static TCHAR status_ident[4] = { _T('|'), _T('/'), _T('-'), _T('\\') };

static TCHAR package_path[MAX_PATH];
static TCHAR package_directory[MAX_PATH];
static TCHAR lst_filepath[MAX_PATH];
static TCHAR output_directory[MAX_PATH];
static TCHAR CUI_name[32];
static unsigned int cui_show_infomation;
static unsigned int extract_idx;
static unsigned int extract_null;
static unsigned int extract_raw_resource;
static unsigned int verbose_info;
static unsigned int specify_option;
static unsigned int force_use_no_magic_cui;
static unsigned int ignore_crage_error;
static const TCHAR *specify_resource;

enum {
	CUI_EXTRACT_PACKAGE,
	CUI_EXTRACT_RESOURCE
};

static void print_issue(TCHAR *__argv[])
{
	locale_printf(LOC_ID_CRASS_PROGRAM);
	locale_printf(LOC_ID_CRASS_AUTHOR, CRAGE_AUTHOR);
	locale_printf(LOC_ID_CRASS_REVISION, CRAGE_VERSION);
	locale_printf(LOC_ID_CRASS_DATE, CRAGE_RELEASE_TIME);
	locale_printf(LOC_ID_CRASS_RELEASE, CRAGE_RELEASE);
	locale_printf(LOC_ID_CRASS_SYNTAX, 
		__argv[0] ? __argv[0] : _T("Crage.exe"), CRAGE_SYNTEX);
}

// 针对日文系统中windows文件路径..。
static void conver_backslash(TCHAR *arg)
{
	TCHAR *pos = _tcschr(arg, _T('\\'));
	if (pos)
		*pos = _T('/');
}

static void die(DWORD msg)
{
	locale_printf(msg);
	exit(-1);
}

static void early_parse_cmd(int __argc, TCHAR *__argv[])
{
	for (int i = 0; i < __argc; ++i) {
		if (!_tcscmp(__argv[i], _T("-wj"))) {
			use_jcrage_wrapper = 1;
			atexit(jcrage_exit);
			break;
		}
	}
}

static void parse_cmd(int __argc, TCHAR *__argv[])
{
	int i;

	for (i = 0; i < __argc; i++) {
		if (!_tcscmp(__argv[i], _T("-p"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			conver_backslash(__argv[i + 1]);
#ifndef UNICODE
			acp2unicode(__argv[i + 1], -1, package_path, SOT(package_path));
#else
			_tcsncpy(package_path, __argv[i + 1], SOT(package_path));
#endif
			i++;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-d"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			conver_backslash(__argv[i + 1]);
#ifndef UNICODE
			acp2unicode(__argv[i + 1], -1, package_directory, SOT(package_directory));
#else
			_tcsncpy(package_directory, __argv[i + 1], SOT(package_directory));
#endif
			i++;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-o"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			conver_backslash(__argv[i + 1]);
#ifndef UNICODE
			acp2unicode(__argv[i + 1], -1, output_directory, SOT(output_directory));
#else
			_tcsncpy(output_directory, __argv[i + 1], SOT(output_directory));
#endif
			i++;
			continue;
		}		
		if (!_tcscmp(__argv[i], _T("-O")) || !_tcscmp(__argv[i], _T("-0"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			specify_option = 1;
			if (parse_options(__argv[i + 1]))
				exit(-1);
			i++;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-w"))) {
			use_gui_wrapper = 1;
			crass_printf(_T("Command line:\n"));
			for (int n = 0; n < __argc; ++n)		
				crass_printf(_T("%s "), __argv[n]);			
			crass_printf(_T("\n\n"));			
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-wj"))) {
			crass_printf(_T("Command line:\n\t"));
			for (int n = 0; n < __argc; ++n)		
				crass_printf(_T("%s "), __argv[n]);			
			crass_printf(_T("\n\n"));			
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-l"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			conver_backslash(__argv[i + 1]);
#ifndef UNICODE
			acp2unicode(__argv[i + 1], -1, lst_filepath, SOT(lst_filepath));
#else
			_tcsncpy(lst_filepath, __argv[i + 1], SOT(lst_filepath));
#endif
			i++;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-n"))) {
			extract_null = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-r"))) {
			extract_raw_resource = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-i"))) {
			extract_idx = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-I"))) {
			if (!__argv[i + 1])
				cui_show_infomation = -1;	/* 显示所有插件的信息 */
			else {
				cui_show_infomation = 1;
#ifndef UNICODE
				acp2unicode(__argv[i + 1], -1, CUI_name, SOT(CUI_name));
#else
				_tcsncpy(CUI_name, __argv[i + 1], SOT(CUI_name));
#endif
				i++;
			}
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-u"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			else {
#ifdef UNICODE
				_tcsncpy(CUI_name, __argv[i + 1], SOT(CUI_name));
#else
				acp2unicode(__argv[i + 1], -1, CUI_name, SOT(CUI_name));
#endif
				i++;
			}
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-D"))) {
			warnning_verbose0 = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-Q"))) {
			warnning_exit = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-v"))) {
			verbose_info = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-F"))) {
			ignore_crage_error = 1;
			continue;
		}
		if (!_tcscmp(__argv[i], _T("-f"))) {
			if (!__argv[i + 1])
				die(LOC_ID_CRASS_USAGE);
			specify_resource = __argv[i + 1];
			i++;
			continue;
		}
	}

	if (!package_path[0] && !package_directory[0] && !cui_show_infomation)
		die(LOC_ID_CRASS_USAGE);

	/* 修正根路径引起的困扰 */
	if (package_directory[0]) {
		if (!PathIsRoot(package_directory)) {
			PathAddBackslash(package_directory);
			PathRemoveBackslash(package_directory);
		}
	}

	if (!output_directory[0]) {
		if (package_directory[0])
			_tcsncpy(output_directory, _T("output_dir"), SOT(output_directory));
	} else if (!PathIsRoot(output_directory)) {	// j: 这个不是根盘符；j:\ 这个是根盘符
		PathAddBackslash(output_directory);
		PathRemoveBackslash(output_directory);
	}
}

#if 0
static void package_resource_init(struct package_resource *pkg_res, 
								  void *index)
{
	memset(pkg_res, 0, sizeof(struct package_resource));
	pkg_res->index_entry = index;
}

static void package_resource_free(struct package_resource *pkg_res)
{
	if (pkg_res->raw)
		GlobalFree(pkg_res->raw);

	if (pkg_res->actual_data)
		GlobalFree(pkg_res->actual_data);

	struct package_resource *next, *save;
	for (next = pkg_res->next; next; next = save) {
		save = next->next;
		GlobalFree(next);
	}
}

static void fill_in_header(struct package_index_header *header, 
						   struct acui *cui, unsigned int entries,
						   unsigned int index_length)
{
	TCHAR tchar_name[MAX_PATH];
	char mbcs_name[MAX_PATH];

	_tcsncpy(tchar_name, cui->name, SOT(tchar_name));
	wcstombs(mbcs_name, tchar_name, sizeof(mbcs_name));
	memset(header, 0, sizeof(struct package_index_header));
	strcpy(header->cui_name, mbcs_name);
	header->entries = entries; 
	header->index_length = index_length;
}

static void fill_in_index(struct package_index_entry *entry, struct package_resource *pkg_res)
{
	strcpy(entry->name, pkg_res->name);
	entry->offset = pkg_res->offset_lo;
	entry->length = pkg_res->raw_length;
	entry->actual_length = pkg_res->actual_data_length;
}

static int flush_index(TCHAR *name, struct package_index_header *header, 
					   struct package_index_entry *index, 
					   DWORD index_length)
{
	TCHAR idx_name[MAX_PATH];
	HANDLE idx_file;

	_stprintf(idx_name, _T("%s.idx"), name);
	idx_file = MyCreateFile(idx_name);
	if (idx_file == INVALID_HANDLE_VALUE)
		return -1;

	if (MyWriteFile(idx_file, header, sizeof(struct package_index_header))) {
		MyCloseFile(idx_file);
		DeleteFile(idx_name);
		return -1;
	}

	if (MyWriteFile(idx_file, index, index_length)) {
		MyCloseFile(idx_file);
		DeleteFile(idx_name);
		return -1;
	}
	MyCloseFile(idx_file);
	_stprintf(fmt_buf, _T("%s: 保存索引成功\n"), idx_name);
	wcprintf(fmt_buf);
	return 0;
}
#endif	

static int cui_common_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}


/* 封包资源释放函数 */
static void cui_common_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void cui_common_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static int __crage(struct package *pkg, struct cui_extension *ext, int stepping)
{
	struct package_directory pkg_dir;
	struct package_resource pkg_res;
	TCHAR save_name[MAX_PATH];
	unsigned int i;
	int ret = -1;

	if (stepping == CUI_EXTRACT_RESOURCE)
		goto skip_lst_process;

	/* lst只在第一次处理 */
	if (ext->flags & CUI_EXT_FLAG_LST) {
		if (!lst_filepath[0])
			return -CUI_EMATCH;
	}

	if (lst_filepath[0]) {
		pkg->lst = package_fio_create(_T(""), lst_filepath);
		if (!pkg->lst) {
			crass_printf(_T("%s：为lst文件创建pacakge失败\n"), lst_filepath);
			return -1;
		}
	}

	if ((ext->flags & CUI_EXT_FLAG_OPTION) && !specify_option)
		return -CUI_EMATCH;

skip_lst_process:
	if (ext->op->match) {
		ret = ext->op->match(pkg);
		if (ret) {
			package_release(pkg->lst);
			return -CUI_EMATCH;
		}
	}

	memset(&pkg_dir, 0, sizeof(pkg_dir));
	if (ext->flags & CUI_EXT_FLAG_DIR) {
		/* 解压缩、解码、并提取出索引段数据 */
		ret = ext->op->extract_directory(pkg, &pkg_dir);
		if (ret) {
			locale_printf(LOC_ID_CRASS_GET_DIR_INFO, pkg->name, ret);
			ext->op->release(pkg, &pkg_dir);
			package_release(pkg->lst);
//			if (ext->flags & CUI_EXT_FLAG_IMPACT)
//				ret = -CUI_EIMPACT;

	dir_retry:
			TCHAR yes_or_no[3];

			yes_or_no[0] = _T('\x80');
			yes_or_no[1] = 0;

			locale_printf(LOC_ID_CRASS_ASK_PKG);
			wscanf(yes_or_no, SOT(yes_or_no));
			if (yes_or_no[0] == _T('y'))
				return -CUI_EIMPACT;
			else if (yes_or_no[0] != _T('n'))
				goto dir_retry;
			return ret;
		}

		if (!pkg_dir.directory || !pkg_dir.directory_length 
				|| !pkg_dir.index_entries || (!(pkg_dir.flags & PKG_DIR_FLAG_VARLEN) 
					&& !pkg_dir.index_entry_length)) {
			locale_printf(LOC_ID_CRASS_INVAL_DIR_INFO, pkg->name);
			ext->op->release(pkg, &pkg_dir);
			package_release(pkg->lst);

	dirinfo_retry:
			TCHAR yes_or_no[3];

			yes_or_no[0] = _T('\x80');
			yes_or_no[1] = 0;

			locale_printf(LOC_ID_CRASS_ASK_PKG);
			wscanf(yes_or_no, SOT(yes_or_no));
			if (yes_or_no[0] == _T('y'))
				return -CUI_EIMPACT;
			else if (yes_or_no[0] != _T('n'))
				goto dirinfo_retry;

			return -CUI_EPARA;			
		}

		if (!(pkg_dir.flags & PKG_DIR_FLAG_VARLEN)) {
			if (pkg_dir.index_entries * pkg_dir.index_entry_length 
					!= pkg_dir.directory_length) {
				locale_printf(LOC_ID_CRASS_INCOR_DIR_INFO, pkg->name);
				ext->op->release(pkg, &pkg_dir);
				package_release(pkg->lst);
				return -CUI_EPARA;	
			}
		}
		if (verbose_info || (stepping != CUI_EXTRACT_RESOURCE))
			locale_printf(LOC_ID_CRASS_START_RIP_PKG, 
				pkg->name, pkg_dir.index_entries);
	} else {
		/* 资源封包文件没有directory, 因此伪造directory信息 */
		pkg_dir.index_entries = 1;
		pkg_dir.flags = PKG_DIR_FLAG_SKIP0;
		if (verbose_info)
			locale_printf(LOC_ID_CRASS_START_RIP_RPKG, pkg->name);
	}	

	int res_pkg_count = 0;
	int specify_resource_hit = 0;	
	memset(&pkg_res, 0, sizeof(pkg_res));
	pkg_res.actual_index_entry = pkg_dir.directory;
	if ((ext->flags & CUI_EXT_FLAG_DIR) && !(pkg_dir.flags & PKG_DIR_FLAG_VARLEN))
		pkg_res.actual_index_entry_length = pkg_dir.index_entry_length;

	ret = -1;
	for (i = 0; i < pkg_dir.index_entries; i++) {
		TCHAR pkg_res_name[MAX_PATH];
		int skip_save, is_bio;
		int skip_resource;

		pkg_res.index_number = i;
		memset(pkg_res_name, 0, sizeof(pkg_res_name));
		if (i + 1 == pkg_dir.index_entries)
			pkg_res.flags = PKG_RES_FLAG_LAST;
		if (ext->op->parse_resource_info) {
			ret = ext->op->parse_resource_info(pkg, &pkg_res);
			if (ret) {
				locale_printf(LOC_ID_CRASS_PARSE_ERR, pkg->name, ret);
				ext->op->release_resource(pkg, &pkg_res);

			parse_retry:
				TCHAR yes_or_no[3];

				yes_or_no[0] = _T('\x80');
				yes_or_no[1] = 0;

				locale_printf(LOC_ID_CRASS_ASK_RES);
				wscanf(yes_or_no, SOT(yes_or_no));
				if (yes_or_no[0] == _T('y'))
					goto retry_next;
				else if (yes_or_no[0] != _T('n'))
					goto parse_retry;
				break;
			}
		
			if (ext->flags & CUI_EXT_FLAG_DIR) {
				if (pkg_res.flags & PKG_RES_FLAG_UNICODE) {
					if (!*(WCHAR *)pkg_res.name) {
						locale_printf(LOC_ID_CRASS_INVAL_RES_NAME, pkg->name);
						ext->op->release_resource(pkg, &pkg_res);
						break;
					}
#ifndef UNICODE
					WCHAR specify_resource_u[MAX_PATH];
					acp2unicode(specify_resource, -1, specify_resource_u, MAX_PATH);
#endif
					if (!lstrcmpi(specify_resource, (WCHAR *)pkg_res.name))
						specify_resource_hit = 1;
				} else if (!pkg_res.name[0]) {
					locale_printf(LOC_ID_CRASS_INVAL_RES_NAME, pkg->name);
					ext->op->release_resource(pkg, &pkg_res);
					break;
				} else if (specify_resource) {
#ifdef UNICODE
					char specify_resource_ansi[MAX_PATH];
					unicode2sj(specify_resource_ansi, MAX_PATH, specify_resource, -1);
					if (!strcmpi(specify_resource_ansi, pkg_res.name))
#else
					if (!strcmpi(specify_resource, pkg_res.name))
#endif
						specify_resource_hit = 1;
				}

				if (specify_resource && !specify_resource_hit)
					goto retry_next;

				if ((pkg_dir.flags & PKG_DIR_FLAG_VARLEN) && !pkg_res.actual_index_entry_length) {
					locale_printf(LOC_ID_CRASS_INVAL_RES, pkg->name);
					ext->op->release_resource(pkg, &pkg_res);
					break;
				}
				//if (output_index)
				//	fill_in_index(&index_buffer[i], &pkg_res);
			}

			if (pkg_res.name[0]) { 
				if (!(pkg_res.flags & PKG_RES_FLAG_UNICODE)) {
					if (sj2unicode((char *)pkg_res.name, pkg_res.name_length, 
							pkg_res_name, SOT(pkg_res_name))) {
						locale_printf(LOC_ID_CRASS_RES_NAME_FAIL, pkg->name);
						ext->op->release_resource(pkg, &pkg_res);
						break;
					}
				} else {
					if (pkg_res.name_length == -1)
						wcscpy(pkg_res_name, (WCHAR *)pkg_res.name);
					else
						wcsncpy(pkg_res_name, (WCHAR *)pkg_res.name, pkg_res.name_length);
				}
			} else {
				/* 资源封包通常没有独立的文件名 */
				_tcsncpy(pkg_res_name, pkg->path, SOT(pkg_res_name));
			}

			if (!pkg_res.raw_data_length && (pkg_dir.flags & PKG_DIR_FLAG_SKIP0)) {
				skip_resource = 1;
				goto skip_zero_resource;
			} else
				skip_resource = 0;

			if (!pkg_res.raw_data_length) {
				locale_printf(LOC_ID_CRASS_INVAL_RES_FILE, 
					pkg_res_name, pkg_res.raw_data_length);
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
		} else {
			u32 len_lo, len_hi;
			if (pkg->pio->length_of64(pkg, &len_lo, &len_hi)) {
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
			// FIXME
			pkg_res.raw_data_length = len_lo;
			/* 资源封包通常没有独立的文件名 */
			pkg_res.flags |= PKG_RES_FLAG_UNICODE;
			_tcsncpy((TCHAR *)pkg_res.name, pkg->path, SOT(pkg_res.name));
			pkg_res.name_length = -1;
			_tcsncpy(pkg_res_name, pkg->path, SOT(pkg_res_name));			
		}

		if (extract_raw_resource)
			pkg_res.flags |= PKG_RES_FLAG_RAW;
	
		is_bio = 0;
		if (ext->op->extract_resource) {	
			ret = ext->op->extract_resource(pkg, &pkg_res);
			if (ret) {
				TCHAR yes_or_no[3];

				locale_printf(LOC_ID_CRASS_RIP_RES_FAIL, pkg_res_name, ret);
				if (ignore_crage_error)
					goto retry_next;

			crage_retry:
				locale_printf(LOC_ID_CRASS_ASK_RES);

				yes_or_no[0] = _T('\x80');
				yes_or_no[1] = 0;
				wscanf(yes_or_no, SOT(yes_or_no));
				if (yes_or_no[0] == _T('y'))
					goto retry_next;
				else if (yes_or_no[0] != _T('n'))
					goto crage_retry;
				break;	
			}
			if (!(pkg_res.flags & PKG_RES_FLAG_FIO))
				is_bio = 1;
		}
		if (!is_bio) {
			if (pkg->pio->seek(pkg, pkg_res.offset, IO_SEEK_SET)) {
				locale_printf(LOC_ID_CRASS_LOC_RES_FAIL, pkg_res_name);
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
		}

		if (!(pkg_res.flags & PKG_RES_FLAG_UNICODE)) {
			if (pkg_res.name[0]) { 
				if (sj2unicode((char *)pkg_res.name, pkg_res.name_length, 
						pkg_res_name, SOT(pkg_res_name))) {
						locale_printf(LOC_ID_CRASS_RES_NAME_FAIL, pkg->name);
						ext->op->release_resource(pkg, &pkg_res);
					break;
				}
			}
		} else {
			if (((WCHAR *)pkg_res.name)[0]) { 
				if (pkg_res.name_length == -1)
					wcscpy(pkg_res_name, (WCHAR *)pkg_res.name);
				else
					wcsncpy(pkg_res_name, (WCHAR *)pkg_res.name, pkg_res.name_length);
			}
		}

		//if (!pkg_res.actual_data_length)
		//	pkg_res.actual_data_length = pkg_res.raw_data_length;

		if (verbose_info) {
			crass_printf(_T("[%d]%s: %ld / %ld bytes @ %08x\n"), i, pkg_res_name,
				pkg_res.raw_data_length, 
				!pkg_res.actual_data_length ? pkg_res.raw_data_length : pkg_res.actual_data_length,
				pkg_res.offset);
		}

		if (pkg_res.flags & PKG_RES_FLAG_REEXT) {
			if (pkg_res.replace_extension)
				PathRenameExtension(pkg_res_name, pkg_res.replace_extension);
		} else if (pkg_res.flags & PKG_RES_FLAG_APEXT)
			_tcscat(pkg_res_name, pkg_res.replace_extension);
		else if (ext->replace_extension && ext->replace_extension[0] == _T('.')) {			
		//	if (!_tcschr(pkg_res_name, _T('.')))
		//		_tcscat(pkg_res_name, ext->replace_extension);
	//		else
				PathRenameExtension(pkg_res_name, ext->replace_extension);
		}

		TCHAR res_full_path[MAX_PATH];		
		memset(res_full_path, 0, sizeof(res_full_path));
		if (ext->flags & CUI_EXT_FLAG_DIR) {
			/* 构造资源文件的存储路径 */
			if (!(ext->flags & CUI_EXT_FLAG_NOEXT))
				_tcsncpy(res_full_path, pkg->path, pkg->extension - pkg->path);
			else
				_tcsncpy(res_full_path, pkg->path, SOT(res_full_path));
		}

		if (!PathAppend(res_full_path, pkg_res_name)) {
			locale_printf(LOC_ID_CRASS_BUILD_PATH, pkg_res_name);
			ext->op->release_resource(pkg, &pkg_res);
			break;
		}

		skip_save = 0;
		if (!extract_raw_resource /*&& (ext->flags & CUI_EXT_FLAG_DIR)*/) {
			struct package *res_pkg;			
			struct cui_extension *res_ext;
			int err;
			
			/* 为资源文件创建一个新的pkg */
			if (is_bio)
				res_pkg = package_bio_create(_T(""), res_full_path);
			else
				res_pkg = package_fio_create(_T(""), res_full_path);
			if (!res_pkg) {
				crass_printf(_T("%s: 分配资源文件package时失败\n"), pkg_res_name);
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
			res_pkg->pkg_res = &pkg_res;
			res_pkg->parent = pkg;

			res_ext = NULL;	// from first				
			err = 0;
			while ((res_ext = cui_extension_walk_each(ext->cui, res_ext, CUI_EXT_FLAG_RES))) {
				/* 
				 * 形如MalieSystem、RealLive seen.txt那样的内部资源递归提取,
				 * 如果match不做特殊检测,则会发生二次提取错误.
				 */
				if ((ext == res_ext) && !(ext->flags & CUI_EXT_FLAG_RECURSION))
					continue;

				//if (lst_filepath[0] && !(ext->flags & CUI_EXT_FLAG_LST))
				//if (lst_filepath[0] && !(res_ext->flags & CUI_EXT_FLAG_LST))
				//	continue;

				if (!CUI_name[0] && !force_use_no_magic_cui) {
					if (res_ext->flags & CUI_EXT_FLAG_NO_MAGIC)
						continue;
				}

				if (!(res_ext->flags & CUI_EXT_FLAG_NOEXT) ^ 
						(res_pkg->extension && (res_pkg->extension[0] == _T('.'))))
					continue;

				if (!(res_ext->flags & CUI_EXT_FLAG_NOEXT)) {
					DWORD res_pkg_ext_len = _tcslen(res_pkg->extension);
					DWORD res_ext_len = _tcslen(res_ext->extension);
					for (DWORD ext_len = 1; ext_len < res_ext_len; ++ext_len) {
						if (res_ext->extension[ext_len] == _T('*')) {
							if (res_pkg_ext_len < ext_len)
								res_pkg_ext_len = 0;
							else
								res_ext_len = res_pkg_ext_len = ext_len;
							break;
						}
					}
					if (!res_pkg_ext_len)
						continue;

					if (res_pkg_ext_len != res_ext_len)
						continue;

					if (res_ext->flags & CUI_EXT_FLAG_SUFFIX_SENSE) {
						if (_tcsncmp(res_pkg->extension, res_ext->extension, res_pkg_ext_len))
							continue;
					} else {
						if (_tcsnicmp(res_pkg->extension, res_ext->extension, res_pkg_ext_len))
							continue;
					}
				}

				int ret = __crage(res_pkg, res_ext, CUI_EXTRACT_RESOURCE);
				if (ret < 0 && ret != -CUI_EMATCH) {
					locale_printf(LOC_ID_CRASS_RIP_RES_ERR, res_pkg->name, ret);
					err = 1;
					break;
				} else if (!ret)
					break;
			}
			package_release(res_pkg);	
			if (res_ext) {
				if (err) {
					ext->op->release_resource(pkg, &pkg_res);
					break;	/* 发生错误 */
				} else {
					skip_save = 1;	
				}
			}
			/* 对于无需另做处理的资源文件，res_ext返回NULL */
		}

		if (!extract_null && !skip_save) {
			struct resource *res;

			if (output_directory[0]) {
				_tcsncpy(save_name, output_directory, SOT(save_name));
				if (!PathAppend(save_name, res_full_path)) {
					locale_printf(LOC_ID_CRASS_BUILD_PATH, save_name);
					ext->op->release_resource(pkg, &pkg_res);
					break;
				}
			} else
				_tcsncpy(save_name, res_full_path, SOT(save_name));

//			if (ext->replace_extension && ext->replace_extension[0] == _T('.'))
//				PathRenameExtension(save_name, ext->replace_extension);
		resave:
			res = resource_create(_T(""), save_name);
			if (!res) {
				crass_printf(_T("%s: 分配资源文件resource时失败\n"), save_name);
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
			res->pkg_res = &pkg_res;

			ret = ext->op->save_resource(res, &pkg_res);
			if (ret) {
				int reason = GetLastError();
				locale_printf(LOC_ID_CRASS_SAVE_FAIL, save_name);
				if (reason == ERROR_DISK_FULL) {
					TCHAR yes_or_no[3];
				retry:
					locale_printf(LOC_ID_CRASS_NO_SPACE);

					yes_or_no[0] = _T('\x80');
					yes_or_no[1] = 0;
					wscanf(yes_or_no, SOT(yes_or_no));
					if (yes_or_no[0] == _T('y')) {
						resource_release(res);
						goto resave;
					} else if (yes_or_no[0] != _T('n'))
						goto retry;
				}
				resource_release(res);
				ext->op->release_resource(pkg, &pkg_res);
				break;
			}
			resource_release(res);
		}
		res_pkg_count++;
retry_next:
		ext->op->release_resource(pkg, &pkg_res);
skip_zero_resource:
		if (ext->flags & CUI_EXT_FLAG_DIR) {
			if (specify_resource && specify_resource_hit) {
				locale_printf(LOC_ID_CRASS_RIP_RES_OK, pkg->name);
				break;
			}

			void *actual_index_entry = pkg_res.actual_index_entry;
			unsigned int actual_index_entry_length = pkg_res.actual_index_entry_length;

			memset(&pkg_res, 0, sizeof(pkg_res));
			if (pkg_dir.flags & PKG_DIR_FLAG_VARLEN)
				pkg_res.actual_index_entry = (BYTE *)actual_index_entry 
					+ actual_index_entry_length;
			else {
				pkg_res.actual_index_entry_length = pkg_dir.index_entry_length;
				pkg_res.actual_index_entry = (BYTE *)actual_index_entry
					+ pkg_dir.index_entry_length;
			}
			if (!verbose_info && (stepping != CUI_EXTRACT_RESOURCE) && !specify_resource && !skip_resource) {
				locale_printf(LOC_ID_CRASS_RIP_RES_OK_C, pkg->name,
					res_pkg_count, pkg_dir.index_entries, status_ident[res_pkg_count % 4]);
			}
		}
		if (verbose_info && !skip_resource) {
			locale_printf(LOC_ID_CRASS_RIP_RES_OK_N, pkg->name,
				res_pkg_count, pkg_dir.index_entries);
		}
	}
	if (ext->op->release)
		ext->op->release(pkg, &pkg_dir);
	package_release(pkg->lst);

	if (i != pkg_dir.index_entries && !specify_resource) {
		locale_printf(LOC_ID_CRASS_RIP_RES_N_FAIL, pkg->name, i + 1);
		return ret;	
	}

	if ((ext->flags & CUI_EXT_FLAG_DIR) && !verbose_info && (stepping != CUI_EXTRACT_RESOURCE) && !specify_resource) {
		locale_printf(LOC_ID_CRASS_RIP_RES_OK_N, pkg->name, 
			res_pkg_count, pkg_dir.index_entries);
		crass_printf(_T("\n"));
	}

	return 0;		
}

static int crage(const TCHAR *cui_name)
{
	struct cui *cui = NULL;
	int ret = -1;

	while ((cui = cui_walk_each(cui))) {		
		struct cui_extension *ext = NULL;		

		/* 使用特定的cui.
		 * TODO: 实现只使用特定集合的cui和不使用特定集合的cui.
		 */
		if (CUI_name[0]) {
			if (lstrcmpi(cui->name, CUI_name))
				continue;
		}

		while ((ext = cui_extension_walk_each(cui, ext, CUI_EXT_FLAG_PKG))) {
			struct package *pkg;
			struct package *next_pkg = NULL;
			unsigned int pkg_count = 0;

			if (!CUI_name[0] && !force_use_no_magic_cui) {
				if (ext->flags & CUI_EXT_FLAG_NO_MAGIC)
					continue;
			}

			while ((pkg = package_walk_each_safe(&next_pkg))) {
				if ((!(ext->flags & CUI_EXT_FLAG_NOEXT)) ^ 
						(pkg->extension && (pkg->extension[0] == _T('.'))))
					continue;

				if (!(ext->flags & CUI_EXT_FLAG_NOEXT)) {
					DWORD pkg_ext_len = _tcslen(pkg->extension);
					DWORD ext_len = _tcslen(ext->extension);
					for (DWORD e_len = 1; e_len < ext_len; ++e_len) {
						if (ext->extension[e_len] == _T('*')) {
							if (pkg_ext_len < e_len)
								pkg_ext_len = 0;
							else
								ext_len = pkg_ext_len = e_len;
							break;
						}
					}
					if (!pkg_ext_len)
						continue;

					if (pkg_ext_len != ext_len)
						continue;

					if (ext->flags & CUI_EXT_FLAG_SUFFIX_SENSE) {
						if (_tcsncmp(pkg->extension, ext->extension, pkg_ext_len))
							continue;
					} else {
						if (_tcsnicmp(pkg->extension, ext->extension, pkg_ext_len))
							continue;
					}
				}

				ret = __crage(pkg, ext, CUI_EXTRACT_PACKAGE);
				if (ret < 0 && ret != -CUI_EMATCH) {
					if (ret == -CUI_EIMPACT)
						continue;
					locale_printf(LOC_ID_CRASS_RIP_PKG_FAIL, 
						cui->name, pkg->name, ret);
					return ret;
				} else if (!ret) {
					package_uninstall(pkg);
					pkg_count++;
					if (!verbose_info) {
						locale_printf(LOC_ID_CRASS_RIP_PKG_OK_C, 
							cui->name, pkg_count, ext->extension ? ext->extension : _T(""),
							status_ident[pkg_count % 4]);
					}
				}
			}
			if (pkg_count) {
				locale_printf(LOC_ID_CRASS_RIP_PKG_OK_N, 
					cui->name, pkg_count, ext->extension ? ext->extension : _T(""));
			}
		}

		if (CUI_name[0])
			break;
	}

	return 0;
}

static void show_cui_information(void)
{
	struct cui *cui = NULL;

	crass_printf(_T("\n"));
	while ((cui = cui_walk_each(cui))) {
		if (cui_show_infomation != -1 && lstrcmpi(CUI_name, cui->name))
			continue;
		crass_printf(_T("\n"));
		cui_print_information(cui);
		crass_printf(_T("\n"));
		if (cui_show_infomation == 1)
			break;
	}
	if (!cui && cui_show_infomation == 1)
		locale_printf(LOC_ID_CRASS_CUI_NON_EXIST, CUI_name);
}

int _tmain(int argc, TCHAR *argv[])
{	
	int cui_count, pkg_count;
	struct cui *cui;

	early_parse_cmd(argc, argv);

	locale_init();
	print_issue(argv);

	parse_cmd(argc, argv);

	locale_printf(LOC_ID_CRASS_INIT_CUI_CORE);

	cui_count = cui_core_init(_T("cui"));
	if (cui_count < 0)
		return -1;
	else if (!cui_count) {
		locale_printf(LOC_ID_CRASS_NO_CUI);
		return -1;
	}
	locale_printf(LOC_ID_CRASS_LOAD_CUI, cui_count);

	if (verbose_info) {
		cui = NULL;
		while ((cui = cui_walk_each(cui))) {
			struct cui_extension *ext;			

			locale_printf(LOC_ID_CRASS_SUPPORT, cui->name);

			ext = NULL;
			while ((ext = cui_extension_walk_each(cui, ext, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))) {
				if (ext->flags & CUI_EXT_FLAG_NOEXT)
					continue;

				if (ext->extension) {
					if (!ext->description)
						crass_printf(_T("%s "), ext->extension);	
					else
						crass_printf(_T("%s(%s) "), ext->extension, ext->description);
				} else {
					if (!ext->description)
						crass_printf(_T("<%s> "), ext->replace_extension);	
					else
						crass_printf(_T("%s(%s) "), ext->replace_extension, ext->description);
				}
			}
			crass_printf(_T("\n"));
		}
	}

	if (cui_show_infomation) {	/* 只显示cui信息 */
		show_cui_information();
		cui_core_uninstall();
		return 0;
	}

	locale_printf(LOC_ID_CRASS_INIT_PKG_CORE);

	package_core_init();	
	
	pkg_count = 0;
	if (package_path[0])	/* 单独的封包 */
		pkg_count = package_search_file(package_path);
	if (package_directory[0])
		pkg_count += package_search_directory(package_directory, _T(""));

	if (pkg_count < 0) {
		package_core_uninstall();
		cui_core_uninstall();
		return -1;
	} else if (!pkg_count) {
		locale_printf(LOC_ID_CRASS_NO_PKG, package_directory);
		package_core_uninstall();
		cui_core_uninstall();
		return -1;
	}

	locale_printf(LOC_ID_CRASS_LOAD_PKG, pkg_count);
			
	locale_printf(LOC_ID_CRASS_START_CRAGE);

	crage(CUI_name);

	package_core_uninstall();
	cui_core_uninstall();	
	
	locale_printf(LOC_ID_CRASS_FINISH_CRAGE);

	return 0;
}
