#ifndef IMAGESTONE_STANDARD_DEFINE_H
#define IMAGESTONE_STANDARD_DEFINE_H

#ifdef FREEIMAGE_H
    #define IMAGESTONE_USE_FREEIMAGE
#endif

#if (defined(_WIN32) || defined(__WIN32__))

    #include <windows.h>

    #if (!defined(__MINGW32__) && !defined(__CYGWIN__))
        #include <comdef.h> // for SDK app, must include this before include GDI+
    #endif

    // use GDI+ default on windows
    #include <GdiPlus.h>
    #pragma comment (lib, "GdiPlus.lib")

    #pragma comment (lib, "Ole32.lib")
    #pragma comment (lib, "Msimg32.lib")

#else

    // Linux, Unix, Mac... must include FreeImage.h before include ImageStone.h
    #include "linux_define.inl"

#endif

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <memory.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

//-------------------------------------------------------------------------------------
// basic function
//-------------------------------------------------------------------------------------
// bound in [tLow, tHigh]
template<class T> inline T FClamp (const T& t, const T& tLow, const T& tHigh)
{
    if (t < tHigh)
    {
        return ((t > tLow) ? t : tLow) ;
    }
    return tHigh ;
}
inline BYTE FClamp0255 (int n)
{
    return (BYTE)(int)FClamp (n, 0, 0xFF) ;
}
// if d is very large, such as 2e+20, FClamp0255((int)d) will get result 0, so we must clamp to [0,255] first, then convert to int.
inline BYTE FClamp0255 (const double& d)
{
    return (BYTE)(int)(FClamp(d, 0.0, 255.0) + 0.5) ;
}

#define   LIB_PI   3.14159265358979323846

struct FCDeleteEachObject
{
    template<class T>
    void operator()(T*& p)
    {
        delete p ;
        p = 0 ;
    }
};

//-------------------------------------------------------------------------------------
// using these macro to avoid memory big/little endian
#define   PCL_R(p)   (((RGBQUAD*)(p))->rgbRed)
#define   PCL_G(p)   (((RGBQUAD*)(p))->rgbGreen)
#define   PCL_B(p)   (((RGBQUAD*)(p))->rgbBlue)
#define   PCL_A(p)   (((RGBQUAD*)(p))->rgbReserved)
//-------------------------------------------------------------------------------------
// image format
enum IMAGE_TYPE
{
    IMG_UNKNOW,
    IMG_BMP,
    IMG_PCX,
    IMG_JPG,
    IMG_GIF,
    IMG_TGA,
    IMG_TIF,
    IMG_PNG,
    IMG_PSD,
    IMG_ICO,
    IMG_XPM,
    IMG_PHOXO,
    IMG_CUSTOM,
};
//-------------------------------------------------------------------------------------
/// Shadow data
struct SHADOWDATA
{
    /// blur level, default is 5
    int   m_smooth ;
    /// color of shadow, default is RGB(63,63,63)
    RGBQUAD   m_color ;
    /// 0 <= opacity <= 100, default is 75
    int   m_opacity ;
    /// x offset of shadow, default is 5
    int   m_offset_x ;
    /// y offset of shadow, default is 5
    int   m_offset_y ;

    SHADOWDATA()
    {
        m_smooth = 5 ;
        m_color.rgbRed = m_color.rgbGreen = m_color.rgbBlue = 63 ;
        m_color.rgbReserved = 0xFF ;
        m_opacity = 75 ;
        m_offset_x = m_offset_y = 5 ;
    }
};

#if (defined(_WIN32) || defined(__WIN32__))
//-------------------------------------------------------------------------------------
/**
    Memory DC with auto bitmap select ( <b>Win only</b> ).
@verbatim
example:
    HDC       dc = GetDC(NULL) ;
    HBITMAP   bmp = CreateCompatibleBitmap (dc, 300, 300) ;
    ReleaseDC (NULL, dc) ;
    {
        FCImageDrawDC   memDC (bmp) ;
        SetTextColor (memDC, RGB(255,255,255)) ;
        TextOutW (memDC, 0, 0, L"PhoXo", 5) ;
    }
    DeleteObject(bmp) ;
@endverbatim
*/
class FCImageDrawDC
{
    HDC      m_dc ;
    HGDIOBJ  m_old ;
public:
    /// Create memory dc and select hBmp in.
    FCImageDrawDC (HBITMAP hBmp)
    {
        m_dc = CreateCompatibleDC(NULL) ;
        m_old = SelectObject (m_dc, hBmp) ;
        SetBkMode (m_dc, TRANSPARENT) ;
        SetStretchBltMode (m_dc, COLORONCOLOR) ;
    }
    ~FCImageDrawDC()
    {
        SelectObject (m_dc, m_old) ;
        DeleteDC(m_dc) ;
    }

    /// Get HDC handle.
    operator HDC() const {return m_dc;}

    /// Draw bitmap.
    static void DrawBitmap (HDC hdc, RECT rect_on_dc, HBITMAP hBmp, RECT* rect_on_image=NULL)
    {
        BITMAP   bm = {0} ;
        if (!::GetObject (hBmp, sizeof(BITMAP), &bm))
            return ;

        RECT   rcImg = {0, 0, bm.bmWidth, bm.bmHeight} ;
        if (rect_on_image)
        {
            rcImg = *rect_on_image ;
        }

        int   nOldMode = SetStretchBltMode (hdc, COLORONCOLOR) ;
        StretchBlt (hdc, rect_on_dc.left, rect_on_dc.top,
                    rect_on_dc.right - rect_on_dc.left,
                    rect_on_dc.bottom - rect_on_dc.top,
                    FCImageDrawDC(hBmp), rcImg.left, rcImg.top,
                    rcImg.right - rcImg.left,
                    rcImg.bottom - rcImg.top,
                    SRCCOPY) ;
        SetStretchBltMode (hdc, nOldMode) ;
    }
};
#endif

#endif
