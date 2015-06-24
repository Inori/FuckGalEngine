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
#include <stdio.h>

// TIM2ファイルヘッダの構造を設定

#ifdef R5900

// PS2環境のとき
#include <eekernel.h>
#include <sifdev.h>
#include <libgraph.h>
#include "tim2.h"

// プロトタイプ宣言
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int sx, int sy, u_long128 *pImage);
static int  Tim2CalcBufWidth(int psm, int w);
static int  Tim2CalcBufSize(int psm, int w, int h);
static int  Tim2GetLog2(int n);

#else	// R5900

// 非PS2環境のとき

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4200)
#endif	// WIN32
#include "tim2.h"
#ifdef WIN32
#pragma warning(pop)
#endif	// WIN32

#endif	// R5900



// TIM2ファイルのファイルヘッダをチェックする
// 引数
// pTim2    TIM2形式のデータの先頭アドレス
// 返り値
//          0のときエラー
//          1のとき正常終了(TIM2)
//          2のとき正常終了(CLUT2)
int Tim2CheckFileHeaer(void *pTim2)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	int i;

	// TIM2シグネチャをチェック
	if(pFileHdr->FileId[0]=='T' || pFileHdr->FileId[1]=='I' || pFileHdr->FileId[2]=='M' || pFileHdr->FileId[3]=='2') {
		// TIM2だったとき
		i = 1;
	} else if(pFileHdr->FileId[0]=='C' || pFileHdr->FileId[1]=='L' || pFileHdr->FileId[2]=='T' || pFileHdr->FileId[3]=='2') {
		// CLUT2だったとき
		i = 2;
	} else {
		// イリーガルな識別文字だったとき
		printf("Tim2CheckFileHeaer: TIM2 is broken %02X,%02X,%02X,%02X\n",
					pFileHdr->FileId[0], pFileHdr->FileId[1], pFileHdr->FileId[2], pFileHdr->FileId[3]);
		return(0);
	}

	// TIM2ファイルフォーマットバージョン,フォーマットIDをチェック
	if(!(pFileHdr->FormatVersion==0x03 ||
	    (pFileHdr->FormatVersion==0x04 && (pFileHdr->FormatId==0x00 || pFileHdr->FormatId==0x01)))) {
		printf("Tim2CheckFileHeaer: TIM2 is broken (2)\n");
		return(0);
	}
	return(i);
}



// 指定された番号のピクチャヘッダを得る
// 引数
// pTim2    TIM2形式のデータの先頭アドレス
// imgno    何番目のピクチャヘッダを参照するか指定
// 返り値
//          ピクチャヘッダへのポインタ
TIM2_PICTUREHEADER *Tim2GetPictureHeader(void *pTim2, int imgno)
{
	TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)pTim2;
	TIM2_PICTUREHEADER *pPictHdr;
	int i;

	// ピクチャ番号をチェック
	if(imgno>=pFileHdr->Pictures) {
		printf("Tim2GetPictureHeader: Illegal image no.(%d)\n", imgno);
		return(NULL);
	}

	if(pFileHdr->FormatId==0x00) {
		// フォーマットIDが0x00のとき、16バイトアラインメント
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + sizeof(TIM2_FILEHEADER));
	} else {
		// フォーマットIDが0x01のとき、128バイトアラインメント
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pTim2 + 0x80);
	}

	// 指定されたピクチャ番号までスキップ
	for(i=0; i<imgno; i++) {
		pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
	}
	return(pPictHdr);
}


// ピクチャデータがCLUT2データであるかどうか判別
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// 戻り値
//          0のときTIM2
//          1のときCLUT2
int Tim2IsClut2(TIM2_PICTUREHEADER *ph)
{
	// ピクチャヘッダのMipMapTexturesメンバを判別
	if(ph->MipMapTextures==0) {
		// テクスチャ枚数が0のとき、CLUT2データ
		return(1);
	} else {
		// テクスチャ枚数が1枚以上のとき、TIM2データ
		return(0);
	}
}


// MIPMAPレベルごとのテクスチャサイズを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// mipmap:  MIPMAPテクスチャレベル(放缩级别：0-原始大小)
// pWidth:  Xサイズを受け取るためのint型変数へポインタ
// pHeight: Yサイズを受け取るためのint型変数へポインタ
// 戻り値
//          MIPMAPテクスチャのサイズ
int Tim2GetMipMapPictureSize(TIM2_PICTUREHEADER *ph, int mipmap, int *pWidth, int *pHeight)
{
	int w, h, n;
	w = ph->ImageWidth>>mipmap;
	h = ph->ImageHeight>>mipmap;
	if(pWidth) {
		*pWidth  = w;
	}
	if(pHeight) {
		*pHeight = h;
	}

	n = w * h;
	switch(ph->ImageType) {
		case TIM2_RGB16:	n *= 2;		break;
		case TIM2_RGB24:	n *= 3;		break;
		case TIM2_RGB32:	n *= 4;		break;
		case TIM2_IDTEX4:	n /= 2;		break;
		case TIM2_IDTEX8:				break;
	}

	// MIPMAPテクスチャサイズはファイルヘッダのFormatIdの指定にかかわらず、
	// 常に16バイトアラインメント境界で整列される
	n = (n + 15) & ~15;
	return(n);
}


