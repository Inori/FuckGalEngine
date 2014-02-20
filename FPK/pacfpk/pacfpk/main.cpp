//Code by Fuyin
//2013/10/30

#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "png.h"
#include "zlib.h"
#include "dirent.h"
using namespace std;

#pragma pack(1)

#define FPK_VERSION 2
//#define FPK_VERSION 3

typedef struct  FPKHDR_S{
	DWORD entry_count; // flag in the high bit
} FPKHDR;

typedef struct FPKTRL_S{
	DWORD key;
	DWORD toc_offset;
} FPKTRL;

#if FPK_VERSION >= 3
typedef struct FPKENTRY1_S {
	DWORD offset;
	DWORD length;
	char  filename[128];
	DWORD unknown;
} FPKENTRY1;
#else
typedef struct FPKENTRY1_S{
	DWORD offset;
	DWORD length;
	char  filename[24];
	DWORD unknown;
} FPKENTRY1;
#endif

typedef struct FPKENTRY2_S{
	DWORD offset;
	DWORD length;
	char  filename[24];
}FPKENTRY2;

typedef struct ZLC2HDR_S{
	BYTE  signature[4]; // "ZLC2"
	DWORD original_length;
} ZLC2HDR;

typedef struct RLE0HDR_S{
	BYTE  signature[4]; // "RLE0"
	DWORD depth; // ?
	DWORD length;
	DWORD original_length;
	DWORD type;
	DWORD unknown2;
} RLE0HDR;

typedef struct FILEINFO_S{
	string filename;
	DWORD hash;
} FILEINFO;

typedef struct KGHDR_S {
	unsigned char  signature[4]; // "GCGK"
	unsigned short width;
	unsigned short height;
	unsigned long  data_length;
} KGHDR;
#pragma pack()

typedef struct _pic_data pic_data;
struct _pic_data
{
	unsigned int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	int flag;   /* 一个标志，表示是否有alpha通道 */

	unsigned char *rgba; /* 图片数组 */
};

#define PNG_BYTES_TO_CHECK 4
#define HAVE_ALPHA 1
#define NO_ALPHA 0

const DWORD KEY = 0x78D00251;

vector<string> GetDirNameList(string dir_name)
{
	vector<string> List;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(dir_name.c_str())) != NULL)
	{
		
		while ((ent = readdir(dir)) != NULL)
		{
			if (!strncmp(ent->d_name, ".", 1))
				continue;
			string filename(ent->d_name);
			string fullname = dir_name + "\\" + filename;
			List.push_back(fullname);
		}
		closedir(dir);

		return List;
	}
	else
	{
		return List;
	}
}

void zlc2(BYTE*& buff, DWORD& len)
{
	DWORD outlen = 0;
	DWORD zeros = len / 8;
	if (len % 8)
		zeros += 1;
	outlen = len + zeros + sizeof(ZLC2HDR);

	BYTE *outdata = new BYTE[outlen];

	BYTE *p = buff;
	BYTE *end = buff + len;
	BYTE *p_out = outdata + sizeof(ZLC2HDR);


	while (p < end)
	{
		*p_out++ = 0;
		for (DWORD i = 0; i < 8; i++)
		{
			if (p > end)
				break;
			*p_out++ = *p++;
		}
	}

	delete[] buff;
	ZLC2HDR zhdr;
	memcpy(&zhdr.signature, "ZLC2", 4);
	zhdr.original_length = len;
	memcpy(outdata, &zhdr, sizeof(ZLC2HDR));

	buff = outdata;
	len = outlen;
}

