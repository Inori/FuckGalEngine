
//-------------------------------------------------------------------------------------
/// Fill pattern (32 bit).
class FCEffectFillPattern : public FCImageEffect
{
    std::auto_ptr<FCObjImage>   m_pattern ;
    int   m_alpha ;
    int   m_only_texture ;
    int   m_lock_alpha ;
public:
    /**
        Constructor \n
        pPattern - use <B>new</B> to create and don't delete it \n
        0 <= nAlpha <= 255 - transparency of pPattern\n
        bLockAlpha - set true will not change alpha.
    */
    FCEffectFillPattern (FCObjImage* pPattern, int nAlpha, bool bOnlyTexture, bool bLockAlpha=false) : m_pattern(pPattern)
    {
        if (pPattern)
            pPattern->ConvertTo24Bit() ;

        m_alpha = FClamp0255(nAlpha) ;
        m_only_texture = (bOnlyTexture ? 1 : 0) ;
        m_lock_alpha = (bLockAlpha ? 1 : 0) ;
    }
private:
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() == 32) && m_pattern.get() && m_pattern->IsValidImage() ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        BYTE   * pPat = m_pattern->GetBits (x % m_pattern->Width(), y % m_pattern->Height()) ;
        if (m_only_texture)
        {
            // calculate texture
            int   n = (PCL_B(pPat)+PCL_G(pPat)+PCL_R(pPat) - 384) * m_alpha / 765 ;
            PCL_B(pPixel) = FClamp0255 (PCL_B(pPixel) - n) ;
            PCL_G(pPixel) = FClamp0255 (PCL_G(pPixel) - n) ;
            PCL_R(pPixel) = FClamp0255 (PCL_R(pPixel) - n) ;
        }
        else
        {
            int       old_a = PCL_A(pPixel) ;
            RGBQUAD   c = FCColor(PCL_R(pPat),PCL_G(pPat),PCL_B(pPat),m_alpha) ;
            *(RGBQUAD*)pPixel = FCColor::CombinePixel (*(RGBQUAD*)pPixel, c) ;
            if (m_lock_alpha)
            {
                PCL_A(pPixel) = (BYTE)old_a ;
            }
        }
    }
};
//-------------------------------------------------------------------------------------
/// Shift (>=24 bit).
class FCEffectShift : public FCImageEffect
{
    int   m_amount ; // max shift pixel
    int   m_current ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        nAmount >= 2.
    */
    FCEffectShift (int nAmount)
    {
        m_amount = ((nAmount >= 2) ? nAmount : 2) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (x == 0)
        {
            m_current = (rand() % m_amount) * ((rand() % 2) ? 1 : -1) ;
        }

        int   sx = FClamp (x+m_current, 0, img.Width()-1) ;
        FCColor::CopyPixelBPP (pPixel, m_bak.GetBits(sx,y), img.ColorBits()) ;
    }
};
//-------------------------------------------------------------------------------------
/// Auto contrast stretch (>=24 bit).
class FCEffectAutoContrast : public FCImageEffect
{
    int   m_lut[256][3] ;
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        int   cMin[3] = {255, 255, 255} ;
        int   cMax[3] = {0, 0, 0} ;

        // Get minimum and maximum values for each channel
        for (int y=0 ; y < img.Height() ; y++)
        {
            for (int x=0 ; x < img.Width() ; x++)
            {
                BYTE   * pPixel = img.GetBits(x,y) ;
                for (int b=0 ; b < 3 ; b++)
                {
                    if (pPixel[b] < cMin[b])  cMin[b] = pPixel[b] ;
                    if (pPixel[b] > cMax[b])  cMax[b] = pPixel[b] ;
                }
            }
        }

        // Calculate LUTs with stretched contrast
        for (int b=0 ; b < 3 ; b++)
        {
            int   nRange = cMax[b] - cMin[b] ;
            if (nRange)
            {
                for (int x=cMin[b] ; x <= cMax[b] ; x++)
                    m_lut[x][b] = 255 * (x - cMin[b]) / nRange ;
            }
            else
                m_lut[cMin[b]][b] = cMin[b] ;
        }
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        for (int b=0 ; b < 3 ; b++)
            pPixel[b] = (BYTE)m_lut[pPixel[b]][b] ;
    }
public:
    /// Constructor.
    FCEffectAutoContrast() {}
};
//-------------------------------------------------------------------------------------
/// Auto stretch saturation (>=24 bit).
class FCEffectAutoColorEnhance : public FCImageEffect
{
    double   m_vhi, m_vlo ;
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_vhi = 0 ; m_vlo = 1 ;