// MIPMAPヘッダのアドレス,サイズを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// pSize:   MIPMAPヘッダのサイズを受け取るためのint型変数へポインタ
//          サイズが必要のないときはNULLに
// 戻り値
//          NULLのときMIPMAPヘッダなし
//          NULLでないとき、MIPMAPヘッダの先頭アドレス
TIM2_MIPMAPHEADER *Tim2GetMipMapHeader(TIM2_PICTUREHEADER *ph, int *pSize)
{
	TIM2_MIPMAPHEADER *pMmHdr;

	static const char mmsize[] = {
		 0,  // テクスチャ0枚(CLUT2データのとき)
		 0,  // LV0テクスチャのみ(MIPMAPヘッダなし)
		32,  // LV1 MipMapまで
		32,  // LV2 MipMapまで
		32,  // LV3 MipMapまで
		48,  // LV4 MipMapまで
		48,  // LV5 MipMapまで
		48   // LV6 MipMapまで
	};

	if(ph->MipMapTextures>1) {
		pMmHdr = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	} else {
		pMmHdr = NULL;
	}

	if(pSize) {
		// 拡張ヘッダがなかった場合、
		*pSize = mmsize[ph->MipMapTextures];
	}
	return(pMmHdr);
}


// ユーザースペースのアドレス,サイズを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// pSize:   ユーザースペースのサイズを受け取るためのint型変数へポインタ
//          サイズが必要のないときはNULLに
// 戻り値
//          NULLのときユーザースペースなし
//          NULLでないとき、ユーザースペースの先頭アドレス
void *Tim2GetUserSpace(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;

	static const char mmsize[] = {
		sizeof(TIM2_PICTUREHEADER)     ,	// テクスチャ0枚(CLUT2データのとき)
		sizeof(TIM2_PICTUREHEADER)     ,	// LV0テクスチャのみ(MIPMAPヘッダなし)
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV1 MipMapまで
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV2 MipMapまで
		sizeof(TIM2_PICTUREHEADER) + 32,	// LV3 MipMapまで
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV4 MipMapまで
		sizeof(TIM2_PICTUREHEADER) + 48,	// LV5 MipMapまで
		sizeof(TIM2_PICTUREHEADER) + 48 	// LV6 MipMapまで
	};

	// ヘッダサイズを調べる
	if(ph->HeaderSize==mmsize[ph->MipMapTextures]) {
		// ユーザースペースが存在しないとき
		if(pSize) *pSize = 0;
		return(NULL);
	}

	// ユーザースペースが存在するとき
	pUserSpace = (void *)((char *)ph + mmsize[ph->MipMapTextures]);
	if(pSize) {
		// ユーザースペースのサイズを計算
		// 拡張ヘッダがある場合は、そちらからトータルサイズを取得
		TIM2_EXHEADER *pExHdr;

		pExHdr = (TIM2_EXHEADER *)pUserSpace;
		if(pExHdr->ExHeaderId[0]!='e' ||
			pExHdr->ExHeaderId[1]!='X' ||
			pExHdr->ExHeaderId[2]!='t' ||
			pExHdr->ExHeaderId[3]!=0x00) {

			// 拡張ヘッダがなかった場合、
			*pSize = (ph->HeaderSize - mmsize[ph->MipMapTextures]);
		} else {
			// 拡張ヘッダがあった場合
			*pSize = pExHdr->UserSpaceSize;
		}
	}
	return(pUserSpace);
}


// ユーザーデータのアドレス,サイズを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// pSize:   ユーザーデータのサイズを受け取るためのint型変数へポインタ
//          サイズが必要のないときはNULLに
// 戻り値
//          NULLのときユーザーデータなし
//          NULLでないとき、ユーザーデータの先頭アドレス
void *Tim2GetUserData(TIM2_PICTUREHEADER *ph, int *pSize)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, pSize);
	if(pUserSpace==NULL) {
		// ユーザースペースが存在しなかったとき
		return(NULL);
	}

	// ユーザースペースに拡張ヘッダがあるかどうかチェック
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// 拡張ヘッダが見つからなかった場合
		return(pUserSpace);
	}

	// 拡張ヘッダが見つかった場合
	if(pSize) {
		// ユーザーデータ部分のサイズを返す
		*pSize = pExHdr->UserDataSize;
	}
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER));
}


// ユーザースペースに格納されたコメント文字列の先頭アドレスを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// 戻り値
//          NULLのときコメント文字列なし
//          NULLでないとき、コメント文字列(ASCIZ)の先頭アドレス
char *Tim2GetComment(TIM2_PICTUREHEADER *ph)
{
	void *pUserSpace;
	TIM2_EXHEADER *pExHdr;

	pUserSpace = Tim2GetUserSpace(ph, NULL);
	if(pUserSpace==NULL) {
		// ユーザースペースが存在しなかったとき
		return(NULL);
	}

	// ユーザースペースに拡張ヘッダがあるかどうかチェック
	pExHdr = (TIM2_EXHEADER *)pUserSpace;
	if(pExHdr->ExHeaderId[0]!='e' ||
		pExHdr->ExHeaderId[1]!='X' ||
		pExHdr->ExHeaderId[2]!='t' ||
		pExHdr->ExHeaderId[3]!=0x00) {

		// 拡張ヘッダが見つからなかった場合
		return(NULL);
	}

	// 拡張ヘッダ存在していたとき
	if(pExHdr->UserSpaceSize==((sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize))) {
		// ユーザースペースの有意なサイズが、拡張ヘッダとユーザーデータの合計サイズに等しかったとき
		// コメント文字列は存在しないと判断できる
		return(NULL);
	}

	// コメントが見つかった場合
	return((char *)pUserSpace + sizeof(TIM2_EXHEADER) + pExHdr->UserDataSize);
}