int read_png_file(string filepath, pic_data *out)
/* 用于解码png图片 */
{
	FILE *pic_fp;
	pic_fp = fopen(filepath.c_str(), "rb");
	if (pic_fp == NULL) /* 文件打开失败 */
		return -1;

	/* 初始化各种结构 */
	png_structp png_ptr;
	png_infop   info_ptr;
	char        buf[PNG_BYTES_TO_CHECK];
	int         temp;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);

	setjmp(png_jmpbuf(png_ptr)); // 这句很重要

	temp = fread(buf, 1, PNG_BYTES_TO_CHECK, pic_fp);
	temp = png_sig_cmp((png_const_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK);

	/*检测是否为png文件*/
	if (temp != 0) return 1;

	rewind(pic_fp);
	/*开始读文件*/
	png_init_io(png_ptr, pic_fp);
	// 读文件了
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

	int color_type, channels;

	/*获取宽度，高度，位深，颜色类型*/
	channels = png_get_channels(png_ptr, info_ptr); /*获取通道数*/


	/////////////////此段代码仅适用于 FPK Galgame, 即所有图片均为4通道//////////////////////////////////////////////////////////////////////////////////////////////////////
	if (channels < 4)
		return 1;
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	out->bit_depth = png_get_bit_depth(png_ptr, info_ptr); /* 获取位深 */
	color_type = png_get_color_type(png_ptr, info_ptr); /*颜色类型*/

	int i, j;
	int size;
	/* row_pointers里边就是rgba数据 */
	png_bytep* row_pointers;
	row_pointers = png_get_rows(png_ptr, info_ptr);
	out->width = png_get_image_width(png_ptr, info_ptr);
	out->height = png_get_image_height(png_ptr, info_ptr);

	size = out->width * out->height; /* 计算图片的总像素点数量 */

	if (channels == 4 || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{/*如果是RGB+alpha通道，或者RGB+其它字节*/
		size *= (4 * sizeof(unsigned char)); /* 每个像素点占4个字节内存 */
		out->flag = HAVE_ALPHA;    /* 标记 */
		out->rgba = (unsigned char*)malloc(size);
		if (out->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}

		temp = (4 * out->width);/* 每行有4 * out->width个字节 */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 4)
			{/* 一个字节一个字节的赋值 */
				out->rgba[i*temp + j] = row_pointers[i][j];       // red
				out->rgba[i*temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i*temp + j + 2] = row_pointers[i][j + 2];   // blue
				out->rgba[i*temp + j + 3] = row_pointers[i][j + 3];   // alpha
			}
		}
	}
	else if (channels == 3 || color_type == PNG_COLOR_TYPE_RGB)
	{/* 如果是RGB通道 */
		size *= (3 * sizeof(unsigned char)); /* 每个像素点占3个字节内存 */
		out->flag = NO_ALPHA;    /* 标记 */
		out->rgba = (unsigned char*)malloc(size);
		memset(out->rgba, 0, size);
		if (out->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}

		temp = (3 * out->width);/* 每行有3 * out->width个字节 */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 3)
			{/* 一个字节一个字节的赋值 */
				out->rgba[i*temp + j] = row_pointers[i][j];       // red
				out->rgba[i*temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i*temp + j + 2] = row_pointers[i][j + 2];   // blue
			}
		}
	}
	else return 1;

	/* 撤销数据占用的内存 */
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	return 0;
}
/*
bool if_diff(unsigned char x, unsigned char y)
{
	return (x != y);
}

vector<unsigned int> convert_channel(unsigned char*& data, unsigned int width, unsigned int height, unsigned int &data_len)
{
	vector<unsigned int> offset_table;
	vector<unsigned char> kg_data;
	kg_data.reserve(5 * width * height);

	vector<unsigned char>r, g, b, a;
	vector<unsigned char>::iterator ires;
	unsigned int x, y;
	unsigned int row_size = width * 4;
	unsigned int n = 0;

	for (y = 0; y < height; y++)
	{
		offset_table.push_back(kg_data.size());

		unsigned char *cur_row = data + y * row_size;
		for (x = 0; x < row_size; x += 4)
		{
			r.push_back(cur_row[x]);
			g.push_back(cur_row[x + 1]);
			b.push_back(cur_row[x + 2]);
			a.push_back(cur_row[x + 3]);
		}

		while (true)
		{
			if (a.size() >= 256)
			{
				ires = adjacent_find(a.begin(), a.begin() + 256, if_diff);
				if (ires != a.begin() + 256)
					n = distance(a.begin(), ires) + 1;
				else
					n = distance(a.begin(), ires);

				kg_data.push_back(a[0]);
				kg_data.push_back((unsigned char)n);
				if (a[0])
				{
					for (unsigned int i = 0; i < n; i++)
					{
						kg_data.push_back(r[i]);
						kg_data.push_back(g[i]);
						kg_data.push_back(b[i]);
					}
				}
				a.erase(a.begin(), a.begin() + n);
				r.erase(r.begin(), r.begin() + n);
				g.erase(g.begin(), g.begin() + n);
				b.erase(b.begin(), b.begin() + n);
			}
			else
			{
				ires = adjacent_find(a.begin(), a.end(), if_diff);
				if (ires != a.end())
					n = distance(a.begin(), ires) + 1;
				else
					n = distance(a.begin(), ires);

				kg_data.push_back(a[0]);
				kg_data.push_back((unsigned char)n);
				if (a[0])
				{
					for (unsigned int i = 0; i < n; i++)
					{
						kg_data.push_back(r[i]);
						kg_data.push_back(g[i]);
						kg_data.push_back(b[i]);
					}
				}
				a.erase(a.begin(), a.begin() + n);
				r.erase(r.begin(), r.begin() + n);
				g.erase(g.begin(), g.begin() + n);
				b.erase(b.begin(), b.begin() + n);
			}
			if (!a.size()) break;
		}
	}
	delete[]data;

	unsigned int out_len = kg_data.size();
	unsigned char *out_data = new unsigned char[out_len];
	memcpy(out_data, &kg_data[0], out_len); //kg_data在函数返回后自动回收？？
	data = out_data;
	data_len = out_len;

	return offset_table;
}
*/
unsigned int find_diff(unsigned char*buff, unsigned int begin, unsigned int end)
{
	unsigned int res = 0;
	unsigned int i;
	for (i = begin; i < end; i++)
	{
		if ((i + 1) == end)
		{
			return end;
		}
		if (buff[i] != buff[i + 1])
		{
			res = i;
			break;
		}
	}
	return res;
}

