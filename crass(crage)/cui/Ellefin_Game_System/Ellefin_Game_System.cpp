#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <shlwapi.h>
#include <utility.h>
#include <vector>
#include <string>

using namespace std;
using std::vector;

struct acui_information Ellefin_Game_System_cui_information = {
	_T(""),			/* copyright */
	_T("Ellefin Game System"),			/* system */
	_T(".EPK"),				/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[3];	// "EPK"
	u8 flags;		// bit1 must be set
	u32 index_length;
} EPK_header_t;

typedef struct {
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} EPK_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} my_EPK_entry_t;

static DWORD global_dec_key;

static void parse_name_tree(vector<my_EPK_entry_t> &index, BYTE *directory, string &name)
{
	DWORD sets = *directory++;
	if (!sets)
		return;

	for (DWORD s = 0; s < sets; ++s) {
		BYTE chr = *directory++;
		name += chr;
		if (!chr) {
			my_EPK_entry_t entry;
			memset(&entry, 0, sizeof(entry));
			strcpy(entry.name, name.c_str());
			entry.offset = *(u16 *)directory;
			index.push_back(entry);
			name = "";
			return;
		}
		parse_name_tree(index, directory + 2 + *(u16 *)directory, name);
		directory += 2;
	}
}

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				break;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen >= uncomprlen)
					return;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* EPK *********************/

static int Ellefin_Game_System_EPK_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	EPK_header_t header;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "EPK", 3) || !(header.flags & 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Ellefin_Game_System_EPK_extract_directory(struct package *pkg,
													 struct package_directory *pkg_dir)
{
	EPK_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	char name[MAX_PATH];
	unicode2acp(name, MAX_PATH, pkg->name, -1);
	*strchr(name, '.') = 0;
	int nlen = strlen(name);
	DWORD key0 = 0xA6BD375E;
	DWORD key1 = 0x375D916B;
	char *np = name + nlen - 1;
	char *fnp = name;
	for (int i = 0; i < nlen; ++i) {
		key0 ^= (u8)*np--;
		key1 ^= (u8)*fnp++;
		key0 = (key0 >> 8) | (key0 << 24);
		key1 = (key1 << 8) | (key1 >> 24);
	}
	global_dec_key = key0;
	header.index_length ^= key1;
	if (header.flags & 1)
		header.index_length <<= 11;
    if (!(header.flags & 0xf0) || (header.flags & 1))
		header.index_length -= 8;

	BYTE *index = new BYTE[header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, header.index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	if (header.flags & 0xf0) {
		DWORD key = 0x17236351;
		u32 *p = (u32 *)index;
		for (DWORD i = 0; i < header.index_length / 4; ++i) {
			key = (key << 4) | (key >> 28);
			*p++ ^= key1;
			key1 = (key1 >> key) | (key1 << (32 - key));
		}
	}

	BYTE *p = index;
	u32 may_index_entries = *(u32 *)p;
	p += 4;
	int unknown_len = *p++;
	if (unknown_len) {
		char unknown[256];
		for (int i = 0; i < unknown_len; ++i)
			unknown[i] = *p++;
		unknown[i] = 0;
		printf("unknown %s\n", unknown);
	}

	u8 mode = *p++;
	u32 name_tree_length = *(u32 *)p;
	p += 4;
	vector<my_EPK_entry_t> EPK_index;
	string cur_name;
	DWORD index_entries;
	if (mode) {

	} else {
		parse_name_tree(EPK_index, p, cur_name);
		index_entries = EPK_index.size();
    }

	if (may_index_entries != index_entries) {
		delete [] index;
		return -CUI_EMATCH;
	}

	my_EPK_entry_t *my_index = new my_EPK_entry_t[index_entries];
	if (!my_index) {
		delete [] index;
		return -CUI_EMEM;
	}

	p += name_tree_length;
	for (i = 0; i < index_entries; ++i) {
		strcpy(my_index[i].name, EPK_index[i].name);
		EPK_entry_t *entry = (EPK_entry_t *)(p + EPK_index[i].offset * sizeof(EPK_entry_t));
		my_index[i].offset = entry->offset;
		my_index[i].comprlen = entry->comprlen;
		if (header.flags & 1) {
			my_index[i].offset <<= 11;
			my_index[i].comprlen <<= 11;
		}		
		my_index[i].comprlen = entry->comprlen;
		my_index[i].uncomprlen = entry->uncomprlen;
	}
	delete [] index;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = index_entries * sizeof(my_EPK_entry_t);
	pkg_dir->index_entry_length = sizeof(my_EPK_entry_t);
	package_set_private(pkg, header.flags);

	return 0;
}

static int Ellefin_Game_System_EPK_parse_resource_info(struct package *pkg,
													   struct package_resource *pkg_res)
{
	my_EPK_entry_t *EPK_entry;

	EPK_entry = (my_EPK_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, EPK_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = EPK_entry->comprlen;
	pkg_res->actual_data_length = EPK_entry->uncomprlen;
	pkg_res->offset = EPK_entry->offset;

	return 0;
}

static int Ellefin_Game_System_EPK_extract_resource(struct package *pkg,
													struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	BYTE *act_data;
	DWORD act_data_len;
	if (pkg_res->actual_data_length) {
		BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, pkg_res->actual_data_length, 
			compr, pkg_res->raw_data_length);
		pkg_res->actual_data = uncompr;
		act_data = uncompr;
		act_data_len = pkg_res->actual_data_length;
	} else {
		act_data = compr;
		act_data_len = pkg_res->raw_data_length;
	}

	u32 flags = (u32)package_get_private(pkg);
	if (flags & 4) {
		for (DWORD i = 0; i < act_data_len; ++i)
			act_data[i] = ((act_data[i] ^ 0xDF) >> 4) 
				| (((act_data[i] ^ 0xF9) & 0xF) << 4);
	}

	if (flags & 8) {
		DWORD key0 = 0x17236351;
		DWORD key1 = global_dec_key;
		u32 *p = (u32 *)act_data;
		for (DWORD i = 0; i < 4; ++i) {
			key0 = (key0 >> 4) | (key0 << 28);
			*p++ ^= key1;
			key1 = (key1 << key0) | (key1 >> (32 - key0));
		}
	}

	if (strstr(pkg_res->name, ".wav"))
		*(u32 *)act_data = 'FFIR';

	pkg_res->raw_data = compr;

	return 0;
}

static int Ellefin_Game_System_EPK_save_resource(struct resource *res, 
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

static void Ellefin_Game_System_EPK_release_resource(struct package *pkg, 
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

static void Ellefin_Game_System_EPK_release(struct package *pkg, 
											struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}


static cui_ext_operation Ellefin_Game_System_EPK_operation = {
	Ellefin_Game_System_EPK_match,				/* match */
	Ellefin_Game_System_EPK_extract_directory,	/* extract_directory */
	Ellefin_Game_System_EPK_parse_resource_info,/* parse_resource_info */
	Ellefin_Game_System_EPK_extract_resource,	/* extract_resource */
	Ellefin_Game_System_EPK_save_resource,		/* save_resource */
	Ellefin_Game_System_EPK_release_resource,	/* release_resource */
	Ellefin_Game_System_EPK_release				/* release */
};

int CALLBACK Ellefin_Game_System_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".EPK"), NULL, 
		NULL, &Ellefin_Game_System_EPK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