// 指定したMIPMAPレベルのイメージデータの先頭アドレスを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// mipmap:  MIPMAPテクスチャレベル
// 戻り値
//          NULLのとき、該当するイメージデータなし
//          NULLでないとき、イメージデータの先頭アドレス
void *Tim2GetImage(TIM2_PICTUREHEADER *ph, int mipmap)
{
	void *pImage;
	TIM2_MIPMAPHEADER *pm;
	int i;

	if(mipmap>=ph->MipMapTextures) {
		// 指定されたレベルのMIPMAPテクスチャは存在しない
		return(NULL);
	}

	// イメージデータの先頭アドレスを計算
	pImage = (void *)((char *)ph + ph->HeaderSize);
	if(ph->MipMapTextures==1) {
		// LV0テクスチャのみの場合
		return(pImage);
	}

	// MIPMAPテクスチャが存在している場合
	pm = (TIM2_MIPMAPHEADER *)((char *)ph + sizeof(TIM2_PICTUREHEADER));
	for(i=0; i<mipmap; i++) {
		pImage = (void *)((char *)pImage + pm->MMImageSize[i]);
	}
	return(pImage);
}


// CLUTデータの先頭アドレスを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// 戻り値
//          NULLのとき、該当するCLUTデータなし
//          NULLでないとき、CLUTデータの先頭アドレス
void *Tim2GetClut(TIM2_PICTUREHEADER *ph)
{
	void *pClut;
	if(ph->ClutColors==0) {
		// CLUTデータ部を構成する色数が0のとき
		pClut = NULL;
	} else {
		// CLUTデータが存在するとき
		pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);
	}
	return(pClut);
}


// CLUTカラーを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// clut:    CLUTセットの指定
// no:      何番目のインデクスを取得するか指定
// 戻り値
//          RGBA32フォーマットで色を返す
//          clut,no等の指定にエラーがあるとき、0x00000000(黒)を返す
unsigned int Tim2GetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no)
{
	unsigned char *pClut;
	int n;
	unsigned char r, g, b, a;

	pClut = (unsigned char *)Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUTデータがなかったとき
		return(0);
	}

	// まず、何番目の色データか計算
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // 不正なピクセルカラーのとき
	}
	if(n>ph->ClutColors) {
		// 指定されたCLUT番号,インデクスの色データが存在しなかったとき
		return(0);
	}

	// CLUT部のフォーマットによっては、配置が並び替えられている可能性がある
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,16ビット,並び替えずみ
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,24ビット,並び替えずみ
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,32ビット,並び替えずみ
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256色,CSM1,16ビット,並び替えずみ
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256色,CSM1,24ビット,並び替えずみ
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256色,CSM1,32ビット,並び替えずみ
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8～15を+16～23に
				} else if((n & 31)<24) {
					n -= 8;    // +16～23を+8～15に
				}
			}
			break;
	}

	// CLUT部のピクセルフォーマットに基づいて、色データを得る
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bitカラーのとき
			r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
			g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
			b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
			a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);
			break;

		case TIM2_RGB24:
			// 24bitカラーのとき
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			break;

		case TIM2_RGB32:
			// 32bitカラーのとき
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			break;

		default:
			// 不正なピクセルフォーマットの場合
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// CLUTカラーを設定する
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// clut:    CLUTセットの指定
// no:      何番目のインデクスを設定するか指定
// color:   設定する色(RGB32フォーマット)
// 戻り値
//          RGBA32フォーマットで古い色を返す
//          clut,no等の指定にエラーがあるとき、0x00000000(黒)を返す
unsigned int Tim2SetClutColor(TIM2_PICTUREHEADER *ph, int clut, int no, unsigned int newcolor)
{
	unsigned char *pClut;
	unsigned char r, g, b, a;
	int n;

	pClut = (unsigned char *)Tim2GetClut(ph);
	if(pClut==NULL) {
		// CLUTデータがなかったとき
		return(0);
	}

	// まず、何番目の色データか計算
	switch(ph->ImageType) {
		case TIM2_IDTEX4:	n = clut*16 + no;	break;
		case TIM2_IDTEX8:	n = clut*256 + no;	break;
		default:         	return(0);    // 不正なピクセルカラーのとき
	}
	if(n>ph->ClutColors) {
		// 指定されたCLUT番号,インデクスの色データが存在しなかったとき
		return(0);
	}

	// CLUT部のフォーマットによっては、配置が並び替えられている可能性がある
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,16ビット,並び替えずみ
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,24ビット,並び替えずみ
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,32ビット,並び替えずみ
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256色,CSM1,16ビット,並び替えずみ
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256色,CSM1,24ビット,並び替えずみ
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256色,CSM1,32ビット,並び替えずみ
			if((n & 31)>=8) {
				if((n & 31)<16) {
					n += 8;    // +8～15を+16～23に
				} else if((n & 31)<24) {
					n -= 8;    // +16～23を+8～15に
				}
			}
			break;
	}

	// CLUT部のピクセルフォーマットに基づいて、色データを得る
	switch(ph->ClutType & 0x3F) {
		case TIM2_RGB16:
			// 16bitカラーのとき
			{
				unsigned char rr, gg, bb, aa;
				r = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])<<3) & 0xF8);
				g = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>2) & 0xF8);
				b = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>7) & 0xF8);
				a = (unsigned char)((((pClut[n*2 + 1]<<8) | pClut[n*2])>>8) & 0x80);

				rr = (unsigned char)((newcolor>>3)  & 0x1F);
				gg = (unsigned char)((newcolor>>11) & 0x1F);
				bb = (unsigned char)((newcolor>>19) & 0x1F);
				aa = (unsigned char)((newcolor>>31) & 1);

				pClut[n*2]     = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr))    & 0xFF);
				pClut[n*2 + 1] = (unsigned char)((((aa<<15) | (bb<<10) | (gg<<5) | rr)>>8) & 0xFF);
			}
			break;

		case TIM2_RGB24:
			// 24bitカラーのとき
			r = pClut[n*3];
			g = pClut[n*3 + 1];
			b = pClut[n*3 + 2];
			a = 0x80;
			pClut[n*3]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*3 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*3 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			break;

		case TIM2_RGB32:
			// 32bitカラーのとき
			r = pClut[n*4];
			g = pClut[n*4 + 1];
			b = pClut[n*4 + 2];
			a = pClut[n*4 + 3];
			pClut[n*4]     = (unsigned char)((newcolor)     & 0xFF);
			pClut[n*4 + 1] = (unsigned char)((newcolor>>8)  & 0xFF);
			pClut[n*4 + 2] = (unsigned char)((newcolor>>16) & 0xFF);
			pClut[n*4 + 3] = (unsigned char)((newcolor>>24) & 0xFF);
			break;

		default:
			// 不正なピクセルフォーマットの場合
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			break;
	}
	return((a<<24) | (g<<16) | (b<<8) | r);
}