vector<unsigned int> convert_channel(unsigned char*& data, unsigned int width, unsigned int height, unsigned int &data_len)
{
	vector<unsigned int> offset_table;

	unsigned int row_size = width * 4;
	unsigned char *kg_data = new unsigned char[row_size * height];

	unsigned char *r = new unsigned char[width];
	unsigned char *g = new unsigned char[width];
	unsigned char *b = new unsigned char[width];
	unsigned char *a = new unsigned char[width];

	unsigned int x, y;

	unsigned int n = 0;
	unsigned int pos = 0, pix = 0, cur_front = 0;

	for (y = 0; y < height; y++)
	{
		offset_table.push_back(pos);

		unsigned char *cur_row = data + y * row_size;
		pix = 0;
		for (x = 0; x < row_size; x += 4)
		{
			r[pix] = (cur_row[x]);
			g[pix] = (cur_row[x + 1]);
			b[pix] = (cur_row[x + 2]);
			a[pix] = (cur_row[x + 3]);
			++pix;
		}
		cur_front = 0;
		while (true)
		{
			if (pix - cur_front >= 256)
			{
				n = find_diff(a, cur_front, cur_front + 256);
				if (n != cur_front + 256)
					n = n - cur_front + 1;
				else
					n -= cur_front;

				kg_data[pos++] = a[cur_front];
				kg_data[pos++] = (unsigned char)n;
				if (a[cur_front])
				{
					for (unsigned int i = 0; i < n; i++)
					{
						kg_data[pos++] = r[cur_front + i];
						kg_data[pos++] = g[cur_front + i];
						kg_data[pos++] = b[cur_front + i];
					}
				}
				cur_front += n;
			}
			else
			{
				n = find_diff(a, cur_front, pix);
				if (n != pix)
					n = n - cur_front + 1;
				else
					n -= cur_front;

				kg_data[pos++] = a[cur_front];
				kg_data[pos++] = (unsigned char)n;
				if (a[cur_front])
				{
					for (unsigned int i = 0; i < n; i++)
					{
						kg_data[pos++] = r[cur_front + i];
						kg_data[pos++] = g[cur_front + i];
						kg_data[pos++] = b[cur_front + i];
					}
				}
				cur_front += n;
			}
			if (pix == cur_front) break;
		}
	}
	delete[]data;
	delete[]r;
	delete[]g;
	delete[]b;
	delete[]a;

	unsigned int out_len = pos;
	unsigned char *out_data = new unsigned char[out_len];
	memcpy(out_data, kg_data, out_len);

	delete[]kg_data;

	data = out_data;
	data_len = out_len;

	return offset_table;
}


