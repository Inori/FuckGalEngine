/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2001-4-27
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef OBJ_IMAGE_2001_04_27_H
#define OBJ_IMAGE_2001_04_27_H
#include "graph.h"
#include "image_property.inl"
#include "image_codec.h"
#include "image_codecfactory.h"
#include "image_effect.inl"
#include "image_resize_filter.inl"

//-------------------------------------------------------------------------------------
/**
    Image object.
*/
class FCObjImage : public FCObjGraph
{
public:
    /// @name Construction.
    //@{
    /// Default constructor.
    FCObjImage () ;
    /// Copy constructor.
    FCObjImage (const FCObjImage& img) ;
    /// Destructor.
    virtual ~FCObjImage() {Destroy();}
    //@}

    FCObjImage& operator= (const FCObjImage& img) ;

#if (defined(_WIN32) || defined(__WIN32__))
    /// @name Windows OS only.
    //@{
    /**
        Get HBITMAP ( DIB handle ) \n
        you can select this handle in HDC to draw on it, but <B>don't</B> call DeleteObject and SetDIBColorTable on it.
    */
    operator HBITMAP() const {return m_DIB_Handle;}
    /**
        Create Gdiplus bitmap, return NULL if failed \n
        caller must <B>delete</B> returned object.
    */
    Gdiplus::Bitmap* CreateBitmap (int nTransparencyIndex=-1) const ;
    /// Create image based on Gdiplus bitmap.
    bool FromBitmap (Gdiplus::Bitmap& gpBmp) ;
    /// Create image based on HBITMAP, you can delete hBitmap after call.
    bool FromHBITMAP (HBITMAP hBitmap) ;
    /// Load bitmap resource.
    bool LoadBitmap (UINT nID, HINSTANCE hInstance=NULL) ;
    /// Load image resource.
    bool LoadResource (UINT nID, LPCTSTR resource_type, IMAGE_TYPE image_type, HMODULE hModule=NULL) ;
    /// Get image that in clipboard.
    bool GetClipboard() ;
    /// Copy image to clipboard.
    void CopyToClipboard() const ;
    /// Capture rect_on_dc of hdc, the image of captured is 24bpp.
    void CaptureDC (HDC hdc, RECT rect_on_dc) ;
    /**
        Create HRGN from image, white RGB(255,255,255) is in region, black RGB(0,0,0) is outside region \n
        user need call DeleteObject to delete returned HRGN.
    */
    HRGN CreateHRGN() const ;
    /// Draw image, if has alpha, must has been premultipled.
    void Draw (HDC hdc, RECT rect_on_dc, RECT* pOnImage=NULL) const ;
    /// Draw image, (x,y) is upper-left corner of destination, if has alpha, must has been premultipled.
    void Draw (HDC hdc, int x, int y) const ;
    //@}
#endif

    /// @name Create / Destroy.
    //@{
    /**
        Create image \n
        nColorBit - can be : <span style='color:#FF0000'>8 , 24 , 32</span> \n
        1 ) all pixel data are initialized to 0 \n
        2 ) if nColorBit == 8, a gray palette will be set.
    */
    bool Create (int nWidth, int nHeight, int nColorBit) ;
    /// Destroy image.
    void Destroy() ;
    //@}

    /**
        @name Attributes.
        origin (0,0) located at upper-left, x increase from left to right, y increase from top to down.
    */
    //@{
    /// Is current image object valid.
    bool IsValidImage() const {return (m_pixel != 0);}
    /// Is point inside image.
    bool IsInside (int x, int y) const {return (x>=0) && (x<Width()) && (y>=0) && (y<Height());}
    /// this function doesn't do boundary check, so <span style='color:#FF0000'>Crash</span> if nLine exceed.
    BYTE* GetBits (int nLine) const ;
    /// this function doesn't do boundary check, so <span style='color:#FF0000'>Crash</span> if y exceed.
    BYTE* GetBits (int x, int y) const ;
    /// Get start address of pixel (address of bottom-left point).
    BYTE* GetMemStart() const {return m_pixel;}
    /// Width of image.
    int Width() const {return m_width;}
    /// Height of image.
    int Height() const {return m_height;}
    /// Image's bpp (bit per pixel), available : 8, 24, 32.
    int ColorBits() const {return m_bpp;}
    /// Bytes every scan line (the value is upper 4-bytes rounded).
    int GetPitch() const {return 4 * ((m_width * m_bpp + 31) / 32);}
    //@}