        // Get minimum and maximum values
        for (int y=0 ; y < img.Height() ; y++)
        {
            for (int x=0 ; x < img.Width() ; x++)
            {
                BYTE   * pPixel = img.GetBits(x,y) ;
                int    c = 255 - PCL_B(pPixel),
                       m = 255 - PCL_G(pPixel),
                       y = 255 - PCL_R(pPixel),
                       k = c ;
                if (m < k)  k = m ;
                if (y < k)  k = y ;

                BYTE    byMap[4] = { (BYTE)(c-k), (BYTE)(m-k), (BYTE)(y-k) } ;
                double  h, z, v ;
                FCColor::RGBtoHSV (byMap, h, z, v) ;
                if (v > m_vhi)  m_vhi = v ;
                if (v < m_vlo)  m_vlo = v ;
            }
        }
    }
    virtual void ProcessPixel (FCObjImage& img, int nx, int ny, BYTE* pPixel)
    {
        int   c = 255 - PCL_B(pPixel),
              m = 255 - PCL_G(pPixel),
              y = 255 - PCL_R(pPixel),
              k = c ;
        if (m < k)  k = m ;
        if (y < k)  k = y ;

        BYTE    byMap[4] = { (BYTE)(c-k), (BYTE)(m-k), (BYTE)(y-k) } ;
        double  h, z, v ;
        FCColor::RGBtoHSV (byMap, h, z, v) ;
        if (m_vhi != m_vlo)
            v = (v-m_vlo) / (m_vhi-m_vlo) ;
        *(RGBQUAD*)byMap = FCColor::HSVtoRGB (h, z, v) ;
        c = byMap[0] ; m = byMap[1] ; y = byMap[2] ;
        c += k ; if (c > 255)  c = 255 ;
        m += k ; if (m > 255)  m = 255 ;
        y += k ; if (y > 255)  y = 255 ;
        PCL_B(pPixel) = (BYTE)(255 - c) ;
        PCL_G(pPixel) = (BYTE)(255 - m) ;
        PCL_R(pPixel) = (BYTE)(255 - y) ;
    }
public:
    /// Constructor.
    FCEffectAutoColorEnhance() {}
};
//-------------------------------------------------------------------------------------
/// Emboss (>=24 bit).
class FCEffectEmboss : public FCImageEffect
{
    double      m_weights[3][3] ;
    FCObjImage  m_bak ;
public:
    /**
        Constructor \n
        0 <= nAngle < 360.
    */
    FCEffectEmboss (int nAngle)
    {
        while (nAngle < 0)
            nAngle += 360 ;
        nAngle %= 360 ;

        double   r = LIB_PI * nAngle / 180.0,
                 dr = LIB_PI / 4.0 ;
        m_weights[0][0] = cos(r + dr) ;
        m_weights[0][1] = cos(r + 2.0*dr) ;
        m_weights[0][2] = cos(r + 3.0*dr) ;

        m_weights[1][0] = cos(r) ;
        m_weights[1][1] = 0 ;
        m_weights[1][2] = cos(r + 4.0*dr) ;

        m_weights[2][0] = cos(r - dr) ;
        m_weights[2][1] = cos(r - 2.0*dr) ;
        m_weights[2][2] = cos(r - 3.0*dr) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   s = 0 ;
        for (int dy=-1 ; dy <= 1 ; dy++)
        {
            for (int dx=-1 ; dx <= 1 ; dx++)
            {
                int   sx = FClamp(x+dx, 0, img.Width()-1) ;
                int   sy = FClamp(y+dy, 0, img.Height()-1) ;
                int   n = FCColor::GetGrayscale(m_bak.GetBits(sx,sy)) ;
                s += m_weights[dy+1][dx+1] * n ;
            }
        }
        PCL_R(pPixel) = PCL_G(pPixel) = PCL_B(pPixel) = FClamp0255(s + 128) ;
    }
};
//-------------------------------------------------------------------------------------
/// Halftone (>=24 bit).
class FCEffectHalftoneM3 : public FCImageEffect
{
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        const BYTE  s_BayerPattern[8][8] = // 64 level gray
        {
            0,32,8,40,2,34,10,42,
            48,16,56,24,50,18,58,26,
            12,44,4,36,14,46,6,38,
            60,28,52,20,62,30,54,22,
            3,35,11,43,1,33,9,41,
            51,19,59,27,49,17,57,25,
            15,47,7,39,13,45,5,37,
            63,31,55,23,61,29,53,21
        };
        int   gr = FCColor::GetGrayscale (pPixel) ;
        PCL_R(pPixel) = PCL_G(pPixel) = PCL_B(pPixel) = (((gr>>2) > s_BayerPattern[y&7][x&7]) ? 0xFF : 0) ;
    }
