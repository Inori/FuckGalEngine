/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2004-4-9
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef IMAGE_CODEC_FREEIMAGE_2004_04_09_H
#define IMAGE_CODEC_FREEIMAGE_2004_04_09_H

//-------------------------------------------------------------------------------------
/**
    Read/Write image use FreeImage library (<B>Need FreeImage</B>).
*/
class FCImageCodec_FreeImage : public FCImageCodec
{
public:
    FCImageCodec_FreeImage() {FreeImage_Initialise(TRUE);}
    virtual ~FCImageCodec_FreeImage() {FreeImage_DeInitialise();}

private:
    struct FCAutoFIMEMORY
    {
        FIMEMORY   * m_mem ;
        FREE_IMAGE_FORMAT   m_format ;

        FCAutoFIMEMORY (const void* p, int nSize)
        {
            if (p && nSize)
            {
                m_mem = FreeImage_OpenMemory ((BYTE*)const_cast<void*>(p), nSize) ;
            }
            else
            {
                m_mem = 0 ;
            }
            m_format = (m_mem ? FreeImage_GetFileTypeFromMemory(m_mem) : FIF_UNKNOWN) ;
        }

        ~FCAutoFIMEMORY()
        {
            if (m_mem)
            {
                FreeImage_CloseMemory (m_mem) ;
            }
        }
    };

    struct FCAutoFIBITMAP
    {
        FIBITMAP   * m_bmp ;

        FCAutoFIBITMAP (FIBITMAP* pBmp) : m_bmp(pBmp) {}
        ~FCAutoFIBITMAP()
        {
            if (m_bmp)
            {
                FreeImage_Unload (m_bmp) ;
            }
        }
    };

    struct FCAutoFIMULTIBITMAP
    {
        FIMULTIBITMAP   * m_bmp ;

        FCAutoFIMULTIBITMAP (FIMULTIBITMAP* pBmp) : m_bmp(pBmp) {}
        ~FCAutoFIMULTIBITMAP()
        {
            if (m_bmp)
            {
                FreeImage_CloseMultiBitmap (m_bmp) ;
            }
        }
    };

    static void StoreFrame (FIBITMAP* pFIimage, std::vector<FCObjImage*>& rImageList)
    {
        FCObjImage   * pImg = new FCObjImage ;
        LoadImage (pFIimage, *pImg) ;

        if (pImg->IsValidImage())
        {
            rImageList.push_back (pImg) ;
        }
        else
        {
            delete pImg ;
        }
    }

    static void StoreMultiPage (FIMULTIBITMAP* pMulti, std::vector<FCObjImage*>& image_list, FCImageProperty& rProperty)
    {
        if (!pMulti)
            return ;

        for (int i=0 ; i < FreeImage_GetPageCount(pMulti) ; i++)
        {
            FIBITMAP   * pFrame = FreeImage_LockPage (pMulti, i) ;
            if (pFrame)
            {
                StoreFrame (pFrame, image_list) ;

                // save frame interval, in millisecond
                FITAG   * tag = 0 ;
                if (FreeImage_GetMetadata (FIMD_ANIMATION, pFrame, "FrameTime", &tag) && tag)
                {
                    rProperty.m_frame_delay.push_back (*(int*)FreeImage_GetTagValue(tag)) ;
                }

                FreeImage_UnlockPage (pMulti, pFrame, FALSE) ;
            }
        }
    }

    // Read file routine
    virtual bool LoadImageFile (const wchar_t* szFileName,
                                std::vector<FCObjImage*>& rImageList,
                                FCImageProperty& rImageProp)
    {
        std::string   fname = FC_W2A(szFileName) ;

        FREE_IMAGE_FORMAT   imgType = FreeImage_GetFileType (fname.c_str()) ;
        if (imgType == FIF_UNKNOWN)
            return false ;

        if ((imgType == FIF_GIF) || (imgType == FIF_TIFF) || (imgType == FIF_ICO))
        {
            FCAutoFIMULTIBITMAP   FI (FreeImage_OpenMultiBitmap (imgType, fname.c_str(), FALSE, TRUE)) ;
            StoreMultiPage (FI.m_bmp, rImageList, rImageProp) ;
        }
        else
        {
            FCAutoFIBITMAP   FI (FreeImage_Load (imgType, fname.c_str())) ;
            StoreFrame (FI.m_bmp, rImageList) ;
        }
        return (rImageList.size() != 0) ;
    }

    // Read memory routine
    virtual bool LoadImageMemory (const void* pStart, int nMemSize,
                                  std::vector<FCObjImage*>& rImageList,
                                  FCImageProperty& rImageProp)
    {
        FCAutoFIMEMORY   pMem (pStart, nMemSize) ;

        FREE_IMAGE_FORMAT   imgType = pMem.m_format ;
        if (imgType == FIF_UNKNOWN)
            return false ;

        if ((imgType == FIF_GIF) || (imgType == FIF_TIFF) || (imgType == FIF_ICO))
        {
            FCAutoFIMULTIBITMAP   FI (FreeImage_LoadMultiBitmapFromMemory (imgType, pMem.m_mem)) ;
            StoreMultiPage (FI.m_bmp, rImageList, rImageProp) ;
        }
        else
        {
            FCAutoFIBITMAP   FI (FreeImage_LoadFromMemory (imgType, pMem.m_mem)) ;
            StoreFrame (FI.m_bmp, rImageList) ;
        }
        return (rImageList.size() != 0) ;
    }

