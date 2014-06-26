#include "stdafx.h"
#include "drawtext.h"


////////////////////////////////FreeType/////////////////////////////////////////


FreeType::FreeType()
{
}

FreeType::FreeType(string fontname, int fontsize)
{
	SetFont(fontname, fontsize);
}

bool FreeType::SetFont(string fontname, int fontsize)
{
	HANDLE hfile = CreateFile(fontname.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		MessageBoxA(NULL, "CreateFile Error!", "Error", MB_OK);
		return false;
	}

	DWORD filesize = GetFileSize(hfile, NULL);
	DWORD sizeread = 0;
	fontdata = new BYTE[filesize];
	ReadFile(hfile, (LPVOID)fontdata, filesize, &sizeread, NULL);
	CloseHandle(hfile);

	FT_Error error;
	//初始化FreeType
	error = FT_Init_FreeType(&library);
	if (error)
	{
		MessageBoxA(NULL, "Init_FreeType Error!", "Error", MB_OK);
		return false;
	}

	error = FT_New_Memory_Face(library, fontdata, filesize, 0, &face);
	if (error)
	{
		MessageBoxA(NULL, "New_Memory_Face Error!", "Error", MB_OK);
		return false;
	}

	error = FT_Set_Char_Size(face, fontsize << 6, fontsize << 6, 90, 90);
	if (error)
	{
		MessageBoxA(NULL, "Set_Char_Size Error!", "Error", MB_OK);
		return false;
	}
	
	return true;
}

charBitmap FreeType::GetCharBitmap(wchar_t wchar)
{
	charBitmap cbmp;

	FT_GlyphSlot slot = face->glyph;
	FT_Bitmap bmp;
	FT_Error error;

	//加载Glyph并转换成256色抗锯齿位图
	error = FT_Load_Char(face, wchar, FT_LOAD_RENDER);

	if (error)
	{
		MessageBox(NULL, "Load_Char Error", "Error", MB_OK);
	}

	bmp = slot->bitmap;

	cbmp.bmp_width = bmp.width;
	cbmp.bmp_height = bmp.rows;
	cbmp.bearingX = slot->bitmap_left;
	cbmp.bearingY = slot->bitmap_top;
	

	//以slot->advance增加笔位置，slot->advance符合字形的步进宽度（也就是通常所说的走格(escapement)）。
	//步进矢量以象素的1/64为单位表示，并且在每一次迭代中删减为整数象素
	cbmp.Advance = slot->advance.x >> 6;
	//这部分内存不需要手动释放，FT在下次装载字形槽的时候会抹掉上次的数据
	cbmp.bmpBuffer = bmp.buffer;

	return cbmp;
}


