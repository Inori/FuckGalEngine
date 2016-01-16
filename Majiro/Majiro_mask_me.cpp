#include <windows.h>
#include <iostream>
#include <fstream>

using namespace std;

#define VERSION		"1.1.4"

static char *src_bmp_path, *mask_bmp_path, *delta_bmp_path;
static char *output_dir;
static int no_alpha_reverse;
static unsigned int background_color = 0xffffff, filter_color = 0xff0000;

static void create_path(const char *dir)
{
	char path[MAX_PATH];
	char *p;
	int len = strlen(dir);

	strncpy(path, dir, sizeof(path));
	p = path;
	while ((p = strchr(p, '/')))
		*p = '\\';
	if (path[len - 1] != '\\') {
		path[len] = '\\';
		path[len + 1] = 0;
	}

	p = path;
	while ((p = strchr(p, '\\'))) {
		*p = 0;
		CreateDirectory(path, NULL);
		*p++ = '\\';
	}
}

static char *path_name(const char *__path, char *name)
{
	char path[MAX_PATH];
	char *p = path;

	if (!__path)
		return NULL;

	p = path;
	strcpy(path, __path);

	while ((p = strchr(p, '/')))
		*p++ = '\\';

	char *n = NULL;
	p = path;
	while ((p = strstr(p, "\\")))
		n = p++;

	if (!n)
		n = path;
	else
		++n;

	strcpy(name, n);

	return name;
}

static void usage(void)
{
	cout << endl;
	if (GetACP() == 936) {
		cout << "Majiro mask me - Majiro图像合成器version " VERSION " By 痴汉公贼" << endl << endl;
		cout << "用法：" << endl;
		cout << "\tMajiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp -o output_dir [-a]" << endl;
		cout << "举例:" << endl << endl;
		cout << "\t1) Majiro_mask_me -s src_bmp -m mask_bmp" << endl;
		cout << "\t用于表情图与其mask图做合成。" << endl << endl;
		cout << "\t2) Majiro_mask_me -s src_bmp -d delta_bmp" << endl;
		cout << "\t用于背景/CG图与其差分图做合成。" << endl << endl;
		cout << "\t3) Majiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp" << endl;
		cout << "\t用于立绘图与其差分和mask图做合成." << endl << endl;
		cout << "\t4) Majiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp -a" << endl;
		cout << "\t与3)类似, 但不做alpha blending." << endl << endl;
	} else {
		cout << "Majiro mask me - Majiro image synthesizer version " VERSION " By 痴汉公贼" << endl << endl;
		cout << "Usage:" << endl;
		cout << "\tMajiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp -o output_dir [-a]" << endl;
		cout << "For example:" << endl << endl;
		cout << "\t1) Majiro_mask_me -s src_bmp -m mask_bmp" << endl;
		cout << "\twhich is used to composition to face image with its mask image." << endl << endl;
		cout << "\t2) Majiro_mask_me -s src_bmp -d delta_bmp" << endl;
		cout << "\twhich is used to composition to bg/cg image with its differential image." << endl << endl;
		cout << "\t3) Majiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp" << endl;
		cout << "\twhich is used to composition to character image with its mask image and differential image." << endl << endl;
		cout << "\t4) Majiro_mask_me -s src_bmp -m mask_bmp -d delta_bmp -a" << endl;
		cout << "\tsimilar with 3), but no alpha blending." << endl << endl;
	}
	exit(-1);
}

static char *simpified_chinese_string_table[] = {
	"打开src_bmp失败",
	"无法为src_bmp分配内存",
	"src_bmp不是bmp文件",
	"打开mask_bmp失败",
	"无法为mask_bmp分配内存",
	"mask_bmp不是bmp文件",
	"打开delta_bmp失败",
	"无法为delta_bmp分配内存",
	"delta_bmp不是bmp文件",
	"src_bmp应该是24位色的bmp文件",
	"mask_bmp的宽度与src_bmp不一致",
	"mask_bmp的高度与src_bmp不一致",
	"mask_bmp应该是8位色的bmp文件",
	"delta_bmp的宽度与src_bmp不一致",
	"delta_bmp的高度与src_bmp不一致",
	"delta_bmp应该是24位色的bmp文件",
	"无法创建合成后的bmp文件",
	"无法为out_bmp分配内存",
};

