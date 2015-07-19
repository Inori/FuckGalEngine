/*
	Copyright (C) =USTC= Fu Li

	Author   :  Fu Li
	Create   :  2005-7-29
	Home     :  http://www.phoxo.com
	Mail     :  crazybitwps@hotmail.com

	This file is part of ImageStone

	The code distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	Redistribution and use the source code, with or without modification,
	must retain the above copyright.
*/
#ifndef IMAGE_EFFECT_2005_07_29_H
#define IMAGE_EFFECT_2005_07_29_H
#include "../image.h"

// standard effect
class FCEffectBlur_Gauss ; // gauss blur (>=24 bit)
class FCEffectGrayscale ; // gray scale (>=24 bit)
class FCEffectFillBackGround ; // blend image with specified color (32 bit).
class FCEffectFillColor ; // fill color (>=24 bit)
class FCEffectPremultipleAlpha ; // premultiple image's alpha (32 bit)
class FCEffectMirror ; // mirror (>=24 bit)
class FCEffectFlip ; // flip (>=24 bit)
class FCEffectThreshold ; // threshold (>=24 bit)
class FCEffectAdjustRGB ; // adjust RGB channel (>=24 bit)
class FCEffectRotate90 ; // clockwise rotate 90 degrees (>=24 bit)
    class FCEffectRotate270 ; // clockwise rotate 270 degrees (>=24 bit)
class FCEffectGetHistogram ; // get histogram (>=24 bit)
class FCEffectMosaic ; // mosaic (>=24 bit)
class FCEffectHueSaturation ; // hue and saturation (>=24 bit)
class FCEffectAddShadow ; // add shadow (32 bit)
class FCEffectGradient ; // base class of gradient fill (>=24 bit)
    class FCEffectGradientLinear ; // gradient fill linear (>=24 bit)
        class FCEffectGradientBiLinear ; // gradient fill bilinear (>=24 bit)
        class FCEffectGradientConicalSym ; // gradient fill symmetric conical (>=24 bit)
        class FCEffectGradientConicalASym ; // gradient fill anti-symmetric conical (>=24 bit)
    class FCEffectGradientRect ; // gradient fill rect (>=24 bit)
    class FCEffectGradientRadial ; // gradient fill radial (>=24 bit)
class FCEffectLUT ; // LUT(look up table) routine (>=24 bit)
    class FCEffectBrightnessContrast ; // brightness and contrast (>=24 bit)
    class FCEffectGamma ; // adjust gamma (>=24 bit)
    class FCEffectInvert ; // negate (>=24 bit)
    class FCEffectSolarize ; // solarize (>=24 bit)
    class FCEffectPosterize ; // posterize (>=24 bit)
class FCEffectColorTone ; // color tone (>=24 bit)
class FCEffectFillGrid ; // fill grid (>=24 bit)
class FCEffectSplash ; // splash (>=24 bit)

// extend effect
class FCEffectFillPattern ; // fill pattern (32 bit)
class FCEffectShift ; // shift (>=24 bit)
class FCEffectAutoContrast ; // auto contrast stretch (>=24 bit)
class FCEffectAutoColorEnhance ; // auto stretch saturation (>=24 bit)
class FCEffectEmboss ; // emboss (>=24 bit)
class FCEffectHalftoneM3 ; // halftone (>=24 bit)
class FCEffectOilPaint ; // oil paint (>=24 bit)
class FCEffectNoisify ; // noisify (>=24 bit)
class FCEffectVideo ; // video (>=24 bit)
class FCEffectColorBalance ; // color balance (>=24 bit)
class FCEffect3DGrid ; // add 3D grid (>=24 bit)
class FCEffectIllusion ; // illusion (>=24 bit)
class FCEffectBlinds ; // blinds (>=24 bit)
class FCEffectRaiseFrame ; // add raise frame (>=24 bit)
class FCEffectPatternFrame ; // add pattern frame (>=24 bit)
class FCEffectExportAscII ; // save a ASCII text file (>=24 bit)
class FCEffectConvolute ; // convolute (>= 24 bit)
    class FCEffectSharp ; // sharp (laplacian template) (>=24 bit)