void png2kgcg(string filename, unsigned char*&out_buff, unsigned long &out_len)
{
	pic_data pic;
	//判断是否4通道图片
	int is_not_4ch = 0;
	is_not_4ch = read_png_file(filename, &pic);
	//如果不是则返回
	if (is_not_4ch)
	{
		printf("不支持 非4通道图片\n");
		return;
	}

	unsigned int data_len = 0;
	vector<unsigned int> off_tbl;

	off_tbl = convert_channel(pic.rgba, pic.width, pic.height, data_len);

	KGHDR kg;
	memcpy(kg.signature, "GCGK", 4);
	kg.height = (unsigned short)pic.height;
	kg.width = (unsigned short)pic.width;
	kg.data_length = data_len;

	unsigned int out_size = sizeof(KGHDR)+sizeof(unsigned int)* off_tbl.size() + data_len;
	unsigned char *out_data = new unsigned char[out_size];
	unsigned char *p = out_data;

	memcpy(p, &kg, sizeof(KGHDR));
	p += sizeof(KGHDR);
	memcpy(p, &off_tbl[0], sizeof(unsigned int)* off_tbl.size());
	p += sizeof(unsigned int)* off_tbl.size();
	memcpy(p, pic.rgba, data_len);

	out_buff = out_data;
	out_len = out_size;
	delete[]pic.rgba;
}


void obfuscate(BYTE* buff, DWORD len, DWORD key)
{
	auto* p = (DWORD*)buff;
	auto* end = (DWORD*)(buff + len);

	while (p < end) {
		*p++ ^= key;
	}
}

void WriteIndex(FILE* fout, FPKENTRY1 *entry_list, DWORD filenum)
{
	//write entry
	DWORD index_offset = ftell(fout);
	DWORD index_size = sizeof(FPKENTRY1)* filenum;
	BYTE * buff = new BYTE[index_size];
	memset(buff, 0, index_size);

	BYTE *p = buff;
	for (DWORD i = 0; i < filenum; i++)
	{
		memcpy(p, &entry_list[i], sizeof(FPKENTRY1));
		p += sizeof(FPKENTRY1);
	}
	obfuscate(buff, index_size, KEY);
	fwrite(buff, 1, index_size, fout);

	delete[] buff;

	FPKTRL trl;
	trl.key = KEY;
	trl.toc_offset = index_offset;
	fwrite(&trl, 1, sizeof(FPKTRL), fout);

}

vector<FILEINFO> ReadIndex(FILE *fidx)
{
	fseek(fidx, 0, SEEK_END);
	DWORD size = ftell(fidx);
	fseek(fidx, 0, SEEK_SET);

	DWORD filenum = size / sizeof(FPKENTRY1);
	vector<FILEINFO> idx;
	for (DWORD i = 0; i < filenum; i++)
	{
		FPKENTRY1 entry;
		fread(&entry, 1, sizeof(FPKENTRY1), fidx);
		string name = entry.filename;
		DWORD hash = entry.unknown;

		FILEINFO fi;
		fi.filename = name;
		fi.hash = hash;
		idx.push_back(fi);
	}
	
	return idx;
}

