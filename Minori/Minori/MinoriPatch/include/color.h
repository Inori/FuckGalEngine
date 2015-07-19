/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2003-4-17
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef COLOR_HELPER_2003_04_17_H
#define COLOR_HELPER_2003_04_17_H
#include "define.h"

//-------------------------------------------------------------------------------------
/**
    Color helper.
*/
class FCColor : public RGBQUAD
{
#ifdef IMAGESTONE_DOXYGEN_PROCESS
    int   m_doxygen ;
#endif

public:
    /// Constructor.
    FCColor (int r, int g, int b, int a=0xFF)
    {
        rgbRed = (BYTE)r ;
        rgbGreen = (BYTE)g ;
        rgbBlue = (BYTE)b ;
        rgbReserved = (BYTE)a ;
    }

#if (defined(_WIN32) || defined(__WIN32__))
    /// Constructor.
    FCColor (COLORREF c)
    {
        rgbRed = GetRValue(c) ;
        rgbGreen = GetGValue(c) ;
        rgbBlue = GetBValue(c) ;
        rgbReserved = 0xFF ;
    }
#endif

    /**
        Combine two 32bpp pixel \n
        c1 - background \n
        c2 - foreground.
    */
    static RGBQUAD CombinePixel (RGBQUAD c1, RGBQUAD c2)
    {
        if (PCL_A(&c1) || PCL_A(&c2))
        {
            if (PCL_A(&c2) == 0)
            {
                return c1 ;
            }
            if ((PCL_A(&c1) == 0) || (PCL_A(&c2) == 0xFF))
            {
                return c2 ;
            }

            RGBQUAD   ret ;

            // needn't bound in [0,0xFF], i have checked :-)
            int   t1 = 0xFF * PCL_A(&c1),
                  t2 = 0xFF * PCL_A(&c2),
                  k = t1 - PCL_A(&c1) * PCL_A(&c2), // >= 0
                  nTemp = t2 + k ; // ==0 only alpha1=0 & alpha2=0
            PCL_B(&ret) = (BYTE)((t2*PCL_B(&c2) + k*PCL_B(&c1)) / nTemp) ;
            PCL_G(&ret) = (BYTE)((t2*PCL_G(&c2) + k*PCL_G(&c1)) / nTemp) ;
            PCL_R(&ret) = (BYTE)((t2*PCL_R(&c2) + k*PCL_R(&c1)) / nTemp) ;
            PCL_A(&ret) = (BYTE)(nTemp / 0xFF) ;

            /*
            // un-optimized, easier to read
            int    nTemp = 0xFF * (PCL_A(&c1) + PCL_A(&c2)) - PCL_A(&c1)*PCL_A(&c2) ;
            PCL_B(&ret) = (0xFF*PCL_B(c2)*PCL_A(&c2) + (0xFF - PCL_A(&c2))*PCL_B(&c1)*PCL_A(&c1)) / nTemp ;
            PCL_G(&ret) = (0xFF*PCL_G(c2)*PCL_A(&c2) + (0xFF - PCL_A(&c2))*PCL_G(&c1)*PCL_A(&c1)) / nTemp ;
            PCL_R(&ret) = (0xFF*PCL_R(c2)*PCL_A(&c2) + (0xFF - PCL_A(&c2))*PCL_R(&c1)*PCL_A(&c1)) / nTemp ;
            PCL_A(&ret) = nTemp / 0xFF ;
            */
            return ret ;
        }
        return FCColor(0xFF,0xFF,0xFF,0) ;
    }

    /**
        AlphaBlend cr on pDest, put result into pDest \n
        pDest - address of 24bpp or 32bpp pixel \n
        cr - foreground.
    */
    static void AlphaBlendPixel (void* pDest, RGBQUAD cr)
    {
        int   a = PCL_A(&cr) ;
        if (a == 0xFF)
        {
            PCL_B(pDest) = PCL_B(&cr) ;
            PCL_G(pDest) = PCL_G(&cr) ;
            PCL_R(pDest) = PCL_R(&cr) ;
            return ;
        }
        if (a == 0)
            return ;

        int   t = 0xFF - a ;
        PCL_B(pDest) = (BYTE)((PCL_B(&cr) * a + PCL_B(pDest) * t) / 0xFF) ;
        PCL_G(pDest) = (BYTE)((PCL_G(&cr) * a + PCL_G(pDest) * t) / 0xFF) ;
        PCL_R(pDest) = (BYTE)((PCL_R(&cr) * a + PCL_R(pDest) * t) / 0xFF) ;
    }