    // save to file routine
    virtual bool SaveImageFile (const wchar_t* szFileName,
                                const std::vector<const FCObjImage*>& rImageList,
                                const FCImageProperty& rImageProp)
    {
        if (rImageList.empty() || !rImageList[0]->IsValidImage())
            return false ;
        const FCObjImage   & img = *rImageList[0] ;

        // is the FreeImage support
        FREE_IMAGE_FORMAT   imgFormat = FreeImage_GetFIFFromFilename (FC_W2A(szFileName).c_str()) ;
        if (imgFormat == FIF_UNKNOWN)
            return false ;
        if (!FreeImage_FIFSupportsWriting(imgFormat) || !FreeImage_FIFSupportsExportBPP(imgFormat, img.ColorBits()))
            return false ;

        // create FreeImage object
        FCAutoFIBITMAP   FI (AllocateFreeImage (img)) ;
        if (!FI.m_bmp)
            return false ;

        // save flag
        int   nFlag = 0 ;
        if ((imgFormat == FIF_GIF) && (img.ColorBits() == 8))
        {
            // gif transparency
            if (rImageProp.m_SaveFlag != -1)
            {
                std::vector<BYTE>   aTab (256, (BYTE)0xFF) ;
                aTab[FClamp0255(rImageProp.m_SaveFlag)] = 0 ;
                FreeImage_SetTransparent (FI.m_bmp, true) ;
                FreeImage_SetTransparencyTable (FI.m_bmp, &aTab[0], 256) ;
            }
        }
        else if (imgFormat == FIF_JPEG)
        {
            nFlag = ((rImageProp.m_SaveFlag == -1) ? 90 : rImageProp.m_SaveFlag) ;
        }

        // save image file
        return (FreeImage_Save (imgFormat, FI.m_bmp, FC_W2A(szFileName).c_str(), nFlag) ? true : false) ;
    }

    std::string FC_W2A (const wchar_t* pws)
    {
        std::string   s ;
        if (pws)
        {
#if (defined(_WIN32) || defined(__WIN32__))
            int   nByte = WideCharToMultiByte(CP_THREAD_ACP, 0, pws, -1, NULL, 0, NULL, NULL) ;
            if (nByte)
            {
                std::vector<char>   p (nByte+1) ;
                WideCharToMultiByte(CP_THREAD_ACP, 0, pws, -1, &p[0], nByte, NULL, NULL) ;
                s = &p[0] ;
            }
#else
            size_t   nBytes = (wcslen(pws) + 1) * sizeof(wchar_t) ;
            std::vector<char>   p (nBytes, (char)0) ;
            wcstombs (&p[0], pws, nBytes) ;
            s = &p[0] ;
#endif
        }
        return s ;
    }

    // ImageStone --> FreeImage.
    static FIBITMAP* AllocateFreeImage (const FCObjImage& img)
    {
        if (!img.IsValidImage())
            return 0 ;

        // create FreeImage object
        FIBITMAP   * pFIimg = FreeImage_Allocate (img.Width(), img.Height(), img.ColorBits()) ;
        if (!pFIimg)
            return 0 ;

        // set pixel
        memcpy (FreeImage_GetBits(pFIimg), img.GetMemStart(), img.GetPitch()*img.Height()) ;

        // set palette
        if (img.ColorBits() <= 8)
        {
            RGBQUAD   pPal[256] ;
            img.GetColorTable (pPal) ;
            memcpy (FreeImage_GetPalette(pFIimg), pPal, FreeImage_GetColorsUsed(pFIimg) * sizeof(RGBQUAD)) ;
        }

        // clear, default is 2835 = 72dpi
        FreeImage_SetDotsPerMeterX (pFIimg, 0) ;
        FreeImage_SetDotsPerMeterY (pFIimg, 0) ;

        return pFIimg ;
    }

    // FreeImage --> ImageStone.
    static void LoadImage (FIBITMAP* pFrame, FCObjImage& img)
    {
        if (!pFrame)
        {
            img.Destroy() ;
            return ;
        }

        FIBITMAP  * pBMP = pFrame ;
        bool      bDeleteBMP = false ;

        switch (FreeImage_GetBPP(pFrame))
        {
            case 1 :
            case 4 :
            case 8 :
            case 16 :
                if (FreeImage_IsTransparent(pFrame))
                {
                    pBMP = FreeImage_ConvertTo32Bits(pFrame) ;
                }
                else
                {
                    pBMP = FreeImage_ConvertTo24Bits(pFrame) ;
                }
                bDeleteBMP = true ;
                break;

            case 24 :
            case 32 :
                break;
        }

        // create frame
        img.Create (FreeImage_GetWidth(pBMP), FreeImage_GetHeight(pBMP), FreeImage_GetBPP(pBMP)) ;
        if (img.IsValidImage())
        {
            memcpy (img.GetMemStart(), FreeImage_GetBits(pBMP), img.GetPitch()*img.Height()) ;
        }

        if (bDeleteBMP)
        {
            FreeImage_Unload (pBMP) ;
        }
    }
};

#endif