public:
    /// Constructor, use Limb Pattern M3 algorithm.
    FCEffectHalftoneM3() {}
};
//-------------------------------------------------------------------------------------
/// Noisify (>=24 bit).
class FCEffectNoisify : public FCImageEffect
{
    int   m_level ;
    int   m_random ;
public:
    /**
        Constructor \n
        1 <= nLevel <= 100 \n
        bRandom - add white noise when set false.
    */
    FCEffectNoisify (int nLevel, bool bRandom=false)
    {
        m_level = FClamp (nLevel, 1, 100) ;
        m_random = (bRandom ? 1 : 0) ;
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   n1, n2, n3 ;
        if (m_random)
        {
            n1=GenGauss(); n2=GenGauss(); n3=GenGauss();
        }
        else
        {
            n1=n2=n3=GenGauss() ;
        }
        PCL_B(pPixel) = FClamp0255 (PCL_B(pPixel) + n1) ;
        PCL_G(pPixel) = FClamp0255 (PCL_G(pPixel) + n2) ;
        PCL_R(pPixel) = FClamp0255 (PCL_R(pPixel) + n3) ;
    }
    int GenGauss()
    {
        double   d = (rand() + rand() + rand() + rand()) * 5.28596089837e-5 - 3.46410161514 ;
        return (int)(m_level * d * 127.0 / 100.0) ;
    }
};
//-------------------------------------------------------------------------------------
/// Add 3D grid (>=24 bit).
class FCEffect3DGrid : public FCImageEffect
{
    int   m_size ;
    int   m_depth ;
public:
    /**
        Constructor \n
        nSize >= 1
    */
    FCEffect3DGrid (int nSize, int nDepth)
    {
        m_size = ((nSize >= 1) ? nSize : 1) ;
        m_depth = nDepth ;
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   d = 0 ;
        if (((y-1) % m_size == 0) && (x % m_size) && ((x+1) % m_size))
            d = -m_depth ; // top
        else if (((y+2) % m_size == 0) && (x % m_size) && ((x+1) % m_size))
            d = m_depth ; // bottom
        else if (((x-1) % m_size == 0) && (y % m_size) && ((y+1) % m_size))
            d = m_depth ; // left
        else if (((x+2) % m_size == 0) && (y % m_size) && ((y+1) % m_size))
            d = -m_depth ; // right

        PCL_R(pPixel) = FClamp0255 (PCL_R(pPixel) + d) ;
        PCL_G(pPixel) = FClamp0255 (PCL_G(pPixel) + d) ;
        PCL_B(pPixel) = FClamp0255 (PCL_B(pPixel) + d) ;
    }
};
//-------------------------------------------------------------------------------------
/// Illusion (>=24 bit).
class FCEffectIllusion : public FCImageEffect
{
    double   m_amount ;
    double   m_scale ;
    double   m_offset ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        nAmount >= 1
    */
    FCEffectIllusion (int nAmount)
    {
        m_amount = LIB_PI / ((nAmount >= 1) ? nAmount : 1) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        double   d1 = img.Width() ;
        double   d2 = img.Height() ;
        m_scale = sqrt (d1*d1 + d2*d2) / 2.0 ;
        m_offset = (int)(m_scale / 2) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   cx = (x - img.Width() / 2.0) / m_scale ;
        double   cy = (y - img.Height() / 2.0) / m_scale ;

        double   angle = floor (atan2(cy,cx) / 2.0 / m_amount) * 2.0 * m_amount + m_amount ;
        double   radius = sqrt(cx*cx + cy*cy) ;

        int   xx = (int)(x - m_offset * cos (angle)) ;
        int   yy = (int)(y - m_offset * sin (angle)) ;
        xx = FClamp (xx, 0, img.Width()-1) ;
        yy = FClamp (yy, 0, img.Height()-1) ;

        BYTE   * pOld = m_bak.GetBits (xx, yy) ;
        for (int i=0 ; i < img.ColorBits()/8 ; i++)
            pPixel[i] = FClamp0255 (pPixel[i] + radius * (pOld[i] - pPixel[i])) ;
    }
};
//-------------------------------------------------------------------------------------
/// Blind (>=24 bit).
class FCEffectBlinds : public FCImageEffect
{
public:
    enum BLIND_TYPE
    {
        BLIND_X,
        BLIND_Y,
    };

    /**
        Constructor\n
        nDirect - can be FCEffectBlinds::BLIND_X or FCEffectBlinds::BLIND_Y\n
        nWidth >= 2, became scanline effect when set 2\n
        1 <= nOpacity <= 100
    */
    FCEffectBlinds (BLIND_TYPE nDirect, int nWidth, int nOpacity, RGBQUAD blind_color)
    {
        m_direct = nDirect ;
        m_width = ((nWidth >= 2) ? nWidth : 2) ;
        m_color = blind_color ;
        m_opacity = FClamp (nOpacity, 1, 100) ;
    }
private:
    BLIND_TYPE  m_direct ;
    int         m_width ;
    int         m_opacity ;
    RGBQUAD     m_color ;

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   nMod = 0 ;
        if (m_direct == BLIND_X) // horizontal direction
            nMod = y % m_width ;
        else if (m_direct == BLIND_Y) // vertical direction
            nMod = x % m_width ;

        double   fDelta = 255.0 * (m_opacity/100.0) / (m_width-1.0) ;
        PCL_A(&m_color) = FClamp0255(nMod * fDelta) ;
        FCColor::AlphaBlendPixel (pPixel, m_color) ;
    }
};
//-------------------------------------------------------------------------------------
/// Add raise frame (>=24 bit).
class FCEffectRaiseFrame : public FCImageEffect
{
    int   m_size ;
public:
    /**
        Constructor \n
        nSize >= 1
    */
    FCEffectRaiseFrame (int nSize)
    {
        m_size = ((nSize >= 1) ? nSize : 1) ;
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        RGBQUAD   cr ;
        if ((x < m_size) && (y < img.Height()-x) && (y >= x))
            cr = FCColor(255,255,255,65) ; // left
        else if ((y < m_size) && (x < img.Width()-y) && (x >= y))
            cr = FCColor(255,255,255,120) ; // top
        else if ((x > img.Width()-m_size) && (y >= img.Width()-x) && (y < img.Height()+x-img.Width()))
            cr = FCColor(0,0,0,65) ; // right
        else if (y > img.Height()-m_size)
            cr = FCColor(0,0,0,120) ; // bottom
        else
            return ;

        FCColor::AlphaBlendPixel (pPixel, cr) ;
    }
};
//-------------------------------------------------------------------------------------
/// Add pattern frame (>=24 bit).
class FCEffectPatternFrame : public FCImageEffect
{
    FCObjImage   m_top ;
    FCObjImage   m_left ;
public:
    /// Constructor
    FCEffectPatternFrame (const FCObjImage& image_top)
    {
        m_top = image_top ;
        m_top.ConvertTo32Bit() ;

        FCEffectRotate270   c ;
        m_left = m_top ;
        m_left.ApplyEffect(c) ;
    }

private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        // top border
        if (y < m_top.Height())
        {
            if ((x >= y) && (x <= img.Width()-1-y))
            {
                BYTE   * s = m_top.GetBits (x % m_top.Width(), y) ;
                FCColor::CopyPixelBPP (pPixel, s, img.ColorBits()) ;

                // copy to bottom mirror
                FCColor::CopyPixelBPP (img.GetBits(x,img.Height()-1-y), s, img.ColorBits()) ;
            }
        }