    /**
        Calculate grayscale value of pixel \n
        prgb - address of 24bpp or 32bpp pixel.
    */
    static BYTE GetGrayscale (const void* prgb)
    {
        return (BYTE)((30*PCL_R(prgb) + 59*PCL_G(prgb) + 11*PCL_B(prgb)) / 100) ;
    }

    /**
        Fast pixel copy \n
        nBPP - 8, 16, 24, 32
    */
    static void CopyPixelBPP (void* pDest, const void* pSrc, int nBPP)
    {
        if (nBPP == 32)
            *(DWORD*)pDest = *(DWORD*)pSrc ;
        else if (nBPP == 24)
        {
            *(WORD*)pDest = *(WORD*)pSrc ;
            ((BYTE*)pDest)[2] = ((BYTE*)pSrc)[2] ;
        }
        else if (nBPP == 8)
            *(BYTE*)pDest = *(BYTE*)pSrc ;
        else if (nBPP == 16)
            *(WORD*)pDest = *(WORD*)pSrc ;
        else
        {
            assert(false) ;
        }
    }

    /**
        Remove red eye \n
        pPixel - address of 24bpp or 32bpp pixel \n
        param -100 <= nThreshold <= 100
    */
    static void RemoveRedEye (void* pPixel, int nThreshold)
    {
        const double   RED_FACTOR = 0.5133333 ;

        double   r = PCL_R(pPixel) * RED_FACTOR ;
        double   g = PCL_G(pPixel) * 1 ; // green factor
        double   b = PCL_B(pPixel) * 0.1933333 ; // blue factor

        if ((r >= g - nThreshold) && (r >= b - nThreshold))
        {
            PCL_R(pPixel) = FClamp0255 ((g + b) / (2 * RED_FACTOR)) ;
        }
    }

    /// @name RGB <--> HLS (Hue, Lightness, Saturation).
    //@{
    /**
        RGB --> HLS \n
        prgb - address of 24bpp or 32bpp pixel.
    */
    static void RGBtoHLS (const void* prgb, double& H, double& L, double& S)
    {
        int   buf[] = {PCL_R(prgb), PCL_G(prgb), PCL_B(prgb)} ;
        int   n_cmax = *std::max_element(buf, buf+3) ;
        int   n_cmin = *std::min_element(buf, buf+3) ;

        L = (n_cmax + n_cmin) / 2.0 / 255.0 ;

        if (n_cmax == n_cmin)
        {
            S = H = 0.0 ;
            return ;
        }

        double   r = PCL_R(prgb) / 255.0,
                 g = PCL_G(prgb) / 255.0,
                 b = PCL_B(prgb) / 255.0,
                 cmax = n_cmax / 255.0,
                 cmin = n_cmin / 255.0,
                 delta = cmax - cmin ;

        if (L < 0.5) 
            S = delta / (cmax + cmin) ;
        else
            S = delta / (2.0 - cmax - cmin) ;

        if (PCL_R(prgb) == n_cmax)
            H = (g-b) / delta ;
        else if (PCL_G(prgb) == n_cmax)
            H = 2.0 + (b-r) / delta ;
        else
            H = 4.0 + (r-g) / delta ;

        H /= 6.0 ;

        if (H < 0.0)
            H += 1.0 ;
    }

    /// HLS --> RGB.
    static RGBQUAD HLStoRGB (double H, double L, double S)
    {
        if ((!(S > 0)) && (!(S < 0))) // == 0
            return DoubleRGB_to_RGB (L, L, L) ;

        double   m1, m2 ;
        if (L > 0.5)
            m2 = L + S - L*S ;
        else
            m2 = L * (1.0 + S) ;
        m1 = 2.0*L - m2 ;

        double   r = HLS_Value (m1, m2, H*6.0 + 2.0) ;
        double   g = HLS_Value (m1, m2, H*6.0) ;
        double   b = HLS_Value (m1, m2, H*6.0 - 2.0) ;
        return DoubleRGB_to_RGB (r,g,b) ;
    }
    //@}

    /// @name RGB <--> HSV (Hue, Saturation, Value).
    //@{
    /**
        RGB --> HSV \n
        prgb - address of 24bpp or 32bpp pixel.
    */
    static void RGBtoHSV (const void* prgb, double& H, double& S, double& V)
    {
        int   buf[] = {PCL_R(prgb), PCL_G(prgb), PCL_B(prgb)} ;
        int   n_cmax = *std::max_element(buf, buf+3) ;
        int   n_cmin = *std::min_element(buf, buf+3) ;

        double   r = PCL_R(prgb) / 255.0,
                 g = PCL_G(prgb) / 255.0,
                 b = PCL_B(prgb) / 255.0,
                 delta = (n_cmax - n_cmin) / 255.0 ;

        V = n_cmax / 255.0 ;

        if (n_cmax == n_cmin)
        {
            S = H = 0.0 ;
            return ;
        }

        S = (n_cmax - n_cmin) / (double)n_cmax ;
        if (PCL_R(prgb) == n_cmax)
            H = (g - b) / delta ;
        else if (PCL_G(prgb) == n_cmax)
            H = 2.0 + (b - r) / delta ;
        else if (PCL_B(prgb) == n_cmax)
            H = 4.0 + (r - g) / delta ;

        H /= 6.0 ;
        if (H < 0.0)
            H += 1.0 ;
        else if (H > 1.0)
            H -= 1.0 ;
    }