// テクセル(色情報とは限らない)データを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// mipmap:  MIPMAPテクスチャレベル
// x:       テクセルデータを得るテクセルX座標
// y:       テクセルデータを得るテクセルY座標
// 戻り値
//          色情報(4bitまたは8bitのインデクス番号、または16bit,24bit,32bitのダイレクトカラー)
unsigned int Tim2GetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y)
{
	unsigned char *pImage;
	int t;
	int w, h;

	pImage = (unsigned char *)Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// 指定レベルのテクスチャデータがなかった場合
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// テクセル座標が不正なとき
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			return((pImage[t*2 + 1]<<8) | pImage[t*2]);

		case TIM2_RGB24:
			return((pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3]);

		case TIM2_RGB32:
			return((pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4]);

		case TIM2_IDTEX4:
			if(x & 1) {
				return(pImage[t/2]>>4);
			} else {
				return(pImage[t/2] & 0x0F);
			}
		case TIM2_IDTEX8:
			return(pImage[t]);
	}

	// 不正なピクセルフォーマットだった場合
	return(0);
}



// テクセル(色情報とは限らない)データを設定する
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// mipmap:  MIPMAPテクスチャレベル
// x:       テクセルデータを得るテクセルX座標
// y:       テクセルデータを得るテクセルY座標
// 戻り値
//          色情報(4bitまたは8bitのインデクス番号、または16bit,24bit,32bitのダイレクトカラー)
unsigned int Tim2SetTexel(TIM2_PICTUREHEADER *ph, int mipmap, int x, int y, unsigned int newtexel)
{
	unsigned char *pImage;
	int t;
	int w, h;
	unsigned int oldtexel;

	pImage = (unsigned char *)Tim2GetImage(ph, mipmap);
	if(pImage==NULL) {
		// 指定レベルのテクスチャデータがなかった場合
		return(0);
	}
	Tim2GetMipMapPictureSize(ph, mipmap, &w, &h);
	if(x>w || y>h) {
		// テクセル座標が不正なとき
		return(0);
	}

	t = y*w + x;
	switch(ph->ImageType) {
		case TIM2_RGB16:
			oldtexel = (pImage[t*2 + 1]<<8) | pImage[t*2];
			pImage[t*2]     = (unsigned char)((newtexel)    & 0xFF);
			pImage[t*2 + 1] = (unsigned char)((newtexel>>8) & 0xFF);
			return(oldtexel);

		case TIM2_RGB24:
			oldtexel = (pImage[t*3 + 2]<<16) | (pImage[t*3 + 1]<<8) | pImage[t*3];
			pImage[t*3]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*3 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*3 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			return(oldtexel);

		case TIM2_RGB32:
			oldtexel = (pImage[t*4 + 3]<<24) | (pImage[t*4 + 2]<<16) | (pImage[t*4 + 1]<<8) | pImage[t*4];
			pImage[t*4]     = (unsigned char)((newtexel)     & 0xFF);
			pImage[t*4 + 1] = (unsigned char)((newtexel>>8)  & 0xFF);
			pImage[t*4 + 2] = (unsigned char)((newtexel>>16) & 0xFF);
			pImage[t*4 + 3] = (unsigned char)((newtexel>>24) & 0xFF);
			return(oldtexel);

		case TIM2_IDTEX4:
			if(x & 1) {
				oldtexel = pImage[t/2]>>4;
				pImage[t/2] = (unsigned char)((newtexel<<4) | oldtexel);
			} else {
				oldtexel = pImage[t/2] & 0x0F;
				pImage[t/2] = (unsigned char)((oldtexel<<4) | newtexel);
			}
			return(oldtexel);
		case TIM2_IDTEX8:
			oldtexel = pImage[t];
			pImage[t] = (unsigned char)newtexel;
			return(oldtexel);
	}

	// 不正なピクセルフォーマットだった場合
	return(0);
}