static char *english_string_table[] = {
	"failed to open src_bmp",
	"out of memory for src_bmp",
	"src_bmp is not a bmp file",
	"failed to open mask_bmp",
	"out of memory for mask_bmp",
	"mask_bmp is not a bmp file",
	"failed to open delta_bmp",
	"out of memory for delta_bmp",
	"delta_bmp is not a bmp file",
	"src_bmp shoule be 24 bpp",
	"the width of mask_bmp is different with src_bmp",
	"the height of mask_bmp is different with src_bmp",
	"mask_bmp shoule be 8 bpp",
	"the width of delta_bmp is different with src_bmp",
	"the height of delta_bmp is different with src_bmp",
	"delta_bmp shoule be 24 bpp",
	"failed to create output bmp file",
	"out of memory for out_bmp",
};

static print_msg(unsigned int id)
{
	const char *msg;

	if (GetACP() == 936)
		msg = simpified_chinese_string_table[id];
	else
		msg = english_string_table[id];
	cout << msg << endl;
}

static void print_err_msg(unsigned int id)
{
	const char *msg;

	if (GetACP() == 936)
		msg = simpified_chinese_string_table[id];
	else
		msg = english_string_table[id];
	cerr << msg << endl;
}

static void die(unsigned int id)
{
	print_err_msg(id);
	exit(-1);
}

static int do_alpha_blending = 1;

static void parse_cmd(int argc, char *argv[])
{
	int i;

	for (i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], "-s")) {
			src_bmp_path = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "-m")) {
			mask_bmp_path = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "-d")) {
			delta_bmp_path = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "-o")) {
			output_dir = argv[++i];
			continue;
		}
		if (!strcmp(argv[i], "-r")) {
			no_alpha_reverse = 1;
			continue;
		}
		if (!strcmp(argv[i], "-f")) {
			const char *f_color = argv[++i];
			if (!f_color)
				usage();
			filter_color = strtol(f_color, NULL, 0);
			continue;
		}
		if (!strcmp(argv[i], "-b")) {
			const char *bg_color = argv[++i];
			if (!bg_color)
				usage();
			background_color = strtol(bg_color, NULL, 0);
			continue;
		}
		if (!strcmp(argv[i], "-a")) {
			do_alpha_blending = 0;
			continue;
		}			
	}

	if (!src_bmp_path)
		usage();

	if (delta_bmp_path && !strcmp(src_bmp_path, delta_bmp_path))
		delta_bmp_path = NULL;

	if (!mask_bmp_path && !delta_bmp_path)
		usage();

	if (!output_dir)
		output_dir = "output_dir";
}