        // left border
        if (x < m_left.Width())
        {
            if ((y >= x) && (y <= img.Height()-1-x))
            {
                BYTE   * s = m_left.GetBits (x, y % m_left.Height()) ;
                FCColor::CopyPixelBPP (pPixel, s, img.ColorBits()) ;

                // copy to right mirror
                FCColor::CopyPixelBPP (img.GetBits(img.Width()-1-x,y), s, img.ColorBits()) ;
            }
        }
    }
};
//-----------------------------------------------------------------------------
// Calculate optimized image's rect.
class FCEffectGetOptimizedRect : public FCImageEffect
{
    int    m_first ;
    RECT   m_rect ;
public:
    FCEffectGetOptimizedRect()
    {
        RECT   z = {0,0,0,0} ;
        m_rect = z ;
        m_first = true ;
    }
    RECT GetOptimizedRect() const {return m_rect;}
private:
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() == 32) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (PCL_A(pPixel))
        {
            if (m_first)
            {
                m_rect.left = x ;
                m_rect.right = x+1 ;
                m_rect.top = y ;
                m_rect.bottom = y+1 ;
                m_first = false ;
            }
            else
            {
                if (x < m_rect.left)    m_rect.left = x ;
                if (x+1 > m_rect.right)   m_rect.right = x+1 ;
                if (y < m_rect.top)     m_rect.top = y ;
                if (y+1 > m_rect.bottom)  m_rect.bottom = y+1 ;
            }
        }
    }
};
//-------------------------------------------------------------------------------------
/// Bilinear distort (>=24 bit).
class FCEffectBilinearDistort : public FCImageEffect
{
protected:
    /// Test bpp >= 24 and width(height) >= 2
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() >= 24) && (img.Width() >= 2) && (img.Height() >= 2) ;
    }

    /// derived class must call OnBeforeProcess of base first.
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }

    const FCObjImage& GetBackupImage() const {return m_bak;}

    /**
        map current point (x,y) to point (un_x,un_y) that in original image \n
        derived class must set value of un_x and un_y
    */
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y) =0 ;

private:
    FCObjImage   m_bak ;

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   un_x, un_y ;
        calc_undistorted_coord (x, y, un_x, un_y) ;

        RGBQUAD   crNull = FCColor(0xFF,0xFF,0xFF,0) ;
        RGBQUAD   cr ;

        if ( (un_x > -1) && (un_x < m_bak.Width()) &&
             (un_y > -1) && (un_y < m_bak.Height()) )
        {
            // only this range is valid
            int   nSrcX = ((un_x < 0) ? -1 : (int)un_x),
                  nSrcY = ((un_y < 0) ? -1 : (int)un_y),
                  nSrcX_1 = nSrcX + 1,
                  nSrcY_1 = nSrcY + 1 ;

            BYTE   * pcrPixel[4] =
            {
                m_bak.IsInside(nSrcX,nSrcY) ? m_bak.GetBits(nSrcX,nSrcY) : (BYTE*)&crNull,
                m_bak.IsInside(nSrcX_1,nSrcY) ? m_bak.GetBits(nSrcX_1,nSrcY) : (BYTE*)&crNull,
                m_bak.IsInside(nSrcX,nSrcY_1) ? m_bak.GetBits(nSrcX,nSrcY_1) : (BYTE*)&crNull,
                m_bak.IsInside(nSrcX_1,nSrcY_1) ? m_bak.GetBits(nSrcX_1,nSrcY_1) : (BYTE*)&crNull,
            };
            cr = FCColor::GetBilinear (un_x-nSrcX, un_y-nSrcY, img.ColorBits(), pcrPixel) ;
        }
        else
        {
            cr = crNull ;
        }

        FCColor::CopyPixelBPP (pPixel, &cr, img.ColorBits()) ;
    }
};
//-------------------------------------------------------------------------------------
/// Wave (>=24 bit).
class FCEffectWave : public FCEffectBilinearDistort
{
    double   m_phase ;
    double   m_amplitude ;
    double   m_wave_length ;
public:
    /**
        Constructor \n
        nWavelength >= 1 \n
        nAmplitude >= 1
    */
    FCEffectWave (int nWavelength, int nAmplitude, double fPhase=0)
    {
        m_phase = fPhase ;
        m_wave_length = 2 * ((nWavelength >= 1) ? nWavelength : 1) ;
        m_amplitude = ((nAmplitude >= 1) ? nAmplitude : 1) ;
    }
private:
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        const FCObjImage   & bak = GetBackupImage() ;

