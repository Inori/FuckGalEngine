
//-------------------------------------------------------------------------------------
inline Gdiplus::Bitmap* FCObjImage::CreateBitmap (int nTransparencyIndex) const
{
    if (!IsValidImage())
    {
        assert(false); return NULL;
    }

    Gdiplus::PixelFormat   fmt = PixelFormat24bppRGB ;
    switch (ColorBits())
    {
        case 8 : fmt=PixelFormat8bppIndexed; break;
        case 24 : fmt=PixelFormat24bppRGB; break;
        case 32 : fmt=PixelFormat32bppARGB; break;
    }

    Gdiplus::Bitmap   * pBmp = new Gdiplus::Bitmap (Width(), Height(), fmt) ;

    // set palette
    if (ColorBits() == 8)
    {
        RGBQUAD   old_pal[256] ;
        GetColorTable (old_pal) ;

        std::vector<BYTE>   temp_buf (sizeof(Gdiplus::ColorPalette) + 256*sizeof(Gdiplus::ARGB)) ;

        Gdiplus::ColorPalette   * pPal = (Gdiplus::ColorPalette*)&temp_buf[0] ;
        pPal->Flags = Gdiplus::PaletteFlagsHasAlpha ;
        pPal->Count = 256 ;

        for (int i=0 ; i < 256 ; i++)
        {
            void   * p = &old_pal[i] ;
            int    nAlpha = ((i == nTransparencyIndex) ? 0 : 0xFF) ;
            pPal->Entries[i] = Gdiplus::Color::MakeARGB ((BYTE)nAlpha, PCL_R(p), PCL_G(p), PCL_B(p)) ;
        }
        pBmp->SetPalette (pPal) ;
    }

    // set pixel
    int   nLineByte = Width() * ColorBits() / 8 ;
    for (int y=0 ; y < Height() ; y++)
    {
        Gdiplus::Rect         rc (0, y, Width(), 1) ;
        Gdiplus::BitmapData   bd = {0} ;

        pBmp->LockBits (&rc, Gdiplus::ImageLockModeWrite, fmt, &bd) ;
        if (bd.Scan0)
        {
            CopyMemory (bd.Scan0, GetBits(y), nLineByte) ;
        }
        pBmp->UnlockBits (&bd) ;
    }
    return pBmp ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::FromHBITMAP (HBITMAP hBitmap)
{
    Destroy() ;

    DIBSECTION   ds = {0} ;
    if (!::GetObject (hBitmap, sizeof(ds), &ds))
        return false ;

    if (ds.dsBm.bmBits && (ds.dsBmih.biBitCount == 32) && (ds.dsBmih.biCompression == BI_RGB))
    {
        Create (ds.dsBm.bmWidth, ds.dsBm.bmHeight, 32) ;
        ::CopyMemory (GetMemStart(), ds.dsBm.bmBits, GetPitch() * Height()) ;
    }
    else
    {
        Create (ds.dsBm.bmWidth, ds.dsBm.bmHeight, 24) ;
        ::BitBlt (FCImageDrawDC(*this), 0, 0, Width(), Height(), FCImageDrawDC(hBitmap), 0, 0, SRCCOPY) ;
    }
    return IsValidImage() ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::FromBitmap (Gdiplus::Bitmap& gpBmp)
{
    Destroy() ;

    if (!gpBmp.GetWidth())
        return false ;

    Gdiplus::PixelFormat   fmtWant = PixelFormat24bppRGB ;
    int   nBpp = 24 ;

    if ( (gpBmp.GetFlags() & Gdiplus::ImageFlagsHasAlpha) ||
         (gpBmp.GetPixelFormat() == PixelFormat32bppARGB) )
    {
        fmtWant = PixelFormat32bppARGB ;
        nBpp = 32 ;
    }
    else
    {
        // 32bpp bmp file will be loaded as PixelFormat32bppRGB
        GUID   file_format = {0} ;
        gpBmp.GetRawFormat (&file_format) ;
        if ( IsEqualGUID(file_format, Gdiplus::ImageFormatBMP) &&
             (gpBmp.GetPixelFormat() == PixelFormat32bppRGB) )
        {
            fmtWant = PixelFormat32bppRGB ;
            nBpp = 32 ;
        }
    }

    if (Create (gpBmp.GetWidth(), gpBmp.GetHeight(), nBpp))
    {
        // get pixel
        for (int y=0 ; y < Height() ; y++)
        {
            Gdiplus::Rect         rc (0, y, Width(), 1) ;
            Gdiplus::BitmapData   bd = {0} ;
            bd.Width = Width() ;
            bd.Height = 1 ;
            bd.Stride = GetPitch() ;
            bd.PixelFormat = fmtWant ;
            bd.Scan0 = GetBits(y) ;

            Gdiplus::Status   b = gpBmp.LockBits (&rc, Gdiplus::ImageLockModeRead|Gdiplus::ImageLockModeUserInputBuf, fmtWant, &bd) ;
            gpBmp.UnlockBits (&bd) ;

            // error when read scanline
            if (b != Gdiplus::Ok)
            {
                assert(false) ;
                break ;
            }
        }
    }
    return IsValidImage() ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::LoadBitmap (UINT nID, HINSTANCE hInstance)
{
    Destroy() ;

    if (!hInstance)
        hInstance = (HINSTANCE)::GetModuleHandle(NULL) ;

    HBITMAP  hBmp = ::LoadBitmap (hInstance, MAKEINTRESOURCE(nID)) ;
    bool     ret = FromHBITMAP(hBmp) ;
    ::DeleteObject(hBmp) ;
    return ret ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::LoadResource (UINT nID, LPCTSTR resource_type, IMAGE_TYPE image_type, HMODULE hModule)
{
    Destroy() ;

    if (!hModule)
        hModule = ::GetModuleHandle(NULL) ;

    HRSRC   hRes = ::FindResource (hModule, MAKEINTRESOURCE(nID), resource_type) ;
    if (hRes)
    {
        int    nResSize = ::SizeofResource (hModule, hRes) ;
        void   * pData = ::LockResource (::LoadResource (hModule, hRes)) ;
        if (pData)
        {
            Load (pData, nResSize, image_type) ;
        }
    }
    return IsValidImage() ;
}
//-------------------------------------------------------------------------------------
inline bool FCObjImage::GetClipboard()
{
    Destroy() ;

    if (!::OpenClipboard (NULL))
        return false ;

    BITMAPINFOHEADER   * bmif = (BITMAPINFOHEADER*)::GetClipboardData(CF_DIB) ;

    // 32bit with alpha
    if (bmif && (bmif->biBitCount == 32) && (bmif->biCompression == BI_RGB))
    {
        Create (bmif->biWidth, bmif->biHeight, 32) ;
        CopyMemory (GetMemStart(), (BYTE*)bmif + bmif->biSize, GetPitch()*Height()) ;
    }
    else
    {
        FromHBITMAP ((HBITMAP)::GetClipboardData(CF_BITMAP)) ;
    }
    ::CloseClipboard() ;
    return IsValidImage() ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::CopyToClipboard() const
{
    if (!IsValidImage())
        return ;

    const FCObjImage   * pImg = this ;

    FCObjImage   img24 ;
    if (ColorBits() < 24)
    {
        img24 = *this ;
        img24.ConvertTo24Bit() ;
        pImg = &img24 ;
    }

    // make DIB to clipboard
    HGLOBAL   hMem = GlobalAlloc (GMEM_MOVEABLE, pImg->GetPitch()*pImg->Height() + sizeof(BITMAPINFOHEADER)) ;

    // set info
    BITMAPINFOHEADER   * pInfo = (BITMAPINFOHEADER*)GlobalLock(hMem) ;
    ZeroMemory (pInfo, sizeof(BITMAPINFOHEADER)) ;
    pInfo->biSize = sizeof(BITMAPINFOHEADER) ;
    pInfo->biWidth = pImg->Width() ;
    pInfo->biHeight = pImg->Height() ;
    pInfo->biPlanes = 1 ;
    pInfo->biBitCount = (WORD)pImg->ColorBits() ;
    pInfo++ ;
    CopyMemory (pInfo, pImg->GetMemStart(), pImg->GetPitch()*pImg->Height()) ;
    GlobalUnlock (hMem) ;

    if (::OpenClipboard (NULL))
    {
        ::EmptyClipboard() ;
        if (::SetClipboardData (CF_DIB, hMem))
        {
            hMem = NULL ;
        }
        ::CloseClipboard() ;
    }

    if (hMem)
    {
        GlobalFree (hMem) ;
        assert(false);
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::CaptureDC (HDC hdc, RECT rect_on_dc)
{
    Destroy() ;

    if (Create (rect_on_dc.right-rect_on_dc.left, rect_on_dc.bottom-rect_on_dc.top, 24))
    {
        BitBlt (FCImageDrawDC(*this), 0, 0, Width(), Height(), hdc, rect_on_dc.left, rect_on_dc.top, SRCCOPY) ;
    }
}
//-------------------------------------------------------------------------------------
inline HRGN FCObjImage::CreateHRGN() const
{
    if (!IsValidImage())
        return NULL ;

    HRGN   hRgn = ::CreateRectRgn (0, 0, Width(), Height()) ;
    for (int y=0 ; y < Height() ; y++)
    {
        int    nLastX = 0 ;
        BOOL   bStartCount = FALSE ;
        for (int x=0 ; x < Width() ; x++)
        {
            if (PCL_B(GetBits(x,y)))
            {
                if (bStartCount)
                {
                    // erase rect
                    HRGN   hSingle = ::CreateRectRgn (nLastX, y, x, y+1) ;
                    ::CombineRgn (hRgn, hRgn, hSingle, RGN_DIFF) ;
                    ::DeleteObject (hSingle) ;
                    bStartCount = FALSE ;
                }
            }
            else
            {
                if (!bStartCount)
                {
                    bStartCount = TRUE ;
                    nLastX = x ;
                }
            }
        }
        if (bStartCount)
        {
            // erase rect
            HRGN   hSingle = ::CreateRectRgn (nLastX, y, Width(), y+1) ;
            ::CombineRgn (hRgn, hRgn, hSingle, RGN_DIFF) ;
            ::DeleteObject (hSingle) ;
        }
    }
    return hRgn ;
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::Draw (HDC hdc, RECT rect_on_dc, RECT* pOnImage) const
{
    if (!IsValidImage())
        return ;

    if (ColorBits() <= 24)
    {
        FCImageDrawDC::DrawBitmap (hdc, rect_on_dc, *this, pOnImage) ;
    }
    else
    {
        BLENDFUNCTION   bf = {0} ;
        bf.BlendOp = AC_SRC_OVER ;
        bf.SourceConstantAlpha = 255 ;
        bf.AlphaFormat = AC_SRC_ALPHA ;

        RECT   rcImg = {0, 0, Width(), Height()} ;
        if (pOnImage)
        {
            rcImg = *pOnImage ;
        }

        ::AlphaBlend (hdc, rect_on_dc.left, rect_on_dc.top,
                      rect_on_dc.right - rect_on_dc.left,
                      rect_on_dc.bottom - rect_on_dc.top,
                      FCImageDrawDC(*this), rcImg.left, rcImg.top,
                      rcImg.right - rcImg.left,
                      rcImg.bottom - rcImg.top, bf) ;
    }
}
//-------------------------------------------------------------------------------------
inline void FCObjImage::Draw (HDC hdc, int x, int y) const
{
    RECT   rcOnDC = {x, y, x+Width(), y+Height()} ;
    Draw (hdc, rcOnDC) ;
}
//-------------------------------------------------------------------------------------
