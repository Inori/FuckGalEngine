
//-------------------------------------------------------------------------------------
/// Blur zoom (>=24 bit).
class FCEffectBlur_Zoom : public FCImageEffect
{
    int      m_length ;
    double   m_offset_x ;
    double   m_offset_y ;
    int      m_fcx, m_fcy ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        nLength >= 1 \n
        param -2 <= offset_x (offset_y) <= 2
    */
    FCEffectBlur_Zoom (int nLength, double offset_x=0, double offset_y=0)
    {
        m_length = ((nLength >= 1) ? nLength : 1) ;
        m_offset_x = FClamp(offset_x, -2.0, 2.0) ;
        m_offset_y = FClamp(offset_y, -2.0, 2.0) ;
    }
private:
    enum
    {
        RADIUS_LENGTH = 64,
    };
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        m_fcx = (int)(img.Width() * m_offset_x * 32768.0) + (img.Width() * 32768) ;
        m_fcy = (int)(img.Height() * m_offset_y * 32768.0) + (img.Height() * 32768) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int    sr=0, sg=0, sb=0, sa=0, sc=0 ; // sum of

        BYTE   * pSrc = m_bak.GetBits(x,y) ;
        int    ta = ((img.ColorBits() == 32) ? PCL_A(pSrc) : 0xFF) ;
        sr += PCL_R(pSrc) * ta ;
        sg += PCL_G(pSrc) * ta ;
        sb += PCL_B(pSrc) * ta ;
        sa += ta ;
        sc++ ;

        int   fx = (x * 65536) - m_fcx ;
        int   fy = (y * 65536) - m_fcy ;
        for (int i=0 ; i < RADIUS_LENGTH ; i++)
        {
            fx = fx - (fx / 16) * m_length / 1024 ;
            fy = fy - (fy / 16) * m_length / 1024 ;

            int   u = (fx + m_fcx + 32768) / 65536 ;
            int   v = (fy + m_fcy + 32768) / 65536 ;

            if (m_bak.IsInside(u,v))
            {
                pSrc = m_bak.GetBits(u,v) ;
                ta = ((img.ColorBits() == 32) ? PCL_A(pSrc) : 0xFF) ;
                sr += PCL_R(pSrc) * ta ;
                sg += PCL_G(pSrc) * ta ;
                sb += PCL_B(pSrc) * ta ;
                sa += ta ;
                sc++ ;
            }
        }

        RGBQUAD   cr ;
        if (sa)
            cr = FCColor (FClamp0255(sr/sa), FClamp0255(sg/sa), FClamp0255(sb/sa), FClamp0255(sa/sc)) ;
        else
            cr = FCColor (0xFF,0xFF,0xFF,0) ;
        FCColor::CopyPixelBPP (pPixel, &cr, img.ColorBits()) ;
    }
};