        double   hw = bak.Width(),
                 hh = bak.Height() ;
        double   fScaleX = 1.0, fScaleY = 1.0 ;
        if (hw < hh)
            fScaleX = hh / hw ;
        else if (hw > hh)
            fScaleY = hw / hh ;

        // distances to center, scaled
        double   cen_x = bak.Width() / 2.0,
                 cen_y = bak.Height() / 2.0,
                 dx = (x - cen_x) * fScaleX,
                 dy = (y - cen_y) * fScaleY,
                 amnt = m_amplitude * sin (2 * LIB_PI * sqrt (dx*dx + dy*dy) / m_wave_length + m_phase) ;
        un_x = (amnt + dx) / fScaleX + cen_x ;
        un_y = (amnt + dy) / fScaleY + cen_y ;
        un_x = FClamp (un_x, 0.0, bak.Width()-1.0) ;
        un_y = FClamp (un_y, 0.0, bak.Height()-1.0) ;
    }
};
//-------------------------------------------------------------------------------------
/// Bulge and pinch (>=24 bit).
class FCEffectBulge : public FCEffectBilinearDistort
{
    double   m_amount ;
    double   m_offset_x ;
    double   m_offset_y ;
public:
    /**
        Constructor \n
        param -200 <= nAmount <= 100 \n
        param -1 <= offset_x (offset_y) <= 1
    */
    FCEffectBulge (int nAmount, double offset_x=0, double offset_y=0)
    {
        m_amount = nAmount / 100.0 ;
        m_offset_x = FClamp(offset_x, -1.0, 1.0) ;
        m_offset_y = FClamp(offset_y, -1.0, 1.0) ;
    }
private:
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        const FCObjImage   & bak = GetBackupImage() ;

        double   hw = bak.Width() / 2.0 ;
        double   hh = bak.Height() / 2.0 ;
        double   maxrad = (hw < hh ? hw : hh) ;
        hw += m_offset_x * hw ;
        hh += m_offset_y * hh ;

        double   u = x - hw ;
        double   v = y - hh ;
        double   r = sqrt(u*u + v*v) ;
        double   rscale1 = 1.0 - (r / maxrad) ;

        if (rscale1 > 0)
        {
            double   rscale2 = 1.0 - m_amount * rscale1 * rscale1 ;
            un_x = FClamp (u * rscale2 + hw, 0.0, bak.Width()-1.0) ;
            un_y = FClamp (v * rscale2 + hh, 0.0, bak.Height()-1.0) ;
        }
        else
        {
            un_x = x ;
            un_y = y ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Twist (>=24 bit).
class FCEffectTwist : public FCEffectBilinearDistort
{
    double   m_twist ;
    double   m_size ;
    double   m_offset_x ;
    double   m_offset_y ;
public:
    /**
        Constructor \n
        param -45 <= nAmount <= 45 \n
        param 1 <= nSize <= 200 \n
        param -2 <= offset_x (offset_y) <= 2
    */
    FCEffectTwist (int nAmount, int nSize, double offset_x=0, double offset_y=0)
    {
        nAmount = -nAmount ;
        m_twist = nAmount * nAmount * ((nAmount > 0) ? 1 : -1) ;

        m_size = 1.0 / (FClamp(nSize, 1, 200) / 100.0) ;
        m_offset_x = FClamp(offset_x, -2.0, 2.0) ;
        m_offset_y = FClamp(offset_y, -2.0, 2.0) ;
    }
private:
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        const FCObjImage   & bak = GetBackupImage() ;

        double   hw = bak.Width() / 2.0 ;
        double   hh = bak.Height() / 2.0 ;
        double   invmaxrad = 1.0 / (hw < hh ? hw : hh) ;
        hw += m_offset_x * hw ;
        hh += m_offset_y * hh ;

        double   u = x - hw ;
        double   v = y - hh ;
        double   r = sqrt(u*u + v*v) ;
        double   theta = atan2(v, u) ;

        double   t = 1 - ((r * m_size) * invmaxrad) ;
        t = (t < 0) ? 0 : (t * t * t) ;
        theta += (t * m_twist) / 100.0 ;

        un_x = FClamp (hw + r * cos(theta), 0.0, bak.Width()-1.0) ;
        un_y = FClamp (hh + r * sin(theta), 0.0, bak.Height()-1.0) ;
    }
};
//-------------------------------------------------------------------------------------
/// Shear transform (>=24 bit).
class FCEffectShear : public FCEffectBilinearDistort
{
    int   m_delta_x, m_delta_y ;
public:
    /// Constructor.
    FCEffectShear (int nShearX, int nShearY) : m_delta_x(nShearX), m_delta_y(nShearY) {}
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectBilinearDistort::OnBeforeProcess(img) ;