int main(int argc, char *argv[])
{
	ifstream src_bmp, mask_bmp, delta_bmp;
	ofstream out_bmp;
	char *src_bmp_data, *mask_bmp_data, *delta_bmp_data, *out_bmp_data;
	int out_bmp_size, out_bmp_step;

	parse_cmd(argc, argv);

	src_bmp.open(src_bmp_path, ifstream::binary);
	if (!src_bmp)
		die(0);
 
	src_bmp.seekg(0, ios::end);
	unsigned int size = out_bmp_size = src_bmp.tellg();
	src_bmp_data = new char[size + 3];
	if (!src_bmp_data)
		die(1);

	src_bmp.seekg(0, ios::beg);
	src_bmp.read(src_bmp_data, size);
	src_bmp.close();
	if (src_bmp_data[0] != 'B' || src_bmp_data[1] != 'M')
		die(2);

	if (mask_bmp_path) {
		mask_bmp.open(mask_bmp_path, ifstream::binary);
		if (!mask_bmp)
			die(3);
 
		mask_bmp.seekg(0, ios::end);
		size = mask_bmp.tellg();
		mask_bmp_data = new char[size + 3];
		if (!mask_bmp_data)
			die(4);

		mask_bmp.seekg(0, ios::beg);
		mask_bmp.read(mask_bmp_data, size);
		mask_bmp.close();
		if (mask_bmp_data[0] != 'B' || mask_bmp_data[1] != 'M')
			die(5);
	}

	if (delta_bmp_path) {
		delta_bmp.open(delta_bmp_path, ifstream::binary);
		if (!delta_bmp)
			die(6);
 
		delta_bmp.seekg(0, ios::end);
		size = delta_bmp.tellg();
		delta_bmp_data = new char[size + 3];
		if (!delta_bmp_data)
			die(7);

		delta_bmp.seekg(0, ios::beg);
		delta_bmp.read(delta_bmp_data, size);
		delta_bmp.close();	
		if (delta_bmp_data[0] != 'B' || delta_bmp_data[1] != 'M')
			die(8);
	}

	BITMAPINFOHEADER *bmiHeader = (BITMAPINFOHEADER *)((BITMAPFILEHEADER *)src_bmp_data + 1);
	if (bmiHeader->biBitCount != 24)
		die(9);

	int width = bmiHeader->biWidth;
	int height = bmiHeader->biHeight;

	if (mask_bmp_path) {
		bmiHeader = (BITMAPINFOHEADER *)((BITMAPFILEHEADER *)mask_bmp_data + 1);
		if (bmiHeader->biWidth != width)
			die(10);

		if (bmiHeader->biHeight != height)
			die(11);

		if (bmiHeader->biBitCount != 8)
			die(12);
		
		out_bmp_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 4;
		out_bmp_data = new char[out_bmp_size];
		if (!out_bmp_data)
			die(17);
		memcpy(out_bmp_data, src_bmp_data, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		((BITMAPINFOHEADER *)((BITMAPFILEHEADER *)out_bmp_data + 1))->biBitCount = 32;
		out_bmp_step = 4;
	} else {
		out_bmp_step = 3;
		out_bmp_data = delta_bmp_data;
	}

	if (delta_bmp_path) {
		bmiHeader = (BITMAPINFOHEADER *)((BITMAPFILEHEADER *)delta_bmp_data + 1);
		if (bmiHeader->biWidth != width)
			die(13);

		if (bmiHeader->biHeight != height)
			die(14);

		if (bmiHeader->biBitCount != 24)
			die(15);		
	}

	int align = ((width * 3 + 3) & ~3) - width * 3;
	int dst_align = out_bmp_step == 3 ? align : 0;
	if (delta_bmp_path) {
		char *delta = delta_bmp_data + 
			sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		char *dst = out_bmp_data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		char *src = src_bmp_data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {				
				if ((*(int *)delta & 0xffffff) == filter_color)
					*(int *)dst = (dst[3] << 24) | (*(int *)src & 0xffffff);
					//*(int *)dst = *(int *)src & 0xffffff;
				else
					*(int *)dst = *(int *)delta;
				src += 3;
				dst += out_bmp_step;
				delta += 3;
			}
			src += align;
			delta += align;
			dst += dst_align;
		}
	}

	if (mask_bmp_path) {
		int *palette = (int *)(mask_bmp_data + 
			sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		unsigned char *mask = (unsigned char *)palette + 1024;

		char *src;
		if (delta_bmp_path)
			src = out_bmp_data;
		else
			src = src_bmp_data;
		src += sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		char *dst = out_bmp_data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		int align_mask = ((width + 3) & ~3) - width;
		unsigned char *b_b = (unsigned char *)&background_color;
		unsigned char *b_g = b_b + 1;
		unsigned char *b_r = b_g + 1;

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {				
				unsigned int rgb = *(int *)src;
				unsigned int alpha = !no_alpha_reverse ? ~palette[*mask++] : palette[*mask++];

				if (do_alpha_blending) {					
					unsigned char *b = (unsigned char *)&rgb;
					unsigned char *g = b + 1;
					unsigned char *r = g + 1;
					unsigned char *a_b = (unsigned char *)&alpha;
					unsigned char *a_g = a_b + 1;
					unsigned char *a_r = a_g + 1;
					*b = *b * *a_b / 255U + *b_b * (unsigned char)~*a_b / 255U;
					*g = *g * *a_g / 255U + *b_g * (unsigned char)~*a_b / 255U;
					*r = *r * *a_r / 255U + *b_r * (unsigned char)~*a_g / 255U;
				}
				*(int *)dst = rgb | ((alpha & 0xff) << 24);
				src += out_bmp_step;
				dst += out_bmp_step;
			}
			src += dst_align;
			dst += dst_align;
			mask += align_mask;
		}
#if 0
		if (out_bmp_step == 4) {
			mask = (unsigned char *)palette + 1024;
			dst = out_bmp_data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 3;

			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					unsigned int alpha = ~palette[*mask++];
					*dst = alpha;
					dst += 4;
				}
				mask += align_mask;
			}
		}
#endif
	}

	create_path(output_dir);

	char output_name[MAX_PATH];
	char src_bmp_name[MAX_PATH];
	char delta_bmp_name[MAX_PATH];

	path_name(src_bmp_path, src_bmp_name);
	path_name(delta_bmp_path, delta_bmp_name);

	sprintf(output_name, "%s\\%s", output_dir, 
		delta_bmp_path ? delta_bmp_name : src_bmp_name);

	out_bmp.open(output_name, ofstream::binary | ofstream::trunc);
	if (!out_bmp)
		die(16);

	out_bmp.write(out_bmp_data, out_bmp_size);
	out_bmp.close();
	cout << "Done!" << endl;

	return 0;
}