bool Package(FILE* fout, vector<FILEINFO> flist, string dirname)
{
	DWORD filenum = flist.size();
	FPKHDR hdr;
	hdr.entry_count = filenum | 0x80000000;
	fwrite(&hdr, 1, sizeof(FPKHDR), fout);

	FPKENTRY1 *entrys = new FPKENTRY1[filenum];
	//first offset
	for (DWORD i = 0; i < filenum; i++)
	{

		string fullname = dirname + "\\" + flist[i].filename;
		memset(entrys[i].filename, 0, sizeof(entrys[i].filename));
		memcpy(entrys[i].filename, flist[i].filename.c_str(), strlen(flist[i].filename.c_str()));


		DWORD size = 0;
		BYTE* data = NULL;

		FILE *fin;
		fin = fopen(fullname.c_str(), "rb");
		if (!fin)
		{
			if (!strcmp(flist[i].filename.substr(flist[i].filename.find_last_of('.'), 3).c_str(), ".kg"))
			{
				string pngname = fullname.substr(0, fullname.find_last_of('.')) + ".png";
				png2kgcg(pngname, data, size);
				goto process;
			}
			else
			{
				printf("open input_file failed\n");
				return false;
			}
		}

		//size to new a block of memory

		fseek(fin, 0, SEEK_END);
		size = ftell(fin);
		fseek(fin, 0, SEEK_SET);

		data = new BYTE[size];
		fread(data, 1, size, fin);
		fclose(fin);
		/*
		if (strcmp(flist[i].filename.substr(flist[i].filename.find_last_of('.'), 3).c_str(), ".kg"))
		{
			//cal offset and wirte data of each file
			FILE *fin;
			fin = fopen(fullname.c_str(), "rb");
			if (!fin)
			{
				printf("open input_file failed\n");
				return false;
			}

			//size to new a block of memory
			
			fseek(fin, 0, SEEK_END);
			size = ftell(fin);
			fseek(fin, 0, SEEK_SET);

			data = new BYTE[size];
			fread(data, 1, size, fin);
			fclose(fin);
		}
		else
		{
			string pngname = fullname.substr(0, fullname.find_last_of('.')) + ".png";
			png2kgcg(pngname, data, size);
		}
		*/

process:
		zlc2(data, size);

		entrys[i].offset = ftell(fout);
		entrys[i].length = size;
		entrys[i].unknown = flist[i].hash;

		fwrite(data, 1, size, fout);

		printf("packing %s -->done!\n", flist[i].filename.c_str());
		delete[]data;
	}

	WriteIndex(fout, entrys, filenum);
	delete[] entrys;

	return true;
}

int main(int argc, char* argv[])
{
	if (!(argc > 2))
	{
		printf("usage: %s <input_dir> <index_file>\n", argv[0]);
		printf("pacfpk v3.0, code by 福音\n");
		return -1;
	}
	char fpkname[128];
	strcpy(fpkname, argv[1]);
	strcat(fpkname, ".cnp");

	FILE* f;

	f = fopen(fpkname, "wb");
	if (!f)
	{
		printf("open fpk_file failed\n");
		system("pause");
		return 0;
	}

	FILE * index = fopen(argv[2], "rb");
	if (!index)
	{
		printf("Read Index Failed!\n");
		system("pause");
		return -1;
	}

	vector<FILEINFO> flist = ReadIndex(index);

	if (flist.empty())
	{
		printf("Diretory is Empty!\n");
		system("pause");
		return -1;
	}
	if (Package(f, flist, argv[1]))
	{
		printf("Package Done!\n");
		return 0;
	}
	else
	{
		printf("Package Failed!\n");
		system("pause");
		return -1;
	}
	fclose(f);

	
}