class FCEffectColorLevel ; // color level (>=24 bit)
    class FCEffectAutoColorLevel ; // auto color level (>=24 bit)
class FCEffectBilinearDistort ; // bilinear distort (>=24 bit)
    class FCEffectWave ; // wave (>=24 bit)
    class FCEffectBulge ; // bulge and pinch (>=24 bit)
    class FCEffectTwist ; // twist (>=24 bit)
    class FCEffectShear ; // shear transform (>=24 bit)
    class FCEffectPerspective ; // perspective transform (>=24 bit)
    class FCEffectRotate ; // rotate (>=24 bit)
    class FCEffectRibbon ; // ribbon (>=24 bit)
    class FCEffectRipple ; // ripple (>=24 bit)
class FCEffectBlur_Gauss ; // gauss blur (>=24 bit)
    class FCEffectSoftGlow ; // soft glow (>=24 bit)
    class FCEffectUNSharpMask ; // unsharp mask (>=24 bit)
    class FCEffectPencilSketch ; // pencil sketch (>=24 bit)
    class FCEffectOldPhoto ; // old photo (>=24 bit)
    class FCEffectSoftPortrait ; // soft portrait (>=24 bit)
class FCEffectReduceNoise ; // reduce noise (>=24 bit)
class FCEffectLensFlare ; // lens flare effect (>=24 bit)
class FCEffectTileReflection ; // tile reflection effect (>=24 bit)
class FCEffectBlur_Zoom ; // blur zoom (>=24 bit)
class FCEffectBlur_Radial ; // blur radial (>=24 bit)
class FCEffectBlur_Motion ; // blur motion (>=24 bit)
class FCEffectSmoothEdge ; // smooth edge (32 bit)

#include "blur_gauss.h"
//-------------------------------------------------------------------------------------
/// Gray scale (>=24 bit).
class FCEffectGrayscale : public FCImageEffect
{
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        PCL_R(pPixel) = PCL_G(pPixel) = PCL_B(pPixel) = FCColor::GetGrayscale(pPixel) ;
    }
public:
    /// Constructor.
    FCEffectGrayscale() {}
};
//-------------------------------------------------------------------------------------
/// Blend image with specified color (32 bit).
class FCEffectFillBackGround : public FCImageEffect
{
    RGBQUAD   m_color ;
public:
    /// Constructor.
    FCEffectFillBackGround (RGBQUAD c) : m_color(c) {}
private:
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() == 32) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        RGBQUAD   c = *(RGBQUAD*)pPixel ;
        *(RGBQUAD*)pPixel = m_color ;
        FCColor::AlphaBlendPixel (pPixel, c) ;
    }
};
//-------------------------------------------------------------------------------------
/// Fill color (>=24 bit).
class FCEffectFillColor : public FCImageEffect
{
    RGBQUAD  m_color ;
    int      m_alpha ;
    int      m_is_fill_alpha ;
public:
    /**
        Constructor\n
        nAlpha - set -1 means not change alpha.
    */
    FCEffectFillColor (RGBQUAD crFill, int nAlpha=-1) : m_color(crFill), m_alpha(nAlpha), m_is_fill_alpha(false) {}
private:
    virtual bool IsSupport (const FCObjImage& img)
    {
        if (img.IsValidImage() && (img.ColorBits() >= 24))
        {
            m_is_fill_alpha = ((m_alpha != -1) && (img.ColorBits() == 32)) ;
            if (m_is_fill_alpha)
            {
                PCL_A(&m_color) = FClamp0255(m_alpha) ;
            }
            return true ;
        }
        return false ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        FCColor::CopyPixelBPP (pPixel, &m_color, m_is_fill_alpha ? 32 : 24) ;
    }
};
//-------------------------------------------------------------------------------------
/// Premultiple image's alpha (32 bit).
class FCEffectPremultipleAlpha : public FCImageEffect
{
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() == 32) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   a = PCL_A(pPixel) ;
        PCL_R(pPixel) = (BYTE)(PCL_R(pPixel) * a / 255) ;
        PCL_G(pPixel) = (BYTE)(PCL_G(pPixel) * a / 255) ;
        PCL_B(pPixel) = (BYTE)(PCL_B(pPixel) * a / 255) ;
    }