    /// @name Color Table (bpp == 8).
    //@{
    /// Get palette.
    void GetColorTable (RGBQUAD pColors[256]) const ;
    /// Set palette.
    void SetColorTable (const RGBQUAD pColors[256]) ;
    //@}

    /// @name Color Convert.
    //@{
    /// Convert current image's bpp to 24.
    void ConvertTo24Bit() {ConvertToTrueColor(24);}
    /// Convert current image's bpp to 32 (alpha channel will be set 0xFF).
    void ConvertTo32Bit() {ConvertToTrueColor(32);}
    //@}

    /// @name Transform.
    //@{
    /// Swap current image with img.
    void SwapImage (FCObjImage& img) ;
    /// Get region of image, rcRegion must inside image.
    bool GetRegion (FCObjImage* pImg, RECT rcRegion) const ;
    /// Cover image, align top-left point of img at (xDest,yDest).
    bool CoverBlock (const FCObjImage& img, int xDest, int yDest) ;
    /// Combine img32 on point (xDest,yDest), current image and img32 must be 32bpp.
    void CombineImage (const FCObjImage& img32, int xDest, int yDest, int nAlphaPercent=100) ;
    /// Stretch image.
    void Stretch (int nNewWidth, int nNewHeight) ;
    /// Stretch image using interpolation ( bpp >= 24 ).
    void Stretch_Smooth (int nNewWidth, int nNewHeight, FCProgressObserver* pProgress=0) ;
    /// Apply an effect, more detail refer to FCImageEffect.
    void ApplyEffect (FCImageEffect& rEffect, FCProgressObserver* pProgress=0) ;
    //@}

    /**
        @name Read / Write file.
        default to determine image format by file's ext name.
    */
    //@{
    /// Load image from file.
    bool Load (const wchar_t* szFileName, FCImageProperty* pProperty=0) ;
    /// Load image from memory.
    bool Load (const void* pStart, int nMemSize, IMAGE_TYPE imgType, FCImageProperty* pProperty=0) ;
    /**
        Save image to file \n
        JPG - nFlag is compress quality 1 <= n <= 100, default is -1, means quality 90 \n
        GIF - nFlag is transparent color's index in palette.
    */
    bool Save (const wchar_t* szFileName, int nFlag=-1) const
    {
        // save property
        FCImageProperty   imgProp ;
        imgProp.m_SaveFlag = nFlag ;
        return Save (szFileName, imgProp) ;
    }
    /// Save image to file.
    bool Save (const wchar_t* szFileName, const FCImageProperty& rProp) const ;
    //@}

    /**
        Set image codec factory \n
        you must use <B>new</B> to create factory, system is responsible for deleting it.
    */
    static void SetImageCodecFactory (FCImageCodecFactory* pFactory) {g_codec_factory().reset(pFactory);}
    /// Get image codec factory.
    static FCImageCodecFactory* GetImageCodecFactory() {return g_codec_factory().get();}
    /**
        Load multi-page image, such as a GIF image \n
        caller must <b>delete</b> all image in rImageList.
    */
    static bool Load (const wchar_t* szFileName, std::vector<FCObjImage*>& rImageList, FCImageProperty* pProperty=0) ;
    /**
        Load multi-page image, such as a GIF image \n
        caller must <b>delete</b> all image in rImageList.
    */
    static bool Load (const void* pStart, int nMemSize, IMAGE_TYPE imgType, std::vector<FCObjImage*>& rImageList, FCImageProperty* pProperty=0) ;

private:
    int         m_width ;
    int         m_height ;
    int         m_bpp ;
    BYTE      * m_pixel ; // pixel data, from point bottom-left start
    BYTE     ** m_line_ptr ;  // line-pointer, array from top to bottom
    RGBQUAD   * m_palette ; // palette
#if (defined(_WIN32) || defined(__WIN32__))
    HBITMAP     m_DIB_Handle ;
#else
    int         m_DIB_Handle ; // it's only a dummy
#endif

private:
    void InitMember() ;
    void AllocPixelBuffer() ;
    void FreePixelBuffer() ;
    void ConvertToTrueColor (int nColor) ; // nColor == 24 or 32