// テクスチャカラーを得る
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// mipmap:  MIPMAPテクスチャレベル
// clut:    インデクスカラーの変換に用いるCLUTセット番号
// x:       テクセルデータを得るテクセルX座標
// y:       テクセルデータを得るテクセルY座標
// 戻り値
//          RGBA32フォーマットで色を返す
//          clutの指定にエラーがあるとき、0x00000000(黒)を返す
//          x,yの指定にエラーがあったときの動作は保証しない
unsigned int Tim2GetTextureColor(TIM2_PICTUREHEADER *ph, int mipmap, int clut, int x, int y)
{
	unsigned int t;
	if(Tim2GetImage(ph, mipmap)==NULL) {
		// 指定レベルのテクスチャデータがなかった場合
		return(0);
	}
	t = Tim2GetTexel(ph, mipmap, (x>>mipmap), (y>>mipmap));
	switch(ph->ImageType) {
		case TIM2_RGB16:
			{
				unsigned char r, g, b, a;
				r = (unsigned char)((t<<3) & 0xF8);
				g = (unsigned char)((t>>2) & 0xF8);
				b = (unsigned char)((t>>7) & 0xF8);
				a = (unsigned char)((t>>8) & 0x80);
				return((a<<24) | (g<<16) | (b<<8) | r);
			}

		case TIM2_RGB24:
			return((0x80<<24) | (t & 0x00FFFFFF));

		case TIM2_RGB32:
			return(t);

		case TIM2_IDTEX4:
		case TIM2_IDTEX8:
			return(Tim2GetClutColor(ph, clut, t));
	}
	return(0);
}






// これ以降の関数は、PS2のee-gccでのみ使用できる関数
#ifdef R5900

// TIM2ピクチャデータをGSローカルメモリに読み込む
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// tbp:     転送先テクスチャバッファのページベースアドレス(-1のときヘッダ内の値を使用)
// cbp:     転送先CLUTバッファのページベースアドレス(-1のときヘッダ内の値を使用)
// 戻り値
//          NULLのとき	エラー
//          NULLでないとき、次のバッファアドレス
// 注意
//          CLUT格納モードとしてCSM2が指定されていた場合も、強制的にCSM1に変換してGSに送信される
unsigned int Tim2LoadPicture(TIM2_PICTUREHEADER *ph, unsigned int tbp, unsigned int cbp)
{
	// CLUTデータを転送転送
	tbp = Tim2LoadImage(ph, tbp);
	Tim2LoadClut(ph, cbp);
	return(tbp);
}