    /// HSV --> RGB.
    static RGBQUAD HSVtoRGB (double h, double s, double v)
    {
        if ((!(s > 0)) && (!(s < 0))) // == 0
            return DoubleRGB_to_RGB (v, v, v) ;

        if (!(h < 1)) // >= 1
            h = 0.0 ;
        h *= 6.0 ;

        double  f = h - (int)h,
                p = v * (1.0 - s),
                q = v * (1.0 - s * f),
                t = v * (1.0 - s * (1.0 - f)) ;
        switch ((int)h)
        {
            case 0 : return DoubleRGB_to_RGB (v, t, p) ;
            case 1 : return DoubleRGB_to_RGB (q, v, p) ;
            case 2 : return DoubleRGB_to_RGB (p, v, t) ;
            case 3 : return DoubleRGB_to_RGB (p, q, v) ;
            case 4 : return DoubleRGB_to_RGB (t, p, v) ;
            case 5 : return DoubleRGB_to_RGB (v, p, q) ;
        }
        return DoubleRGB_to_RGB (0, 0, 0) ;
    }
    //@}

    /**
        Calculate bilinear interpolation \n
        0 <= x,y < 1, distance to crPixel[0,0] \n
        nBpp - can be 24 or 32 \n
        crPixel - in order [0,0], [1,0], [0,1], [1,1].
    */
    static RGBQUAD GetBilinear (double x, double y, int nBpp, BYTE* crPixel[4])
    {
        const BYTE  * px0 = crPixel[0], * px1 = crPixel[1],
                    * px2 = crPixel[2], * px3 = crPixel[3] ;
        RGBQUAD     crRet = {0xFF, 0xFF, 0xFF, 0xFF} ;

        if ((nBpp == 24) || ((PCL_A(px0) & PCL_A(px1) & PCL_A(px2) & PCL_A(px3)) == 0xFF))
        {
            // is 24bit or all alpha of pixel is 0xFF
            for (int i=0 ; i < 3 ; i++)
            {
                double   m0 = px0[i] + x * (px1[i] - px0[i]),
                         m1 = px2[i] + x * (px3[i] - px2[i]),
                         my = m0 + y * (m1 - m0) ;
                ((BYTE*)&crRet)[i] = FClamp0255(my) ;
            }
        }
        else
        {
            // is 32bit with alpha, calculate destinate alpha value
            double   m0 = PCL_A(px0) + x * (PCL_A(px1) - PCL_A(px0)),
                     m1 = PCL_A(px2) + x * (PCL_A(px3) - PCL_A(px2)),
                     my = m0 + y * (m1 - m0),
                     dAlpha = my ;
            PCL_A(&crRet) = FClamp0255(my) ;
            if (PCL_A(&crRet)) // has alpha
            {
                for (int i=0 ; i < 3 ; i++)
                {
                    double   nSum0 = PCL_A(px0) * px0[i],
                             nSum2 = PCL_A(px2) * px2[i] ;

                    m0 = nSum0 + x * (px1[i] * PCL_A(px1) - nSum0) ;
                    m1 = nSum2 + x * (px3[i] * PCL_A(px3) - nSum2) ;
                    my = m0 + y * (m1 - m0) ;
                    ((BYTE*)&crRet)[i] = FClamp0255(my / dAlpha) ;
                }
            }
        }
        return crRet ;
    }

private:
    static double HLS_Value (double n1, double n2, double h)
    {
        if (h > 6.0)
            h -= 6.0 ;
        else if (h < 0.0)
            h += 6.0 ;

        if (h < 1.0)
            return n1 + (n2 - n1) * h ;
        else if (h < 3.0)
            return n2 ;
        else if (h < 4.0)
            return n1 + (n2 - n1) * (4.0 - h) ;
        return n1 ;
    }

    static RGBQUAD DoubleRGB_to_RGB (double r, double g, double b)
    {
        return FCColor (FClamp0255(r*255), FClamp0255(g*255), FClamp0255(b*255)) ;
    }
};

#endif