FreeType::~FreeType()
{
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	delete[]fontdata;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//所需内存在外部申请，外部释放
void Cvt8BitTo32Bit(BYTE* dst, BYTE*src, DWORD nWidth, DWORD nHeight)
{
	//四通道
	static const int pix_size = 4;

	BYTE *srcline;
	BYTE *dstline;
	BYTE pix;

	for (int y = 0; y < nHeight; y++)
	{
		srcline = &src[y * nWidth];
		//srcline = &src[(nHeight - y - 1)*nWidth]; //垂直翻转图像
		dstline = &dst[y * nWidth * pix_size];
		for (int x = 0; x < nWidth; x++)
		{
			pix = srcline[x];
			dstline[x*pix_size] = pix;
			dstline[x*pix_size + 1] = pix;
			dstline[x*pix_size + 2] = pix;
			dstline[x*pix_size + 3] = pix;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GdipDrawer::GdipDrawer()
{
}

GdipDrawer::GdipDrawer(string fontname, int fontsize)
{
	InitDrawer( fontname, fontsize);
}

bool GdipDrawer::InitDrawer(string fontname, int fontsize)
{

	// 初始化 GDI+
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// 初始化FreeType
	if (!ft.SetFont(fontname, fontsize))
		return false;
	
	has_color = false;
	has_glow = false;
	has_shadow = false;
	return true;
}


void GdipDrawer::SetTextColor(TextColor color)
{
	text_colorMatrix =
	{ 
		(float)color.b / 255.0,	0.0f,					0.0f,					0.0f,					0.0f,
		0.0f,					(float)color.g / 255.0,	0.0f,					0.0f,					0.0f,
		0.0f,					0.0f,					(float)color.r / 255.0, 0.0f,					0.0f,
		0.0f,					0.0f,					0.0f,					(float)color.a / 255.0, 0.0f,
		0.0f,					0.0f,					0.0f,					0.0f,					1.0f 
	};

	has_color = true;
}


void GdipDrawer::ApplyEffect(TextEffect effect, TextColor color, float _d_pixwidth, float _radius)
{
	//填充效果颜色矩阵
	effect_colorMatrix =
	{
		(float)color.b / 255.0, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, (float)color.g / 255.0, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, (float)color.r / 255.0, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, (float)color.a / 255.0, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 1.0f
	};

	d_pixwidth = _d_pixwidth;
	radius = _radius;

	if (effect == Shadow)
	{
		has_shadow = true;

	}
	else if (effect == Glow)
	{
		has_glow = true;
	}
	else
	{
		MessageBox(NULL, "Effect not support!", "Error", MB_OK);
		return;
	}
	
}


//这里的lineHeight应该也可以自己算出来而不是指定，有待完善
//lineHeight：单行文字高度
void  GdipDrawer::DrawString(HDC hdc, wstring str, int xdest, int ydest, int lineHeight)
{
	//绘制起点
	int pen_x = xdest;
	int pen_y = ydest;

	//这段代码的效率值得怀疑……Orz
	for each (wchar_t wc in str)
	{
		//换行
		if (wc == L'\n')
		{
			pen_y += lineHeight;
			pen_x = xdest;
			continue;
		}

		cbmp = ft.GetCharBitmap(wc);

		DWORD nWidth = cbmp.bmp_width;
		DWORD nHeight = cbmp.bmp_height;

		//这里进行了频繁的申请释放内存，有待优化
		BYTE *src = cbmp.bmpBuffer;
		BYTE *dst = new BYTE[nWidth*nHeight*4];
		Cvt8BitTo32Bit(dst, src, nWidth, nHeight);

		//获取bmp， PixelFormat32bppPARGB->Alpha混合
		Gdiplus::Bitmap bitmap(nWidth, nHeight, 4*nWidth, PixelFormat32bppPARGB, dst);

		Graphics graphics(hdc);

		//显示背景效果图层
		float dpixwidth = 0.0;
		if (has_shadow)
		{
			ImageAttributes iAtt;
			iAtt.SetColorMatrix(&effect_colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
			graphics.DrawImage(&bitmap,
				Rect(pen_x + d_pixwidth, lineHeight - cbmp.bearingY + pen_y + d_pixwidth, nWidth + d_pixwidth, nHeight + d_pixwidth),
				0, 0, nWidth, nHeight,
				UnitPixel, &iAtt);
		}
		else if (has_glow)
		{
			ImageAttributes iAtt;
			iAtt.SetColorMatrix(&effect_colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

			Matrix myMatrix(1.3f, 0.0f, 0.0f, 1.3f, pen_x, lineHeight - cbmp.bearingY + pen_y);
			RectF srcRect(0.0f, 0.0f, nWidth, nHeight);

			BlurParams myBlurParams;
			myBlurParams.expandEdge = TRUE;
			myBlurParams.radius = radius;

			Blur myBlur;
			myBlur.SetParameters(&myBlurParams);
			
			graphics.DrawImage(&bitmap, &srcRect, &myMatrix, &myBlur, &iAtt, UnitPixel);

			dpixwidth = d_pixwidth;
		}

		//显示前景文字图层
		//设置字体颜色
		if (has_color)
		{
			ImageAttributes imageAtt;
			imageAtt.SetColorMatrix(&text_colorMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
			graphics.DrawImage(&bitmap, 
				Rect(pen_x + dpixwidth, lineHeight - cbmp.bearingY + pen_y + dpixwidth, nWidth, nHeight),
				0, 0, nWidth, nHeight, 
				UnitPixel, &imageAtt);
		}
		else
		{
			CachedBitmap  cBitmap(&bitmap, &graphics);
			//绘制CachedBitmap更快
			graphics.DrawCachedBitmap(&cBitmap, pen_x + dpixwidth, lineHeight - cbmp.bearingY + pen_y + dpixwidth);
		}
		


		//走格
		pen_x += cbmp.Advance;

		//重定义起点
		//pen_x += cbmp.bearingX;
		//pen_y += cbmp.bearingY;

		
		delete[]dst;
		memset(&cbmp, 0, sizeof(charBitmap));
	}

	
}

GdipDrawer::~GdipDrawer()
{
	GdiplusShutdown(gdiplusToken);
}