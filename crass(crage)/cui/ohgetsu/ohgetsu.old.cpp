#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>
#include <vector>

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ohgetsu_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".PAC"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-6-4 21:51"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			// "LZS"
	u32 uncomprlen;
} PAC_entry_t;

typedef struct {
	u16 is_compressed;
	u16 resource_id;
	u32 offset;
	u32 length;
} exe_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD is_compressed;
	void *release;
} my_PAC_entry_t;

struct PAC_config {
	const TCHAR *PAC_name;
	DWORD index_offset;
	const char *resource_name_prefix;
};

static struct PAC_config PalmyraTrial_PAC_config[] = {
	// type0
	{ _T("SCRIPT.PAC"), 0x000462c0, "TAK%05d.BIN" },
	// type1
	//{ _T("VISUAL.PAC"), 0x000462d8, "VIS%05d.TM2" },
	//{ _T("VISUAL.PAC"), 0x000462f0, "VIS%05d.TM2" },
{ _T("VISUAL.PAC"), 0x000463c8, "VIS%05d.TM2" },
	// type2
	//{ _T("MUSIC.PAC"), 0x00047844, "BGM%05d.OGG" },
	//{ _T("MUSIC.PAC"), 0x00048b88, "BGM%05d.OGG" },
{ _T("MUSIC.PAC"), 0x0004b2f8, "BGM%05d.OGG" },
	// type7
	//{ _T("SE.PAC"), 0x00000000, "_SE%05d.OGG" },
	//{ _T("SE.PAC"), 0x00048c18, "_SE%05d.OGG" },
{ _T("SE.PAC"), 0x0004b3e0, "_SE%05d.OGG" },
	// type8
	//{ _T("VOICE.PAC"), 0x00047878, "VCE%05d.OGG" },
	//{ _T("VOICE.PAC"), 0x00048d78, "VCE%05d.OGG" },
{ _T("VOICE.PAC"), 0x0004b678, "VCE%05d.OGG" },
	// type10
	{ _T("?MOVIE.PAC"), 0x00000000, "MOV%05d.MPG" },
	// type12
	{ _T("?ANIME.PAC"), 0x00000000, "ANM%05d.BIN" },
	{ NULL, 0x00000000, NULL }
};

struct game_config {
	const char *game_name;
	struct PAC_config *PAC_config;
};

static struct game_config PalmyraTrial = {
	"Palmyra.exe", PalmyraTrial_PAC_config
};

static struct game_config *game_config_list[] = {
	&PalmyraTrial,
	NULL
};

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;
	const DWORD win_size = 4096;
	WORD flag = 0;

	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte >= comprlen)
				break;
		}
  
		if (flag & 1) {
			uncompr[act_uncomprlen++] = compr[curbyte++];
			if (act_uncomprlen >= uncomprlen || curbyte >= comprlen)
				break;
		} else {
			DWORD win_pos = (act_uncomprlen + 0xfee) & 0xfff;// 以当前uncompr的指针，虚拟为window的win_pos
			DWORD win_offset = compr[curbyte++];
			DWORD copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			if (curbyte >= comprlen)
				break;

			if (win_offset <= win_pos)
				win_offset = win_pos - win_offset;	// 计算出win_pos和offset之间的长度
			else
				win_offset = win_pos + win_size - win_offset;
			
			BYTE *src = uncompr + act_uncomprlen - win_offset;
			BYTE *src_end = src + copy_bytes;

			while (src < uncompr) {
				uncompr[act_uncomprlen++] = ' ';
				if (act_uncomprlen >= uncomprlen)
					return act_uncomprlen;
				++src;
			}

			while (src < src_end) {
				uncompr[act_uncomprlen++] = *src++;
				if (act_uncomprlen >= uncomprlen)
					return act_uncomprlen;
			}
		}
	}

	return act_uncomprlen;
}

/********************* PAC *********************/

/* 封包匹配回调函数 */
static int ohgetsu_PAC_match(struct package *pkg)
{
	const char *_exe_path = get_options("exe");
	if (!_exe_path)
		return -CUI_EMATCH;

	TCHAR exe_path[MAX_PATH];
	sj2unicode(_exe_path, -1, exe_path, MAX_PATH);
	HANDLE exe_file = MyOpenFile(exe_path);
	if (!exe_file)
		return -CUI_EOPEN;
	MyCloseFile(exe_file);

	char *game = PathFindFileNameA(_exe_path);
	struct game_config **game_cfg;
	struct PAC_config *PAC_config = NULL;
	for (game_cfg = game_config_list; *game_cfg; ++game_cfg) {
		if (lstrcmpiA(game, (*game_cfg)->game_name))
			continue;

		struct PAC_config *PAC_cfg = (*game_cfg)->PAC_config;
		for (; PAC_cfg->PAC_name; ++PAC_cfg) {
			if (!lstrcmpi(PAC_cfg->PAC_name, pkg->name)) {
				PAC_config = PAC_cfg;
				break;
			}
		}
		if (PAC_cfg->PAC_name)
			break;
	}
	if (!PAC_config)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	package_set_private(pkg, PAC_config);

	return 0;	
}

