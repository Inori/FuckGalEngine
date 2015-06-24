/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.7
 */
/*
 *                      ATOK Library Sample
 *
 *                         Version 0.10
 *                           Shift-JIS
 *
 *      Copyright (C) 2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.10
 */

#ifndef _TIM2_H_INCLUDED
#define _TIM2_H_INCLUDED


#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif
// サイズ統一のために型定義
typedef unsigned char  TIM2_UCHAR8;
typedef unsigned int   TIM2_UINT32;
typedef unsigned short TIM2_UINT16;

#ifdef R5900
typedef unsigned long  TIM2_UINT64;     // PS2
#else  // R5900
#ifdef WIN32
typedef unsigned __int64 TIM2_UINT64;   // Win32
#else  // WIN32
typedef unsigned long long TIM2_UINT64; // GNU-C
#endif // WIN32
#endif // R5900


// ピクチャヘッダのClutType,ImageTypeで使用される定数の定義
enum TIM2_gattr_type {
	TIM2_NONE = 0,  // CLUTデータなしのときにClutTypeで使用
	TIM2_RGB16,     // 16bitカラー(ClutType,ImageType両方で使用)
	TIM2_RGB24,     // 24bitカラー(ImageTypeでのみ使用)
	TIM2_RGB32,     // 32bitカラー(ClutType,ImageType両方で使用)
	TIM2_IDTEX4,    // 16色テクスチャ(ImageTypeでのみ使用)
	TIM2_IDTEX8     // 16(?256)色テクスチャ(ImageTypeでのみ使用)
};

// TIM2ファイルヘッダ
typedef struct {
	TIM2_UCHAR8 FileId[4];      // ファイルID('T','I','M','2'または'C','L','T','2')
	TIM2_UCHAR8 FormatVersion;  // ファイルフォーマットバージョン
	TIM2_UCHAR8 FormatId;       // フォーマットID
	TIM2_UINT16 Pictures;       // ピクチャデータ個数
	TIM2_UCHAR8 pad[8];         // 16バイトアラインメント
} TIM2_FILEHEADER;


// TIM2ピクチャヘッダ
typedef struct {
	TIM2_UINT32 TotalSize;      // ピクチャデータ全体のバイトサイズ
	TIM2_UINT32 ClutSize;       // CLUTデータ部のバイトサイズ
	TIM2_UINT32 ImageSize;      // イメージデータ部のバイトサイズ
	TIM2_UINT16 HeaderSize;     // 画像ヘッダサイズ
	TIM2_UINT16 ClutColors;     // クラットサイズ（クラット部の色数）
	TIM2_UCHAR8 PictFormat;     // 画像ID
	TIM2_UCHAR8 MipMapTextures; // MIPMAP枚数
	TIM2_UCHAR8 ClutType;       // クラット部種別
	TIM2_UCHAR8 ImageType;      // イメージ部種別
	TIM2_UINT16 ImageWidth;     // 画像横サイズ(ビット数ではありません)
	TIM2_UINT16 ImageHeight;    // 画像縦サイズ(ビット数ではありません)

	TIM2_UINT64 GsTex0;         // TEX0
	TIM2_UINT64 GsTex1;         // TEX1
	TIM2_UINT32 GsTexaFbaPabe;  // TEXA, FBA, PABEの混合ビット
	TIM2_UINT32 GsTexClut;      // TEXCLUT(下位32bitそのまま)
} TIM2_PICTUREHEADER;


// TIM2ミップマップヘッダ
typedef struct {
	TIM2_UINT64 GsMiptbp1;      // MIPTBP1(64ビットそのまま)
	TIM2_UINT64 GsMiptbp2;      // MIPTBP2(64ビットそのまま)
	TIM2_UINT32 MMImageSize[0]; // MIPMAP[?]枚目の画像バイトサイズ
} TIM2_MIPMAPHEADER;


// TIM2拡張ヘッダ
typedef struct {
	TIM2_UCHAR8 ExHeaderId[4];  // 拡張コメントヘッダID('e','X','t','\x00')
	TIM2_UINT32 UserSpaceSize;  // ユーザースペースサイズ
	TIM2_UINT32 UserDataSize;   // ユーザーデータのサイズ
	TIM2_UINT32 Reserved;       // リザーブ
} TIM2_EXHEADER;


// TIM2ファイルローダのプロトタイプ宣言
int   Tim2CheckFileHeaer(void *p);  // TIM2ファイルかどうかチェック
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *p, int imgno); // 指定枚数目のピクチャヘッダを取得
int   Tim2IsClut2(TIM2_PICTUREHEADER *ph);                    // TIM2,CLUT2の判別

int   Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph,
                   int mipmap, int *pWidth, int *pHeight);  // MIPMAPレベルごとのピクチャサイズを取得

TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize);
                                                            // MIPMAPヘッダの開始アドレス,サイズを取得

void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize); // ユーザースペースの開始アドレス,サイズを取得
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize);  // ユーザーデータの開始アドレス,サイズを取得
char *Tim2GetComment(TIM2_PICTUREHEADER *ph);               // コメント文字列の開始アドレスを取得
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap);     // 指定レベルのイメージデータの開始アドレスを取得
void *Tim2GetClut(TIM2_PICTUREHEADER *ph);                  // CLUTデータの開始アドレスを取得

unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph,
                    int clut, int no);                        // 指定CLUTセット,インデクスのCLUTカラーデータを取得
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph,
                    int clut, int no, unsigned int newcolor); // 指定CLUTセット,インデクスのCLUTカラーデータを設定

unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph,
                    int mipmap, int x, int y);        // 指定MIPMAPレベルの指定テクセル座標からテクセルデータを取得
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph,
                    int mipmap, int x, int y, unsigned int newtexel); // 指定レベルの指定座標へテクセルデータを設定

unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph,
                    int mipmap, int clut, int x, int y); // 指定MIPMAPレベルから、指定CLUTを使ってテクセルカラーを取得


#ifdef R5900

unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph,
                    unsigned int tbp, unsigned int cbp);   // イメージデータ,CLUTデータをGSローカルメモリへ転送
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp); // イメージデータをGSローカルメモリへ転送
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp);  // CLUTデータをGSローカルメモリへ転送

int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1,
                    char *pszFname);    // スナップショット画像をTIM2ファイルでホストHDDに書き出し

#endif // R5900

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif

#endif /* _TIM2_H_INCLUDED */