public:
    /// Constructor.
    FCEffectPremultipleAlpha() {}
};
//-------------------------------------------------------------------------------------
/// Left right mirror (>=24 bit).
class FCEffectMirror : public FCImageEffect
{
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (x < img.Width()/2)
        {
            BYTE   * pRight = img.GetBits (img.Width()-1-x, y) ;
            for (int i=0 ; i < img.ColorBits()/8 ; i++)
            {
                std::swap (pPixel[i], pRight[i]) ;
            }
        }
    }
public:
    /// Constructor.
    FCEffectMirror() {}
};
//-------------------------------------------------------------------------------------
/// Top bottom flip (>=24 bit).
class FCEffectFlip : public FCImageEffect
{
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (y < img.Height()/2)
        {
            BYTE   * pBottom = img.GetBits (x, img.Height()-1-y) ;
            for (int i=0 ; i < img.ColorBits()/8 ; i++)
            {
                std::swap (pPixel[i], pBottom[i]) ;
            }
        }
    }
public:
    /// Constructor.
    FCEffectFlip() {}
};
//-------------------------------------------------------------------------------------
/// Threshold (>=24 bit).
class FCEffectThreshold : public FCImageEffect
{
    int   m_level ;
public:
    /**
        Constructor \n
        0 <= nLevel <= 255
    */
    FCEffectThreshold (int nLevel) : m_level(FClamp0255(nLevel)) {}
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int      g = FCColor::GetGrayscale(pPixel) ;
        RGBQUAD  c = ((g > m_level) ? FCColor(0xFF,0xFF,0xFF) : FCColor(0,0,0)) ;
        FCColor::CopyPixelBPP (pPixel, &c, 24) ;
    }
};
//-------------------------------------------------------------------------------------
/// Adjust RGB channel (>=24 bit).
class FCEffectAdjustRGB : public FCImageEffect
{
    int   m_r, m_g, m_b ;
public:
    /// Constructor.
    FCEffectAdjustRGB (int nR, int nG, int nB) : m_r(nR), m_g(nG), m_b(nB) {}
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        PCL_B(pPixel) = FClamp0255 (PCL_B(pPixel) + m_b) ;
        PCL_G(pPixel) = FClamp0255 (PCL_G(pPixel) + m_g) ;
        PCL_R(pPixel) = FClamp0255 (PCL_R(pPixel) + m_r) ;
    }
};
//-------------------------------------------------------------------------------------
/// Clockwise rotate 90 degrees (>=24 bit).
class FCEffectRotate90 : public FCImageEffect
{
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        img.SwapImage(m_bak) ;

        // create new rotated image
        img.Create (m_bak.Height(), m_bak.Width(), m_bak.ColorBits()) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        FCColor::CopyPixelBPP (pPixel, m_bak.GetBits (y, img.Width()-1-x), img.ColorBits()) ;
    }
protected:
    FCObjImage   m_bak ;
public:
    /// Constructor.
    FCEffectRotate90() {}
};
//-------------------------------------------------------------------------------------
/// Clockwise rotate 270 degrees (>=24 bit).
class FCEffectRotate270 : public FCEffectRotate90
{
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        FCColor::CopyPixelBPP (pPixel, m_bak.GetBits (img.Height()-1-y, x), img.ColorBits()) ;
    }
public:
    /// Constructor.
    FCEffectRotate270() {}
};
//-------------------------------------------------------------------------------------
/// Get histogram (>=24 bit).
class FCEffectGetHistogram : public FCImageEffect
{
public:
    /// red channel
    std::vector<int>   m_red ;
    /// green channel
    std::vector<int>   m_green ;
    /// blue channel
    std::vector<int>   m_blue ;