/* 封包索引目录提取函数 */
static int ohgetsu_PAC_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
#if 1
	const char *_exe_path = get_options("exe");
	if (!_exe_path)
		return -CUI_EMATCH;

	TCHAR exe_path[MAX_PATH];
	acp2unicode(_exe_path, -1, exe_path, MAX_PATH);
	HANDLE exe_file = MyOpenFile(exe_path);
	if (exe_file == INVALID_HANDLE_VALUE)
		return -CUI_EOPEN;

	struct PAC_config *PAC_config = (struct PAC_config *)package_get_private(pkg);

	if (MySetFilePosition(exe_file, PAC_config->index_offset, 0, 0)) {
		MyCloseFile(exe_file);
		return -CUI_ESEEK;
	}

	vector<my_PAC_entry_t> my_PAC_index;
	int ret = 0;
	while (1) {
		exe_entry_t exe_entry;

		if (MyReadFile(exe_file, &exe_entry, sizeof(exe_entry))) {
			ret = -CUI_EREAD;
			break;
		}

		if (!exe_entry.resource_id)
			break;

		my_PAC_entry_t *my_pac_entry = new my_PAC_entry_t;
		if (!my_pac_entry) {
			ret = -CUI_EMEM;
			break;
		}

		sprintf(my_pac_entry->name, PAC_config->resource_name_prefix, exe_entry.resource_id); 
		my_pac_entry->length = exe_entry.length;
		my_pac_entry->is_compressed = exe_entry.is_compressed;
		my_pac_entry->offset = exe_entry.offset;
		my_pac_entry->release = my_pac_entry;
		my_PAC_index.push_back(*my_pac_entry);
	}
	MyCloseFile(exe_file);
	if (ret) {
		for (vector<my_PAC_entry_t>::iterator iter = my_PAC_index.begin(); iter != my_PAC_index.end(); ++iter)
			delete iter->release;
		return ret;
	}	

	
#else
	u32 PAC_size;
	if (pkg->pio->length_of(pkg, &PAC_size))
		return -CUI_ELEN;

	vector<my_PAC_entry_t> my_PAC_index;
	int ret = 0;
	for (DWORD offset = 0; offset < PAC_size; offset += 0x800) {
		PAC_entry_t pac_entry;

		if (pkg->pio->readvec(pkg, &pac_entry, sizeof(PAC_entry_t), 
				offset, IO_SEEK_SET)) {
			ret = -CUI_EREADVEC;
			break;
		}

		if (!strncmp(pac_entry.magic, "LZS", 4)) {
			my_PAC_entry_t *my_pac_entry = new my_PAC_entry_t;
			if (!my_pac_entry) {
				ret = -CUI_EMEM;
				break;
			}

			my_pac_entry->uncomprlen = pac_entry.uncomprlen;
			my_pac_entry->offset = offset + sizeof(PAC_entry_t);
			my_pac_entry->release = my_pac_entry;
			my_PAC_index.push_back(*my_pac_entry);
		}
	}
	if (ret) {
		for (vector<my_PAC_entry_t>::iterator iter = my_PAC_index.begin(); iter != my_PAC_index.end(); ++iter)
			delete iter->release;
		return ret;
	}	
#endif

	my_PAC_entry_t *index_buffer = new my_PAC_entry_t[my_PAC_index.size()]();
	if (!index_buffer) {
		for (vector<my_PAC_entry_t>::iterator iter = my_PAC_index.begin(); iter != my_PAC_index.end(); ++iter)
			delete iter->release;
		return -CUI_EMEM;
	}

#if 1
	for (vector<my_PAC_entry_t>::size_type i = 0; i < my_PAC_index.size(); ++i) {
		my_PAC_entry_t &entry = index_buffer[i];
		entry = my_PAC_index[i];
		delete my_PAC_index[i].release;
	}
#else
	for (vector<my_PAC_entry_t>::size_type i = 0; i < my_PAC_index.size(); ++i) {
		my_PAC_entry_t &entry = index_buffer[i];

		entry = my_PAC_index[i];
		sprintf(entry.name, "%08d", i);
		if (i + 1 != my_PAC_index.size())
			entry.comprlen = my_PAC_index[i + 1].offset - my_PAC_index[i].offset;
		else
			entry.comprlen = PAC_size - my_PAC_index[i].offset;
		
		delete my_PAC_index[i].release;
	}
#endif
	pkg_dir->index_entries = my_PAC_index.size();
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = my_PAC_index.size() * sizeof(my_PAC_entry_t);
	pkg_dir->index_entry_length = sizeof(my_PAC_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ohgetsu_PAC_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	my_PAC_entry_t *my_PAC_entry;

	my_PAC_entry = (my_PAC_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_PAC_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_PAC_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_PAC_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ohgetsu_PAC_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	my_PAC_entry_t *my_PAC_entry = (my_PAC_entry_t *)pkg_res->actual_index_entry;
	if (pkg_res->flags & PKG_RES_FLAG_RAW || !my_PAC_entry->is_compressed) {
		pkg_res->raw_data = raw;
		return 0;
	}

	PAC_entry_t *PAC_entry = (PAC_entry_t *)raw;
	if (strncmp(PAC_entry->magic, "LZS", 4)) {
		delete [] raw;
		return -CUI_EMATCH;		
	}

	BYTE *compr = (BYTE *)(PAC_entry + 1);
	DWORD uncomprlen = PAC_entry->uncomprlen;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = lzss_uncompress(uncompr, uncomprlen,
		compr, pkg_res->raw_data_length);
	delete [] raw;
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int ohgetsu_PAC_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
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
static void ohgetsu_PAC_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void ohgetsu_PAC_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ohgetsu_PAC_operation = {
	ohgetsu_PAC_match,					/* match */
	ohgetsu_PAC_extract_directory,		/* extract_directory */
	ohgetsu_PAC_parse_resource_info,	/* parse_resource_info */
	ohgetsu_PAC_extract_resource,		/* extract_resource */
	ohgetsu_PAC_save_resource,			/* save_resource */
	ohgetsu_PAC_release_resource,		/* release_resource */
	ohgetsu_PAC_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ohgetsu_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".PAC"), NULL, 
		NULL, &ohgetsu_PAC_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_OPTION))
			return -1;

	return 0;
}