    static std::auto_ptr<FCImageCodecFactory>& g_codec_factory() ;
    static bool IsRectInRect (RECT rcOut, RECT rcIn) ;

    void HorizonResize (FCObjImage& imgMid, const FCFilterBase& rFilter, FCProgressObserver* pProgress) const ;
    void VerticalResize (const FCObjImage& imgMid, const FCFilterBase& rFilter, FCProgressObserver* pProgress) const ;
};

//-------------------------------------------------------------------------------------
// inline Implement
//-------------------------------------------------------------------------------------
inline void FCObjImage::InitMember()
{
    m_width=0 ; m_height=0 ; m_bpp=0 ;
    m_pixel=0 ; m_line_ptr=0 ; m_palette=0 ;
    m_DIB_Handle=0 ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::SwapImage (FCObjImage& img)
{
    std::swap (img.m_width, m_width) ;
    std::swap (img.m_height, m_height) ;
    std::swap (img.m_bpp, m_bpp) ;
    std::swap (img.m_pixel, m_pixel) ;
    std::swap (img.m_line_ptr, m_line_ptr) ;
    std::swap (img.m_palette, m_palette) ;
    std::swap (img.m_DIB_Handle, m_DIB_Handle) ;
}
//-------------------------------------------------------------------------------------
inline FCObjImage::FCObjImage()
{
    InitMember() ;
}
inline FCObjImage::FCObjImage (const FCObjImage& img)
{
    InitMember() ;
    *this = img ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::AllocPixelBuffer()
{
#if (defined(_WIN32) || defined(__WIN32__))

    int   nSize = sizeof(BITMAPINFOHEADER) + 16 ;
    if (ColorBits() <= 8)
    {
        nSize += (sizeof(RGBQUAD) * 256) ;
    }

    std::vector<BYTE>   temp_buf ((size_t)nSize, (BYTE)0) ;

    BITMAPINFO   * pInfo = (BITMAPINFO*)&temp_buf[0] ;
    pInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER) ;
    pInfo->bmiHeader.biWidth = Width() ;
    pInfo->bmiHeader.biHeight = Height() ;
    pInfo->bmiHeader.biPlanes = 1 ;
    pInfo->bmiHeader.biBitCount = (WORD)ColorBits() ;
    m_DIB_Handle = CreateDIBSection (NULL, pInfo, DIB_RGB_COLORS, (VOID**)&m_pixel, NULL, 0) ;
    ZeroMemory (m_pixel, GetPitch() * Height()) ;

#else

    m_pixel = (BYTE*)calloc (GetPitch() * Height(), 1) ;

#endif
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::FreePixelBuffer()
{
    if (!m_pixel)
        return ;

#if (defined(_WIN32) || defined(__WIN32__))

    DeleteObject(m_DIB_Handle) ;

#else

    free (m_pixel) ;

#endif
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Create (int nWidth, int nHeight, int nColorBit)
{
    Destroy() ;

    // unsupport format
    if ((nHeight <= 0) || (nWidth <= 0))
    {
        assert(false); return false;
    }
    switch (nColorBit)
    {
        case 8 :
        case 24 :
        case 32 : break ;
        default : assert(false); return false; // not support 1,4,16 bpp
    }

    // init
    m_width  = nWidth ;
    m_height = nHeight ;
    m_bpp    = nColorBit ;

    // alloc pixel buffer, pixel must initialized to zero !
    AllocPixelBuffer() ;

    // create a line pointer, to accelerate pixel access
    m_line_ptr = (BYTE**) new BYTE [sizeof(BYTE*) * Height()] ;

    int   nPitch = GetPitch() ;
    m_line_ptr[0] = m_pixel + (Height() - 1) * nPitch ;
    for (int y=1 ; y < Height() ; y++)
        m_line_ptr[y] = m_line_ptr[y - 1] - nPitch ;

    // 8bit color image set a gray palette default
    if (ColorBits() <= 8)
    {
        m_palette = new RGBQUAD[256] ;

        // calculate gray palette
        RGBQUAD   pPal[256] ;
        for (int i=0 ; i < 256 ; i++)
        {
            pPal[i] = FCColor(i,i,i) ;
        }
        SetColorTable (pPal) ;
    }
    return true ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::Destroy()
{
    if (m_line_ptr)
        delete[] m_line_ptr ;

    FreePixelBuffer() ;

    if (m_palette)
        delete[] m_palette ;

    InitMember() ;
}
//-------------------------------------------------------------------------------------
inline BYTE* FCObjImage::GetBits (int nLine) const
{
    assert (IsInside(0,nLine)) ;
    return m_line_ptr[nLine] ;
}
//-------------------------------------------------------------------------------------
inline BYTE* FCObjImage::GetBits (int x, int y) const
{
    assert (IsInside(x,y)) ;
    if (ColorBits() == 32)
        return (m_line_ptr[y] + x * 4) ;
    if (ColorBits() == 8)
        return (m_line_ptr[y] + x) ;
    return (m_line_ptr[y] + x * 3) ; // 24bpp
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::GetColorTable (RGBQUAD pColors[256]) const
{
    if (IsValidImage() && (ColorBits() == 8) && pColors && m_palette)
    {
        memcpy (pColors, m_palette, 256 * sizeof(RGBQUAD)) ;
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::SetColorTable (const RGBQUAD pColors[256])
{
    if (IsValidImage() && (ColorBits() == 8) && pColors && m_palette)
    {
        memcpy (m_palette, pColors, 256 * sizeof(RGBQUAD)) ;

#if (defined(_WIN32) || defined(__WIN32__))
        ::SetDIBColorTable (FCImageDrawDC(*this), 0, 256, pColors) ;
#endif
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::ConvertToTrueColor (int nColor)
{
    if (!IsValidImage() || (ColorBits() == nColor))
        return ;

    // backup image
    FCObjImage   imgOld ;
    SwapImage(imgOld) ;

    if (!Create (imgOld.Width(), imgOld.Height(), nColor))
        return ;

    // get palette
    std::vector<RGBQUAD>   pPal ;
    if (imgOld.ColorBits() <= 8)
    {
        pPal.resize (256) ;
        imgOld.GetColorTable (&pPal[0]) ;
    }

    // start color convert
    for (int y=0 ; y < Height() ; y++)
    {
        for (int x=0 ; x < Width() ; x++)
        {
            BYTE   * pOld = imgOld.GetBits(x,y) ;
            BYTE   * pNew = GetBits(x,y) ;
            switch (imgOld.ColorBits())
            {
                case 8 : // 8 --> 24,32
                    FCColor::CopyPixelBPP (pNew, &pPal[*pOld], 24) ;
                    break ;
                case 24 :
                case 32 : // 24,32 --> 32,24
                    FCColor::CopyPixelBPP (pNew, pOld, 24) ;
                    break ;
            }
            if (nColor == 32)
                PCL_A(pNew) = 0xFF ;
        }
    }
}
//-------------------------------------------------------------------------------------
inline FCObjImage& FCObjImage::operator= (const FCObjImage& img)
{
    Destroy() ;
    if (!img.IsValidImage() || (&img == this))
        return (*this) ;

    if (Create (img.Width(), img.Height(), img.ColorBits()))
    {
        // copy pixels
        memcpy (GetMemStart(), img.GetMemStart(), img.GetPitch()*img.Height()) ;

        // copy palette
        if (img.ColorBits() <= 8)
        {
            RGBQUAD   pPal[256] ;
            img.GetColorTable (pPal) ;
            SetColorTable (pPal) ;
        }

        // copy position
        FCObjGraph::operator=(img) ;
    }
    return *this ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::GetRegion (FCObjImage* pImg, RECT rcRegion) const
{
    RECT   rcImage = {0, 0, Width(), Height()} ;
    if (!IsValidImage() || !pImg || (pImg == this) || !IsRectInRect (rcImage, rcRegion))
    {
        assert(false); return false;
    }

    if (!pImg->Create (rcRegion.right-rcRegion.left, rcRegion.bottom-rcRegion.top, ColorBits()))
        return false ;

    // copy pixel
    for (int i=0 ; i < pImg->Height() ; i++)
    {
        memcpy (pImg->GetBits(i), GetBits(rcRegion.left, rcRegion.top + i), pImg->Width() * ColorBits() / 8) ;
    }

    // copy palette
    if (ColorBits() <= 8)
    {
        RGBQUAD   pPal[256] ;
        GetColorTable (pPal) ;
        pImg->SetColorTable (pPal) ;
    }
    return true ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::CoverBlock (const FCObjImage& img, int xDest, int yDest)
{
    if (!IsValidImage() || !img.IsValidImage() || (ColorBits() != img.ColorBits()))
    {
        assert(false); return false;
    }

    // calculate covered RECT
    RECT   rcImage = {0, 0, Width(), Height()},
           rcCover = {xDest, yDest, xDest+img.Width(), yDest+img.Height()} ;
    RECT   rc ;
    if (!IntersectRect (&rc, &rcImage, &rcCover))
        return false ; // rect of destination is empty

    // copy pixel
    for (int y=rc.top ; y < rc.bottom ; y++)
    {
        BYTE   * pS = img.GetBits (rc.left-xDest, y-yDest) ; // calculate edge
        memcpy (GetBits (rc.left, y), pS, (rc.right-rc.left) * img.ColorBits() / 8) ;
    }
    return true ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::CombineImage (const FCObjImage& img32, int xDest, int yDest, int nAlphaPercent)
{
    nAlphaPercent = FClamp (nAlphaPercent, 0, 100) ;

    RECT   rcImg = {0, 0, Width(), Height()},
           rcCover = {xDest, yDest, xDest+img32.Width(), yDest+img32.Height()},
           rc ;

    if ( ::IntersectRect (&rc, &rcImg, &rcCover) &&
         IsValidImage() && img32.IsValidImage() &&
         (img32.ColorBits() == 32) && (ColorBits() == 32) )
    {
        for (int y=rc.top ; y < rc.bottom ; y++)
        {
            RGBQUAD   * pDest = (RGBQUAD*)this->GetBits (rc.left, y),
                      * pSrc = (RGBQUAD*)img32.GetBits (rc.left-xDest, y-yDest) ; // calculate edge
            for (int x=rc.left ; x < rc.right ; x++, pDest++, pSrc++)
            {
                RGBQUAD   cr = *pSrc ;
                if (nAlphaPercent != 100)
                {
                    PCL_A(&cr) = (BYTE)(PCL_A(&cr) * nAlphaPercent / 100) ;
                }
                *pDest = FCColor::CombinePixel(*pDest, cr) ;
            }
        }
    }
    else
    {
        assert(false) ;
    }
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Load (const wchar_t* szFileName, std::vector<FCObjImage*>& rImageList, FCImageProperty* pProperty)
{
    bool   bRet = false ;

    // ensure image property exist
    std::auto_ptr<FCImageProperty>   tmpProp (new FCImageProperty) ;
    if (!pProperty)
    {
        pProperty = tmpProp.get() ;
    }
    else
    {
        *pProperty = *tmpProp ; // clear
    }

    IMAGE_TYPE     imgType = GetImageCodecFactory()->QueryImageFileType(szFileName) ;
    std::auto_ptr<FCImageCodec>   pCodec (GetImageCodecFactory()->CreateImageCodec(imgType)) ;
    if (pCodec.get())
    {
        bRet = pCodec->LoadImageFile (szFileName, rImageList, *pProperty) ;
    }
    assert (bRet) ;
    return bRet ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Load (const void* pStart, int nMemSize, IMAGE_TYPE imgType, std::vector<FCObjImage*>& rImageList, FCImageProperty* pProperty)
{
    bool   bRet = false ;

    // ensure image property exist
    std::auto_ptr<FCImageProperty>   tmpProp (new FCImageProperty) ;
    if (!pProperty)
    {
        pProperty = tmpProp.get() ;
    }
    else
    {
        *pProperty = *tmpProp ; // clear
    }

    std::auto_ptr<FCImageCodec>   pCodec (GetImageCodecFactory()->CreateImageCodec(imgType)) ;
    if (pCodec.get())
    {
        bRet = pCodec->LoadImageMemory (pStart, nMemSize, rImageList, *pProperty) ;
    }
    assert (bRet) ;
    return bRet ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Load (const wchar_t* szFileName, FCImageProperty* pProperty)
{
    std::vector<FCObjImage*>   listImage ;
    bool   b = FCObjImage::Load (szFileName, listImage, pProperty) ;

    if (listImage.size())
    {
        SwapImage (*listImage[0]) ;
    }
    std::for_each (listImage.begin(), listImage.end(), FCDeleteEachObject()) ;
    return b ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Load (const void* pStart, int nMemSize, IMAGE_TYPE imgType, FCImageProperty* pProperty)
{
    std::vector<FCObjImage*>   listImage ;
    bool   b = FCObjImage::Load (pStart, nMemSize, imgType, listImage, pProperty) ;

    if (listImage.size())
    {
        SwapImage (*listImage[0]) ;
    }
    std::for_each (listImage.begin(), listImage.end(), FCDeleteEachObject()) ;
    return b ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::Save (const wchar_t* szFileName, const FCImageProperty& rProp) const
{
    bool   bRet = false ;
    if (IsValidImage())
    {
        IMAGE_TYPE     imgType = GetImageCodecFactory()->QueryImageFileType(szFileName) ;
        std::auto_ptr<FCImageCodec>   pCodec (GetImageCodecFactory()->CreateImageCodec(imgType)) ;
        if (pCodec.get())
        {
            std::vector<const FCObjImage*>   saveList ;
            saveList.push_back (this) ;
            bRet = pCodec->SaveImageFile (szFileName, saveList, rProp) ;
        }
    }
    assert (bRet) ;
    return bRet ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::IsRectInRect (RECT rcOut, RECT rcIn)
{
    return (rcIn.left >= rcOut.left) && (rcIn.top >= rcOut.top) && (rcIn.right <= rcOut.right) && (rcIn.bottom <= rcOut.bottom) ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::Stretch (int nNewWidth, int nNewHeight)
{
    if (!IsValidImage() || (nNewWidth <= 0) || (nNewHeight <= 0))
        return ;
    if ((nNewWidth == Width()) && (nNewHeight == Height()))
        return ;

    // first backup image
    FCObjImage   imgOld ;
    SwapImage(imgOld) ;

    if (!Create (nNewWidth, nNewHeight, imgOld.ColorBits()))
    {
        assert(false); return;
    }

    // duplicate palette
    if (ColorBits() <= 8)
    {
        RGBQUAD   pPal[256] ;
        imgOld.GetColorTable (pPal) ;
        SetColorTable (pPal) ;
    }

    std::vector<int>   temp_buf (Width()) ;

    // initialize index table
    int   * pX = &temp_buf[0] ;
    for (int x=0 ; x < Width() ; x++)
    {
        pX[x] = x * imgOld.Width() / Width() ; // omit fraction
    }

    int   nSpan = ColorBits() / 8 ;
    for (int y=0 ; y < Height() ; y++)
    {
        int    sy = y * imgOld.Height() / Height() ;
        BYTE   * pPixel = GetBits (y) ;
        for (int x=0 ; x < Width() ; x++)
        {
            FCColor::CopyPixelBPP (pPixel, imgOld.GetBits(pX[x], sy), ColorBits()) ;
            pPixel += nSpan ;
        }
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::Stretch_Smooth (int nNewWidth, int nNewHeight, FCProgressObserver* pProgress)
{
    if (!IsValidImage() || (nNewWidth <= 0) || (nNewHeight <= 0) || (ColorBits() < 24))
    {
        assert(false) ; return ;
    }
    if ((nNewWidth == Width()) && (nNewHeight == Height()))
        return ;

    FCObjImage   mid ;
    mid.Create (nNewWidth, Height(), ColorBits()) ;

    HorizonResize (mid, FCFilterCatmullRom(), pProgress) ;

    Create (nNewWidth, nNewHeight, ColorBits()) ;
    VerticalResize (mid, FCFilterCatmullRom(), pProgress) ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::HorizonResize (FCObjImage& imgMid, const FCFilterBase& rFilter, FCProgressObserver* pProgress) const
{
    double   dScale = imgMid.Width() / (double)Width() ;
    double   dWidth ;
    double   dFScale ;

    if (dScale < 1.0) 
    {
        // minification
        dWidth = rFilter.GetWidth() / dScale ;
        dFScale = dScale ;
    }
    else
    {
        // magnification
        dWidth = rFilter.GetWidth() ;
        dFScale = 1.0 ;
    }

    // mid image same height to old
    for (int x=0 ; x < imgMid.Width() ; x++)
    {
        double   dCenter = x / dScale ;

        int   nLeft = (int)floor(dCenter - dWidth),
              nRight = (int)ceil(dCenter + dWidth) ;

        // init weight table
        double   * wTab = new double[nRight - nLeft + 1] ;
        double   dTotalWeight = 0 ;
        for (int i=nLeft ; i <= nRight ; i++)
        {
            double   d = dFScale * rFilter.Filter (dFScale * (dCenter - i)) ;
            wTab[i - nLeft] = d ;
            dTotalWeight += d ;
        }
        for (int i=nLeft ; i <= nRight ; i++)
            wTab[i - nLeft] /= dTotalWeight ;

        for (int y=0 ; y < imgMid.Height() ; y++)
        {
            double   b=0, g=0, r=0, a=0 ;
            for (int i=nLeft ; i <= nRight ; i++)
            {
                int    nSX = FClamp (i, 0, Width()-1) ;
                BYTE   * p = GetBits (nSX, y) ;

                double   aw = ((ColorBits() == 32) ? PCL_A(p) : 0xFF) * wTab[i - nLeft] ;
                b += (aw * PCL_B(p)) ;
                g += (aw * PCL_G(p)) ;
                r += (aw * PCL_R(p)) ;
                a += aw ;
            }

            // set pixel
            RGBQUAD   cr ;
            PCL_A(&cr) = FClamp0255(a) ;
            if (PCL_A(&cr))
            {
                PCL_B(&cr) = FClamp0255(b/a) ;
                PCL_G(&cr) = FClamp0255(g/a) ;
                PCL_R(&cr) = FClamp0255(r/a) ;
            }
            FCColor::CopyPixelBPP (imgMid.GetBits(x,y), &cr, ColorBits()) ;
        }
        delete[] wTab ;

        // update progress
        if (pProgress)
        {
            if (!pProgress->UpdateProgress (50 * (x+1) / imgMid.Width()))
                break ;
        }
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::VerticalResize (const FCObjImage& imgMid, const FCFilterBase& rFilter, FCProgressObserver* pProgress) const
{
    double   dScale = Height() / (double)imgMid.Height() ;
    double   dWidth ;
    double   dFScale ;

    if (dScale < 1.0) 
    {
        // minification
        dWidth = rFilter.GetWidth() / dScale ;
        dFScale = dScale ;
    }
    else
    {
        // magnification
        dWidth = rFilter.GetWidth() ;
        dFScale = 1.0 ;
    }

    for (int y=0 ; y < Height() ; y++)
    {
        double   dCenter = y / dScale ;

        int   nTop = (int)floor(dCenter - dWidth),
              nBottom = (int)ceil(dCenter + dWidth) ;

        // init weight table
        double   * wTab = new double[nBottom - nTop + 1] ;
        double   dTotalWeight = 0 ;
        for (int i=nTop ; i <= nBottom ; i++)
        {
            double   d = dFScale * rFilter.Filter (dFScale * (i - dCenter)) ;
            wTab[i - nTop] = d ;
            dTotalWeight += d ;
        }
        for (int i=nTop ; i <= nBottom ; i++)
            wTab[i - nTop] /= dTotalWeight ;

        for (int x=0 ; x < Width() ; x++)
        {
            double   b=0, g=0, r=0, a=0 ;
            for (int i=nTop ; i <= nBottom ; i++)
            {
                int    nSY = FClamp (i, 0, imgMid.Height()-1) ;
                BYTE   * p = imgMid.GetBits (x, nSY) ;

                double   aw = ((ColorBits() == 32) ? PCL_A(p) : 0xFF) * wTab[i - nTop] ;
                b += (aw * PCL_B(p)) ;
                g += (aw * PCL_G(p)) ;
                r += (aw * PCL_R(p)) ;
                a += aw ;
            }

            // set pixel
            RGBQUAD   cr ;
            PCL_A(&cr) = FClamp0255(a) ;
            if (PCL_A(&cr))
            {
                PCL_B(&cr) = FClamp0255(b/a) ;
                PCL_G(&cr) = FClamp0255(g/a) ;
                PCL_R(&cr) = FClamp0255(r/a) ;
            }
            FCColor::CopyPixelBPP (GetBits(x,y), &cr, ColorBits()) ;
        }
        delete[] wTab ;

        // update progress
        if (pProgress)
        {
            if (!pProgress->UpdateProgress (50 + 50 * (y+1) / Height()))
                break ;
        }
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::ApplyEffect (FCImageEffect& rEffect, FCProgressObserver* pProgress)
{
    if (!rEffect.IsSupport (*this))
    {
        assert(false); return;
    }

    // before
    rEffect.OnBeforeProcess (*this) ;
    if (pProgress)
        pProgress->UpdateProgress(0) ;

    switch (rEffect.QueryProcessType())
    {
        case FCImageEffect::PROCESS_TYPE_PIXEL :
            {
                int   nSpan = ColorBits()/8 ;
                for (int y=0 ; y < Height() ; y++)
                {
                    BYTE   * pCurr = GetBits(y) ;
                    for (int x=0 ; x < Width() ; x++)
                    {
                        rEffect.ProcessPixel (*this, x, y, pCurr) ;
                        pCurr += nSpan ;
                    }

                    // update progress
                    if (pProgress)
                    {
                        if (!pProgress->UpdateProgress ((y+1) * 100 / Height()))
                            break ;
                    }
                }
            }
            break ;

        case FCImageEffect::PROCESS_TYPE_WHOLE :
            rEffect.ProcessWholeImage (*this, pProgress) ;
            break ;
    }
}

#if (defined(_WIN32) || defined(__WIN32__))
    #include "image_win.inl"
#endif

#endif