    /// Constructor.
    FCEffectGetHistogram() : m_red((size_t)256,0), m_green((size_t)256,0), m_blue((size_t)256,0) {}

private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        m_blue[PCL_B(pPixel)]++ ;
        m_green[PCL_G(pPixel)]++ ;
        m_red[PCL_R(pPixel)]++ ;
    }
};
//-------------------------------------------------------------------------------------
#include "mosaic.h"
#include "hue_saturation.h"
#include "shadow.h"
#include "gradient.h"
//-------------------------------------------------------------------------------------
/// LUT(look up table) routine (>=24 bit)
class FCEffectLUT : public FCImageEffect
{
    int   m_LUT[256] ;
public:
    /**
        Initialize table \n
        0 <= nLUTIndex <= 0xFF
    */
    virtual int InitLUTtable (int nLUTIndex) =0 ;

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        for (int i=0 ; i <= 0xFF ; i++)
            m_LUT[i] = InitLUTtable (i) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        PCL_B(pPixel) = (BYTE)m_LUT[PCL_B(pPixel)] ;
        PCL_G(pPixel) = (BYTE)m_LUT[PCL_G(pPixel)] ;
        PCL_R(pPixel) = (BYTE)m_LUT[PCL_R(pPixel)] ;
    }
};
//-------------------------------------------------------------------------------------
/// Brightness and contrast (>=24 bit).
class FCEffectBrightnessContrast : public FCEffectLUT
{
    int   m_brightness ;
    int   m_contrast ;
public:
    /**
        Constructor \n
        param -100 <= nBrightness <= 100, 0 means not change \n
        param -100 <= nContrast <= 100, 0 means not change
    */
    FCEffectBrightnessContrast (int nBrightness, int nContrast)
    {
        m_brightness = FClamp(nBrightness, -100, 100) ;
        m_contrast = FClamp(nContrast, -100, 100) ;
    }
private:
    virtual int InitLUTtable (int nLUTIndex)
    {
        double  d = (100 + m_contrast) / 100.0 ;
        int     n = (int)((nLUTIndex-128) * d + (m_brightness + 128) + 0.5) ;
        return FClamp0255(n) ;
    }
};
//-------------------------------------------------------------------------------------
/// Adjust gamma (>=24 bit).
class FCEffectGamma : public FCEffectLUT
{
    double   m_fInvGamma ;
public:
    /**
        Constructor \n
        nGamma >= 1, 100 means not change
    */
    FCEffectGamma (int nGamma)
    {
        nGamma = ((nGamma >= 1) ? nGamma : 1) ;
        m_fInvGamma = 1.0 / (nGamma / 100.0) ;
    }
private:
    virtual int InitLUTtable (int nLUTIndex)
    {
        double   fMax = pow (255.0, m_fInvGamma) / 255.0 ;
        double   d = pow((double)nLUTIndex, m_fInvGamma) ;
        return FClamp0255(d / fMax) ;
    }
};
//-------------------------------------------------------------------------------------
/// Negate (>=24 bit).
class FCEffectInvert : public FCEffectLUT
{
    virtual int InitLUTtable (int nLUTIndex)
    {
        return (255 - nLUTIndex) ;
    }
public:
    /// Constructor.
    FCEffectInvert() {}
};
//-------------------------------------------------------------------------------------
/// Solarize (>=24 bit).
class FCEffectSolarize : public FCEffectLUT
{
    int   m_threshold ;
public:
    /**
        Constructor \n
        0 <= nThreshold <= 0xFF
    */
    FCEffectSolarize (int nThreshold) : m_threshold(FClamp0255(nThreshold)) {}
private:
    virtual int InitLUTtable (int nLUTIndex)
    {
        return (nLUTIndex >= m_threshold) ? (255 - nLUTIndex) : nLUTIndex ;
    }
};
//-------------------------------------------------------------------------------------
/// Posterize (>=24 bit).
class FCEffectPosterize : public FCEffectLUT
{
    int   m_level ;
public:
    /**
        Constructor \n
        nLevel >= 2
    */
    FCEffectPosterize (int nLevel)
    {
        m_level = ((nLevel >= 2) ? nLevel : 2) ;
    }
private:
    virtual int InitLUTtable (int nLUTIndex)
    {
        double  d = 255.0 / (m_level - 1.0) ;
        int     n = (int)(nLUTIndex / d + 0.5) ; // round
        return FClamp0255 (d * n) ; // round
    }
};
//-------------------------------------------------------------------------------------
/// Color tone (>=24 bit).
class FCEffectColorTone : public FCImageEffect
{
    double   m_hue ;
    double   m_saturation ;
    double   m_lum_tab[256] ;
public:
    /**
        Constructor \n
        0 <= nSaturation <= 0xFF
    */
    FCEffectColorTone (RGBQUAD crTone, int nSaturation)
    {
        double   l ;
        FCColor::RGBtoHLS (&crTone, m_hue, l, m_saturation) ;

        m_saturation = m_saturation * (nSaturation/255.0) * (nSaturation/255.0) ;
        m_saturation = ((m_saturation < 1) ? m_saturation : 1) ;

        for (int i=0 ; i < 256 ; i++)
        {
            RGBQUAD   cr = FCColor(i,i,i) ;
            double    h, l, s ;
            FCColor::RGBtoHLS (&cr, h, l, s) ;

            l = l * (1 + (128-abs(nSaturation-128)) / 128.0 / 9.0) ;
            m_lum_tab[i] = ((l < 1) ? l : 1) ;
        }
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double    l = m_lum_tab[FCColor::GetGrayscale(pPixel)] ;
        RGBQUAD   cr = FCColor::HLStoRGB (m_hue, l, m_saturation) ;
        FCColor::CopyPixelBPP (pPixel, &cr, 24) ;
    }
};
//-------------------------------------------------------------------------------------
/// Fill grid (>=24 bit).
class FCEffectFillGrid : public FCImageEffect
{
    RGBQUAD  m_cr1, m_cr2 ;
    int      m_size ;
public:
    /**
        Constructor \n
        nSize - width of grid, >=1
    */
    FCEffectFillGrid (RGBQUAD cr1, RGBQUAD cr2, int nSize, int nAlpha=0xFF)
    {
        m_size = ((nSize >= 1) ? nSize : 1) ;
        m_cr1 = cr1 ;
        m_cr2 = cr2 ;
        PCL_A(&m_cr1) = PCL_A(&m_cr2) = FClamp0255(nAlpha) ;
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int      nX = x / m_size, nY = y / m_size ;
        RGBQUAD  c = (((nX + nY) % 2 == 0) ? m_cr1 : m_cr2) ;
        if (img.ColorBits() == 32)
        {
            *(RGBQUAD*)pPixel = FCColor::CombinePixel (*(RGBQUAD*)pPixel, c) ;
        }
        else
        {
            FCColor::CopyPixelBPP (pPixel, &c, 24) ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Splash (>=24 bit).
class FCEffectSplash : public FCImageEffect
{
    int         m_size ;
    FCObjImage  m_bak ;
public:
    /**
        Constructor \n
        nSize >= 3
    */
    FCEffectSplash (int nSize)
    {
        m_size = ((nSize >= 3) ? nSize : 3) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   xs = x - m_size/2 + (rand() % m_size),
              ys = y - m_size/2 + (rand() % m_size) ;
        xs = FClamp (xs, 0, img.Width()-1) ;
        ys = FClamp (ys, 0, img.Height()-1) ;
        FCColor::CopyPixelBPP (pPixel, m_bak.GetBits(xs,ys), img.ColorBits()) ;
    }
};
//-------------------------------------------------------------------------------------
#ifdef IMAGESTONE_USE_EXT_EFFECT
    #include "effect_ext.h"
#endif
//-------------------------------------------------------------------------------------

#endif
