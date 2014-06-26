/******************************************************************************\
*  Copyright (C) 2014 Fuyin
*  ALL RIGHTS RESERVED
*  Author: Fuyin
*  Description: 定义绘制文本需要的类
\******************************************************************************/

#ifndef DRAWTEXT_H
#define DRAWTEXT_H

#include <Windows.h>
#include <string>
using namespace std;

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_STROKER_H

//定义高版本GDI+, 以使用一些渲染函数
#define GDIPVER 0x0110

#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

//将8位灰度图转换成32位RGBA图
void Cvt8BitTo32Bit(BYTE* dst, BYTE*src, DWORD nWidth, DWORD nHeight);


//FreeType取出的都是8位灰度图像
typedef struct _charBitmap
{
	int bmp_width;  //位图宽
	int bmp_height; //位图高
	int bearingX;   //用于水平文本排列，这是从当前光标位置到字形图像最左边的边界的水平距离
	int bearingY;   //用于水平文本排列，这是从当前光标位置（位于基线）到字形图像最上边的边界的水平距离。 
	int Advance;    //用于水平文本排列，当字形作为字符串的一部分被绘制时，这用来增加笔位置的水平距离
	BYTE* bmpBuffer; //象素数据
} charBitmap;

class FreeType
{
public:
	FreeType();
	FreeType(string fontname, int fontsize);

	//设置并初始化字体
	bool SetFont(string fontname, int fontsize);
	//获取字模图像和信息
	charBitmap GetCharBitmap(wchar_t wchar);

	~FreeType();

private:
	FT_Library library;
	FT_Face face;

	//字体缓存，程序退出(析构)前不可释放！
	BYTE* fontdata;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////
//方便初始化
typedef struct _TextColor
{
	_TextColor(){}
	_TextColor(BYTE bi, BYTE gi, BYTE ri, BYTE ai = 255)
	{
		b = bi;
		g = gi;
		r = ri;
		a = ai;
	}

	struct
	{
		BYTE b, g, r, a;
	};
}TextColor;

enum TextEffect
{
	Glow = 0,
	Shadow
};


class Drawer
{
public:

	virtual bool InitDrawer(string fontname, int fontsize) = 0;
	//virtual void SetTextColor(TextColor color) = 0;

	//effect: 效果类型
	//color: 描边、阴影和外发光的颜色
	//d_pixwidth: 描边、阴影和外发光时外字边缘减内字边缘象素宽度
	//radius: 外发光半径
	//virtual void ApplyEffect(TextEffect effect, TextColor color, int d_pixwidth, float radius = 0.0) = 0;
	virtual void DrawString(HDC hdc, wstring str, int xdest, int ydest, int lineHeight) = 0;
};


class GdipDrawer
{
public:
	GdipDrawer();
	GdipDrawer(string fontname, int fontsize);

	bool InitDrawer(string fontname, int fontsize);

	void SetTextColor(TextColor color);

	//effect: 效果类型
	//color: 描边、阴影和外发光的颜色
	//d_pixwidth: 用于描边、阴影和外发光时外字边缘减内字边缘象素宽度
	//radius: 外发光半径
	void ApplyEffect(TextEffect effect, TextColor color, float _d_pixwidth = 2, float _radius = 0.0);

	void DrawString(HDC hdc, wstring str, int xdest, int ydest, int lineHeight);

	~GdipDrawer();

private:
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	FreeType ft;
	charBitmap cbmp;

	bool has_color;
	bool has_glow;
	bool has_shadow;

	int d_pixwidth;
	int radius;
	ColorMatrix text_colorMatrix;
	ColorMatrix effect_colorMatrix;
};




#endif