        // create sheared image
        img.Create (img.Width() + abs(m_delta_x), img.Height() + abs(m_delta_y), img.ColorBits()) ;
    }
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        if (m_delta_x)
        {
            double   h = GetBackupImage().Height() ;
            un_x = x - ((m_delta_x > 0) ? (h - y) : y) * abs(m_delta_x) / h ;
            un_y = y ;
        }
        else if (m_delta_y)
        {
            double   w = GetBackupImage().Width() ;
            un_x = x ;
            un_y = y - ((m_delta_y > 0) ? (w - x) : x) * abs(m_delta_y) / w ;
        }
        else
        {
            un_x = x ;
            un_y = y ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Perspective transform (>=24 bit).
class FCEffectPerspective : public FCEffectBilinearDistort
{
    POINT  m_ptNewPos[4] ; // LT, RT, RB, LB
    int    m_nNewWidth, m_nNewHeight ;
public:
    /**
        Constructor \n
        ptNewPos - new position array as : left-top, right-top, right-bottom, left-bottom.
    */
    FCEffectPerspective (const POINT ptNewPos[4])
    {
        memcpy (m_ptNewPos, ptNewPos, sizeof(POINT) * 4) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectBilinearDistort::OnBeforeProcess(img) ;

        // create new image
        int   w1 = abs(m_ptNewPos[0].x-m_ptNewPos[1].x) ;
        int   w2 = abs(m_ptNewPos[2].x-m_ptNewPos[3].x) ;
        m_nNewWidth = ((w1 >= w2) ? w1 : w2) ;
        int   h1 = abs(m_ptNewPos[0].y-m_ptNewPos[3].y) ;
        int   h2 = abs(m_ptNewPos[1].y-m_ptNewPos[2].y) ;
        m_nNewHeight = ((h1 >= h2) ? h1 : h2) ;
        img.Create (m_nNewWidth, m_nNewHeight, img.ColorBits()) ;
    }
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        const FCObjImage   & bak = GetBackupImage() ;

        if (m_ptNewPos[0].y != m_ptNewPos[1].y)
        {
            int     nDelta = abs(m_ptNewPos[0].y-m_ptNewPos[3].y) - abs(m_ptNewPos[1].y-m_ptNewPos[2].y) ;
            double  fOffset = fabs(nDelta * ((nDelta > 0) ? x : (m_nNewWidth-x)) / (2.0 * m_nNewWidth)) ;
            un_y = bak.Height() * (y - fOffset) / (m_nNewHeight - 2.0 * fOffset) ;
            un_x = bak.Width() * x / (double)m_nNewWidth ;
        }
        else if (m_ptNewPos[0].x != m_ptNewPos[3].x)
        {
            int     nDelta = abs(m_ptNewPos[0].x-m_ptNewPos[1].x) - abs(m_ptNewPos[2].x-m_ptNewPos[3].x) ;
            double  fOffset = fabs(nDelta * ((nDelta > 0) ? y : (m_nNewHeight-y)) / (2.0 * m_nNewHeight)) ;
            un_x = bak.Width() * (x - fOffset) / (m_nNewWidth - 2.0 * fOffset) ;
            un_y = bak.Height() * y / (double)m_nNewHeight ;
        }
        else
        {
            un_x = x ;
            un_y = y ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Rotate image clockwise (>=24 bit).
class FCEffectRotate : public FCEffectBilinearDistort
{
    double  m_fAngle ;
    int     m_nNewWidth, m_nNewHeight ;
public:
    /**
        Constructor \n
        0 <= nAngle < 360
    */
    FCEffectRotate (int nAngle)
    {
        while (nAngle < 0)
            nAngle += 360 ;
        nAngle %= 360 ;

        m_fAngle = LIB_PI * nAngle / 180.0 ;
    }
private:
    static void RotatePoint (double& x, double& y, double center_x, double center_y, double fAngle)
    {
        double   dx = x - center_x, dy = -y + center_y,
                 cost = cos(fAngle), sint = sin(fAngle) ;
        x = center_x + (dx*cost + dy*sint) ;
        y = center_y - (dy*cost - dx*sint) ;
    }
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectBilinearDistort::OnBeforeProcess (img) ;

        // calculate new width & height
        double   cen_x=img.Width()/2.0, cen_y=img.Height()/2.0,
                 x1=0, y1=0,
                 x2=img.Width(), y2=0,
                 x3=0, y3=img.Height(),
                 x4=img.Width(), y4=img.Height() ;

        RotatePoint (x1, y1, cen_x, cen_y, m_fAngle) ;
        RotatePoint (x2, y2, cen_x, cen_y, m_fAngle) ;
        RotatePoint (x3, y3, cen_x, cen_y, m_fAngle) ;
        RotatePoint (x4, y4, cen_x, cen_y, m_fAngle) ;

        double   xl[] = {x1, x2, x3, x4} ;
        double   yl[] = {y1, y2, y3, y4} ;

        m_nNewWidth = (int)ceil(*std::max_element(xl, xl+4) - *std::min_element(xl, xl+4)) ;
        m_nNewHeight = (int)ceil(*std::max_element(yl, yl+4) - *std::min_element(yl, yl+4)) ;
        img.Create (m_nNewWidth, m_nNewHeight, img.ColorBits()) ;
    }
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        double   cen_x = (m_nNewWidth-1)/2.0, cen_y = (m_nNewHeight-1)/2.0 ;

        un_x=x ; un_y=y ;
        RotatePoint (un_x, un_y, cen_x, cen_y, 2*LIB_PI - m_fAngle) ;

        un_x -= (m_nNewWidth - GetBackupImage().Width()) / 2.0 ;
        un_y -= (m_nNewHeight - GetBackupImage().Height()) / 2.0 ;
    }
};
//-------------------------------------------------------------------------------------
/// Ribbon (>=24 bit).
class FCEffectRibbon : public FCEffectBilinearDistort
{
    int      m_swing, m_frequency ;
    double   m_delta ;
    std::vector<double>   m_shift_down ;
public:
    /**
        Constructor \n
        0 <= nSwing <= 100, percentage \n
        0 <= nFrequency, a pi every 10
    */
    FCEffectRibbon (int nSwing, int nFrequency)
    {
        m_swing = FClamp (nSwing, 0, 100) ;
        m_frequency = ((nFrequency >= 0) ? nFrequency : 0) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectBilinearDistort::OnBeforeProcess (img) ;

        // clear image
        memset (img.GetMemStart(), 0, img.GetPitch()*img.Height()) ;

        m_delta = m_swing * img.Height() * 75.0/100/100 ; // upper, max 75%
        for (int i=0 ; i < (int)m_delta ; i++)
            memset (GetBackupImage().GetBits(i), 0, GetBackupImage().GetPitch()) ;

        double   fAngleSpan = m_frequency * LIB_PI / 10.0 / img.Width() ;
        for (int x=0 ; x < img.Width() ; x++)
        {
            double   d = (1-cos(x * fAngleSpan)) * m_delta / 2.0 ;
            m_shift_down.push_back (d) ;
        }
    }
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        un_x = x ;
        un_y = y + m_delta - m_shift_down[x] ;
    }
};
//-------------------------------------------------------------------------------------
/// Ripple (>=24 bit).
class FCEffectRipple : public FCEffectBilinearDistort
{
    int   m_wave_length ;
    int   m_amplitude ;
    int   m_sin_type ;
public:
    /**
        Constructor \n
        nWavelength >= 1 \n
        nAmplitude >= 1
    */
    FCEffectRipple (int nWavelength, int nAmplitude, bool bSinType=true)
    {
        m_wave_length = ((nWavelength >= 1) ? nWavelength : 1) ;
        m_amplitude = ((nAmplitude >= 1) ? nAmplitude : 1) ;
        m_sin_type = (bSinType ? 1 : 0) ;
    }
private:
    virtual void calc_undistorted_coord (int x, int y, double& un_x, double& un_y)
    {
        double   w = GetBackupImage().Width() ;
        un_x = fmod (x + w + shift_amount(y), w) ;
        un_x = FClamp (un_x, 0.0, w-1) ;
        un_y = y ;
    }
    double shift_amount (int nPos) const
    {
        if (m_sin_type)
            return m_amplitude * sin(nPos*2*LIB_PI/m_wave_length) ;
        else
            return floor (m_amplitude * (fabs ((((nPos % m_wave_length) / (double)m_wave_length) * 4) - 2) - 1)) ;
    }
};
//-------------------------------------------------------------------------------------
#include "video.h"
#include "color_balance.h"
#include "export_text.h"
#include "freeimage_quantize.h"
#include "supernova.h"
#include "oil_paint.h"
#include "convolute.h"
#include "color_level.h"
#include "reduce_noise.h"
#include "lens_flare.h"
#include "tile_reflection.h"
#include "blur_zoom.h"
#include "blur_radial.h"
#include "blur_motion.h"
#include "smooth_edge.h"
//-------------------------------------------------------------------------------------
/// Soft glow (>=24 bit).
class FCEffectSoftGlow : public FCEffectBlur_Gauss
{
    FCObjImage   m_bak ;
    std::auto_ptr<FCImageEffect>   m_bc ;
public:
    /**
        Constructor \n
        param nRadius >= 1 \n
        param -100 <= nBrightness <= 100 \n
        param -100 <= nContrast <= 100
    */
    FCEffectSoftGlow (int nRadius, int nBrightness, int nContrast) : FCEffectBlur_Gauss(nRadius, true)
    {
        m_bc.reset (new FCEffectBrightnessContrast(nBrightness, nContrast)) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
        FCEffectBlur_Gauss::OnBeforeProcess(img) ;
        m_bc->OnBeforeProcess(img) ;
    }
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        m_bc->ProcessPixel (img, x, y, pPixel) ;