// TIM2ピクチャのイメージデータ部をGSローカルメモリに読み込む
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// tbp:     転送先テクスチャバッファのページベースアドレス(-1のときヘッダ内の値を使用)
// 戻り値
//          NULLのとき	エラー
//          NULLでないとき、次のテクスチャバッファアドレス
// 注意
//          CLUT格納モードとしてCSM2が指定されていた場合も、強制的にCSM1に変換してGSに送信される
unsigned int Tim2LoadImage(TIM2_PICTUREHEADER *ph, unsigned int tbp)
{
	// イメージデータを送信
	if(ph->MipMapTextures>0) {
		static const int psmtbl[] = {
			SCE_GS_PSMCT16,
			SCE_GS_PSMCT24,
			SCE_GS_PSMCT32,
			SCE_GS_PSMT4,
			SCE_GS_PSMT8
		};
		int i;
		int psm;
		u_long128 *pImage;
		int w, h;
		int tbw;

		psm = psmtbl[ph->ImageType - 1]; // ピクセルフォーマットを得る
		((sceGsTex0 *)&ph->GsTex0)->PSM  = psm;

		w = ph->ImageWidth;  // イメージXサイズ
		h = ph->ImageHeight; // イメージYサイズ

		// イメージデータの先頭アドレスを計算
		pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
		if(tbp==-1) {
			// tbpの指定が-1のとき、ピクチャヘッダに指定されたGsTex0からtbp,tbwを得る
			tbp = ((sceGsTex0 *)&ph->GsTex0)->TBP0;
			tbw = ((sceGsTex0 *)&ph->GsTex0)->TBW;

			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // テクスチャデータをGSに転送
		} else {
			// tbpの指定が指定されたとき、ピクチャヘッダのGsTex0メンバのtbp,tbwをオーバーライド
			tbw = Tim2CalcBufWidth(psm, w);
			// GSのTEX0レジスタに設定する値を更新
			((sceGsTex0 *)&ph->GsTex0)->TBP0 = tbp;
			((sceGsTex0 *)&ph->GsTex0)->TBW  = tbw;
			Tim2LoadTexture(psm, tbp, tbw, w, h, pImage); // テクスチャデータをGSに転送
			tbp += Tim2CalcBufSize(psm, w, h);            // tbpの値を更新
		}

		if(ph->MipMapTextures>1) {
			// MIPMAPテクスチャがある場合
			TIM2_MIPMAPHEADER *pm;

			pm = (TIM2_MIPMAPHEADER *)(ph + 1); // ピクチャヘッダの直後にMIPMAPヘッダ

			// LV0のテクスチャサイズを得る
			// tbpを引数で指定されたとき、ピクチャヘッダにあるmiptbpを無視して自動計算
			if(tbp!=-1) {
				pm->GsMiptbp1 = 0;
				pm->GsMiptbp2 = 0;
			}

			pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
			// 各MIPMAPレベルのイメージを転送
			for(i=1; i<ph->MipMapTextures; i++) {
				// MIPMAPレベルがあがると、テクスチャサイズは1/2になる
				w = w / 2;
				h = h / 2;

				pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
				if(tbp==-1) {
					// テクスチャページの指定が-1のとき、MIPMAPヘッダに指定されたtbp,tbwを使用する
					int miptbp;
					if(i<4) {
						// MIPMAPレベル1,2,3のとき
						miptbp = (pm->GsMiptbp1>>((i-1)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp1>>((i-1)*20 + 14)) & 0x3F;
					} else {
						// MIPMAPレベル4,5,6のとき
						miptbp = (pm->GsMiptbp2>>((i-4)*20)) & 0x3FFF;
						tbw    = (pm->GsMiptbp2>>((i-4)*20 + 14)) & 0x3F;
					}
					Tim2LoadTexture(psm, miptbp, tbw, w, h, pImage);
				} else {
					// テクスチャページが指定されているとき、MIPMAPヘッダを再設定
					tbw = Tim2CalcBufWidth(psm, w);    // テクスチャ幅を計算
					if(i<4) {
						// MIPMAPレベル1,2,3のとき
						pm->GsMiptbp1 |= ((u_long)tbp)<<((i-1)*20);
						pm->GsMiptbp1 |= ((u_long)tbw)<<((i-1)*20 + 14);
					} else {
						// MIPMAPレベル4,5,6のとき
						pm->GsMiptbp2 |= ((u_long)tbp)<<((i-4)*20);
						pm->GsMiptbp2 |= ((u_long)tbw)<<((i-4)*20 + 14);
					}
					Tim2LoadTexture(psm, tbp, tbw, w, h, pImage);
					tbp += Tim2CalcBufSize(psm, w, h); // tbpの値を更新
				}
			}
		}
	}
	return(tbp);
}



// TIM2ピクチャのCLUTデータ部をGSローカルメモリに転送
// 引数
// ph:      TIM2ピクチャヘッダの先頭アドレス
// cbp:     転送先CLUTバッファのページベースアドレス(-1のときヘッダ内の値を使用)
// 戻り値
//          0のときエラー
//          非0のとき成功
// 注意
//          CLUT格納モードとしてCSM2が指定されていた場合も、強制的にCSM1に変換してGSに送信される
unsigned int Tim2LoadClut(TIM2_PICTUREHEADER *ph, unsigned int cbp)
{
	int i;
	sceGsLoadImage li;
	u_long128 *pClut;
	int	cpsm;

	// CLUTピクセルフォーマットを得る
	if(ph->ClutType==TIM2_NONE) {
		// CLUTデータが存在しないとき
		return(1);
	} else if((ph->ClutType & 0x3F)==TIM2_RGB16) {
		cpsm = SCE_GS_PSMCT16;
	} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
		cpsm = SCE_GS_PSMCT24;
	} else {
		cpsm = SCE_GS_PSMCT32;
	}
	((sceGsTex0 *)&ph->GsTex0)->CPSM = cpsm; // CLUT部ピクセルフォーマット設定
	((sceGsTex0 *)&ph->GsTex0)->CSM  = 0;    // CLUT格納モード(常にCSM1)
	((sceGsTex0 *)&ph->GsTex0)->CSA  = 0;    // CLUTエントリオフセット(常に0)
	((sceGsTex0 *)&ph->GsTex0)->CLD  = 1;    // CLUTバッファのロード制御(常にロード)

	if(cbp==-1) {
		// cbpの指定がないとき、ピクチャヘッダのGsTex0メンバから値を取得
		cbp = ((sceGsTex0 *)&ph->GsTex0)->CBP;
	} else {
		// cbpが指定されたとき、ピクチャヘッダのGsTex0メンバの値をオーバーライド
		((sceGsTex0 *)&ph->GsTex0)->CBP = cbp;
	}

	// CLUTデータの先頭アドレスを計算
	pClut = (u_long128 *)((char *)ph + ph->HeaderSize + ph->ImageSize);

	// CLUTデータをGSローカルメモリに送信
	// CLUT形式とテクスチャ形式によってCLUTデータのフォーマットなどが決まる
	switch((ph->ClutType<<8) | ph->ImageType) {
		case (((TIM2_RGB16 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,16ビット,並び替えずみ
		case (((TIM2_RGB24 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,24ビット,並び替えずみ
		case (((TIM2_RGB32 | 0x40)<<8) | TIM2_IDTEX4): // 16色,CSM1,32ビット,並び替えずみ
		case (( TIM2_RGB16        <<8) | TIM2_IDTEX8): // 256色,CSM1,16ビット,並び替えずみ
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX8): // 256色,CSM1,24ビット,並び替えずみ
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX8): // 256色,CSM1,32ビット,並び替えずみ
			// 256色CLUTかつ、CLUT格納モードがCSM1のとき
			// 16色CLUTかつ、CLUT格納モードがCSM1で入れ替え済みフラグがONのとき
			// すでにピクセルが入れ替えられて配置されているのでそのまま転送可能だー
			Tim2LoadTexture(cpsm, cbp, 1, 16, (ph->ClutColors / 16), pClut);
			break;

		case (( TIM2_RGB16        <<8) | TIM2_IDTEX4): // 16色,CSM1,16ビット,リニア配置
		case (( TIM2_RGB24        <<8) | TIM2_IDTEX4): // 16色,CSM1,24ビット,リニア配置
		case (( TIM2_RGB32        <<8) | TIM2_IDTEX4): // 16色,CSM1,32ビット,リニア配置
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX4): // 16色,CSM2,16ビット,リニア配置
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX4): // 16色,CSM2,24ビット,リニア配置
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX4): // 16色,CSM2,32ビット,リニア配置
		case (((TIM2_RGB16 | 0x80)<<8) | TIM2_IDTEX8): // 256色,CSM2,16ビット,リニア配置
		case (((TIM2_RGB24 | 0x80)<<8) | TIM2_IDTEX8): // 256色,CSM2,24ビット,リニア配置
		case (((TIM2_RGB32 | 0x80)<<8) | TIM2_IDTEX8): // 256色,CSM2,32ビット,リニア配置
			// 16色CLUTかつ、CLUT格納モードがCSM1で入れ替え済みフラグがOFFのとき
			// 16色CLUTかつ、CLUT格納モードがCSM2のとき
			// 256色CLUTかつ、CLUT格納モードがCSM2のとき

			// CSM2はパフォーマンスが悪いので、CSM1として入れ替えながら転送
			for(i=0; i<(ph->ClutColors/16); i++) {
				sceGsSetDefLoadImage(&li, cbp, 1, cpsm, (i & 1)*8, (i>>1)*2, 8, 2);
				FlushCache(WRITEBACK_DCACHE);   // Dキャッシュの掃き出し
				sceGsExecLoadImage(&li, pClut); // CLUTデータをGSに転送
				sceGsSyncPath(0, 0);            // データ転送終了まで待機

				// 次の16色へ、アドレス更新
				if((ph->ClutType & 0x3F)==TIM2_RGB16) {
					pClut = (u_long128 *)((char *)pClut + 2*16); // 16bit色のとき
				} else if((ph->ClutType & 0x3F)==TIM2_RGB24) {
					pClut = (u_long128 *)((char *)pClut + 3*16); // 24bit色のとき
				} else {
					pClut = (u_long128 *)((char *)pClut + 4*16); // 32bit色のとき
				}
			}
			break;

		default:
			printf("Illegal clut and texture combination. ($%02X,$%02X)\n", ph->ClutType, ph->ImageType);
			return(0);
	}
	return(1);
}


// TIM2ファイルでスナップショットを書き出す
// 引数
// d0:      偶数ラスタのフレームバッファ
// d1:      奇数ラスタのフレームバッファ(NULLのときノンインタレース)
// pszFname:TIM2ファイル名
// 返り値
//          0のときエラー
//          非0のとき成功
int Tim2TakeSnapshot(sceGsDispEnv *d0, sceGsDispEnv *d1, char *pszFname)
{
	int i;

	int h;               // ファイルハンドル
	int nWidth, nHeight; // イメージの寸法
	int nImageType;      // イメージ部種別
	int psm;             // ピクセルフォーマット
	int nBytes;          // 1ラスタを構成するバイト数

	// 画像サイズ,ピクセルフォーマットを得る
	nWidth  = d0->display.DW / (d0->display.MAGH + 1);
	nHeight = d0->display.DH + 1;
	psm     = d0->dispfb.PSM;

	// ピクセルフォーマットから、1ラスタあたりのバイト数,TIM2ピクセル種別を求める
	switch(psm) {
		case SCE_GS_PSMCT32 :	nBytes = nWidth*4;	nImageType = TIM2_RGB32;	break;
		case SCE_GS_PSMCT24 :	nBytes = nWidth*3;	nImageType = TIM2_RGB24;	break;
		case SCE_GS_PSMCT16 :	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		case SCE_GS_PSMCT16S:	nBytes = nWidth*2;	nImageType = TIM2_RGB16;	break;
		default:
			// 不明なピクセルフォーマットのとき、エラー終了
			// GS_PSGPU24フォーマットは非サポート
			printf("Illegal pixel format.\n");
			return(0);
	}


	// TIM2ファイルをオープン
	h = sceOpen(pszFname, SCE_WRONLY | SCE_CREAT);
	if(h==-1) {
		// ファイルオープン失敗
		printf("file create failure.\n");
		return(0);
	}

	// ファイルヘッダを書き出し
	{
		TIM2_FILEHEADER fhdr;

		fhdr.FileId[0] = 'T';   // ファイルIDを設定
		fhdr.FileId[1] = 'I';
		fhdr.FileId[2] = 'M';
		fhdr.FileId[3] = '2';
		fhdr.FormatVersion = 3; // フォーマットバージョン4
		fhdr.FormatId  = 0;     // 16バイトアラインメントモード
		fhdr.Pictures  = 1;     // ピクチャ枚数は1枚
		for(i=0; i<8; i++) {
			fhdr.pad[i] = 0x00; // パディングメンバを0x00でクリア
		}

		sceWrite(h, &fhdr, sizeof(TIM2_FILEHEADER)); // ファイルヘッダを書き出し
	}

	// ピクチャヘッダを書き出し
	{
		TIM2_PICTUREHEADER phdr;
		int nImageSize;

		nImageSize = nBytes * nHeight;
		phdr.TotalSize   = sizeof(TIM2_PICTUREHEADER) + nImageSize; // トータルサイズ
		phdr.ClutSize    = 0;                           // CLUT部なし
		phdr.ImageSize   = nImageSize;                  // イメージ部サイズ
		phdr.HeaderSize  = sizeof(TIM2_PICTUREHEADER);  // ヘッダ部サイズ
		phdr.ClutColors  = 0;                           // CLUT色数
		phdr.PictFormat  = 0;                           // ピクチャ形式
		phdr.MipMapTextures = 1;                        // MIPMAPテクスチャ枚数
		phdr.ClutType    = TIM2_NONE;                   // CLUT部なし
		phdr.ImageType   = nImageType;                  // イメージ種別
		phdr.ImageWidth  = nWidth;                      // イメージ横幅
		phdr.ImageHeight = nHeight;                     // イメージ高さ

		// GSレジスタの設定は全部0にしておく
		phdr.GsTex0        = 0;
		((sceGsTex0 *)&phdr.GsTex0)->TBW = Tim2CalcBufWidth(psm, nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->PSM = psm;
		((sceGsTex0 *)&phdr.GsTex0)->TW  = Tim2GetLog2(nWidth);
		((sceGsTex0 *)&phdr.GsTex0)->TH  = Tim2GetLog2(nHeight);
		phdr.GsTex1        = 0;
		phdr.GsTexaFbaPabe = 0;
		phdr.GsTexClut     = 0;

		sceWrite(h, &phdr, sizeof(TIM2_PICTUREHEADER)); // ピクチャヘッダを書き出し
	}


	// イメージデータの書き出し
	for(i=0; i<nHeight; i++) {
		u_char buf[4096];   // ラスタバッファを4KB確保
		sceGsStoreImage si;

		if(d1) {
			// インタレースのとき
			if(!(i & 1)) {
				// インタレース表示の偶数ラスタのとき
				sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
				                      d0->dispfb.DBX, (d0->dispfb.DBY + i/2),
				                      nWidth, 1);
			} else {
				// インタレース表示の奇数ラスタのとき
				sceGsSetDefStoreImage(&si, d1->dispfb.FBP*32, d1->dispfb.FBW, psm,
				                      d1->dispfb.DBX, (d1->dispfb.DBY + i/2),
				                      nWidth, 1);
			}
		} else {
			// ノンインタレースのとき
			sceGsSetDefStoreImage(&si, d0->dispfb.FBP*32, d0->dispfb.FBW, psm,
			                      d0->dispfb.DBX, (d0->dispfb.DBY + i),
			                      nWidth, 1);
		}
		FlushCache(WRITEBACK_DCACHE); // Dキャッシュの掃き出し

		sceGsExecStoreImage(&si, (u_long128 *)buf); // VRAMへの転送を起動
		sceGsSyncPath(0, 0);                        // データ転送終了まで待機

		sceWrite(h, buf, nBytes);  // 1ラスタ分のデータを書き出し
	}

	// ファイルの書き出し完了
	sceClose(h);  // ファイルをクローズ
	return(1);
}






// テクスチャデータを転送
// 引数
// psm:     テクスチャピクセルフォーマット
// tbp:     テクスチャバッファのベースポイント
// tbw:     テクスチャバッファの幅
// w:       転送領域の横幅
// h:       転送領域のライン数
// pImage:  テクスチャイメージが格納されているアドレス
static void Tim2LoadTexture(int psm, u_int tbp, int tbw, int w, int h, u_long128 *pImage)
{
	sceGsLoadImage li;
	int i, l, n;
	u_long128 *p;

	switch(psm) {
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMCT32:
			n = w*4;
			break;

		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT24:
			n = w*3;
			break;

		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
			n = w*2;
			break;

		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT8:
			n = w;
			break;

		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMT4:
			n = w/2;
			break;

		default:
			return;
	}

	// DMAの最大転送量の512KBを超えないように分割しながら送信
	l = 32764 * 16 / n;
	for(i=0; i<h; i+=l) {
		p = (u_long128 *)((char *)pImage + n*i);
		if((i+l)>h) {
			l = h - i;
		}

		sceGsSetDefLoadImage(&li, tbp, tbw, psm, 0, i, w, l);
		FlushCache(WRITEBACK_DCACHE); // Dキャッシュの掃き出し
		sceGsExecLoadImage(&li, p);   // GSローカルメモリへの転送を起動
		sceGsSyncPath(0, 0);          // データ転送終了まで待機
	}
	return;
}



// 指定されたピクセルフォーマットとテクスチャサイズから、バッファサイズを計算
// 引数
// psm:     テクスチャピクセルフォーマット
// w:       横幅
// 返り値
//          1ラインで消費するバッファサイズ
//          単位は256バイト(64word)
static int Tim2CalcBufWidth(int psm, int w)
{
//	return(w / 64);

	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			return((w+63) / 64);

		case SCE_GS_PSMT8:
		case SCE_GS_PSMT4:
			w = (w+63) / 64;
			if(w & 1) {
				w++;
			}
			return(w);
	}
	return(0);
}



// 指定されたピクセルフォーマットとテクスチャサイズから、バッファサイズを計算
// 引数
// psm:     テクスチャピクセルフォーマット
// w:       横幅
// h:       ライン数
// 返り値
//          1ラインで消費するバッファサイズ
//          単位は256バイト(64word)
// 注意
//          続くテクスチャが異なるピクセルフォーマットを格納する場合は、
//          2KBページバウンダリまでtbpをアラインメントを調整する必要がある。
static int Tim2CalcBufSize(int psm, int w, int h)
{
	return(w * h / 64);
/*
	switch(psm) {
		case SCE_GS_PSMT8H:
		case SCE_GS_PSMT4HL:
		case SCE_GS_PSMT4HH:
		case SCE_GS_PSMCT32:
		case SCE_GS_PSMCT24:
		case SCE_GS_PSMZ32:
		case SCE_GS_PSMZ24:
			// 1ピクセルあたり、1ワード消費
			return(((w+63)/64) * ((h+31)/32));

		case SCE_GS_PSMCT16:
		case SCE_GS_PSMCT16S:
		case SCE_GS_PSMZ16:
		case SCE_GS_PSMZ16S:
			// 1ピクセルあたり、1/2ワード消費
			return(((w+63)/64) * ((h+63)/64));

		case SCE_GS_PSMT8:
			// 1ピクセルあたり、1/4ワード消費
			return(((w+127)/128) * ((h+63)/64));

		case SCE_GS_PSMT4:
			// 1ピクセルあたり、1/8ワード消費
			return(((w+127)/128) * ((h+127)/128));
	}
	// エラー?
	return(0);
*/
}



// ビット幅を得る
static int Tim2GetLog2(int n)
{
	int i;
	for(i=31; i>0; i--) {
		if(n & (1<<i)) {
			break;
		}
	}
	if(n>(1<<i)) {
		i++;
	}
	return(i);
}


#endif	// R5900