        BYTE   * pOld = m_bak.GetBits(x,y) ;
        for (int i=0 ; i < 3 ; i++)
        {
            pPixel[i] = 255 - (255 - pOld[i])*(255 - pPixel[i])/255 ;
        }
        if (img.ColorBits() == 32)
        {
            PCL_A(pPixel) = PCL_A(pOld) ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// UNSharp mask (>=24 bit).
class FCEffectUNSharpMask : public FCEffectBlur_Gauss
{
    FCObjImage   m_bak ;
    int   m_amount ;
    int   m_threshold ;
public:
    /**
        Constructor \n
        nRadius >= 1 \n
        1 <= nAmount <= 100 \n
        0 <= nThreshold <= 255
    */
    FCEffectUNSharpMask (int nRadius, int nAmount, int nThreshold) : FCEffectBlur_Gauss(nRadius, true)
    {
        m_amount = FClamp(nAmount, 1, 100) ;
        m_threshold = FClamp0255(nThreshold) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
        FCEffectBlur_Gauss::OnBeforeProcess(img) ;
    }
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        BYTE   * pOld = m_bak.GetBits(x,y) ;
        for (int i=0 ; i < 3 ; i++)
        {
            int   d = pOld[i] - pPixel[i] ;
            if (abs(2*d) < m_threshold)
                d = 0 ;
            pPixel[i] = FClamp0255 (pOld[i] + m_amount * d / 100) ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Pencil sketch (>=24 bit).
class FCEffectPencilSketch : public FCEffectBlur_Gauss
{
    std::vector<FCImageEffect*>   m_cmd ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        nPenSize >= 1 \n
        -100 <= nRange <= 100
    */
    FCEffectPencilSketch (int nPenSize, int nRange) : FCEffectBlur_Gauss(nPenSize, true)
    {
        m_cmd.push_back(new FCEffectBrightnessContrast(nRange, -nRange)) ;
        m_cmd.push_back(new FCEffectInvert) ;
        m_cmd.push_back(new FCEffectGrayscale) ;
    }
    ~FCEffectPencilSketch()
    {
        std::for_each (m_cmd.begin(), m_cmd.end(), FCDeleteEachObject()) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
        FCEffectBlur_Gauss::OnBeforeProcess(img) ;

        for (size_t i=0 ; i < m_cmd.size() ; i++)
            m_cmd[i]->OnBeforeProcess(img) ;
    }
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        for (size_t i=0 ; i < m_cmd.size() ; i++)
            m_cmd[i]->ProcessPixel(img, x, y, pPixel) ;

        BYTE   * pOld = m_bak.GetBits(x,y) ;
        int    nGray = FCColor::GetGrayscale(pOld) ;
        PCL_R(pPixel) = PCL_G(pPixel) = PCL_B(pPixel) = FClamp0255 (nGray * 256 / (256 - pPixel[0])) ;
        if (img.ColorBits() == 32)
        {
            PCL_A(pPixel) = PCL_A(pOld) ;
        }
    }
};
//-------------------------------------------------------------------------------------
/// Old photo (>=24 bit)
class FCEffectOldPhoto : public FCEffectBlur_Gauss
{
    std::vector<FCImageEffect*>   m_cmd ;
public:
    /**
        Constructor\n
        nBlurSize >= 1
    */
    FCEffectOldPhoto (int nBlurSize, int nBrightness=-20, int nContrast=-40) : FCEffectBlur_Gauss(nBlurSize, true)
    {
        m_cmd.push_back(new FCEffectGrayscale) ;
        m_cmd.push_back(new FCEffectBrightnessContrast(nBrightness, nContrast)) ;
        m_cmd.push_back(new FCEffectColorBalance(true, FCEffectColorBalance::TONE_SHADOWS, 30, 0, -30)) ;
    }
    ~FCEffectOldPhoto()
    {
        std::for_each (m_cmd.begin(), m_cmd.end(), FCDeleteEachObject()) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectBlur_Gauss::OnBeforeProcess(img) ;
        for (size_t i=0 ; i < m_cmd.size() ; i++)
            m_cmd[i]->OnBeforeProcess(img) ;
    }
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        for (size_t i=0 ; i < m_cmd.size() ; i++)
            m_cmd[i]->ProcessPixel(img, x, y, pPixel) ;
    }
};
//-------------------------------------------------------------------------------------
/// Soft portrait (>=24 bit)
class FCEffectSoftPortrait : public FCEffectBlur_Gauss
{
    FCObjImage  m_bak ;
    int         m_warmth ;
    std::auto_ptr<FCImageEffect>   m_light ;
public:
    /**
        Constructor \n
        param nSoft >= 1 \n
        param -100 <= nLight <= 100 \n
        param 0 <= nWarmth <= 100
    */
    FCEffectSoftPortrait (int nSoft, int nLight, int nWarmth) : FCEffectBlur_Gauss(nSoft*3, true)
    {
        m_warmth = FClamp(nWarmth, 0, 100) ;
        m_light.reset(new FCEffectBrightnessContrast(nLight,0)) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
        FCEffectBlur_Gauss::OnBeforeProcess(img) ;
        m_light->OnBeforeProcess(img) ;
    }
    static BYTE overlay_blend (int s, int d)
    {
        int   t ;
        if (d < 128)
        {
            t = 2 * d * s + 0x80 ;
            t = ((t >> 8) + t) >> 8 ;
        }
        else
        {
            t = 2 * (255 - d) * (255 - s) + 0x80 ;
            t = ((t >> 8) + t) >> 8 ;
            t = 255 - t ;
        }
        return (BYTE)t ;
    }
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        m_light->ProcessPixel(img, x, y, pPixel) ;

        BYTE   * pOld = m_bak.GetBits(x,y) ;
        int    gr = FCColor::GetGrayscale(pOld) ;
        PCL_B(pPixel) = overlay_blend (PCL_B(pPixel), gr * (100 - m_warmth) / 100) ;
        PCL_G(pPixel) = overlay_blend (PCL_G(pPixel), gr) ;
        PCL_R(pPixel) = overlay_blend (PCL_R(pPixel), FClamp0255(gr * (100 + m_warmth) / 100)) ;

        if (img.ColorBits() == 32)
        {
            PCL_A(pPixel) = PCL_A(pOld) ;
        }
    }
};
