
//-------------------------------------------------------------------------------------
/// Blur radial (>=24 bit).
class FCEffectBlur_Radial : public FCImageEffect
{
    int      m_angle ;
    double   m_offset_x ;
    double   m_offset_y ;
    int      m_fcx, m_fcy, m_fr ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        param 0 <= nAngle <= 360 \n
        param -2 <= offset_x (offset_y) <= 2
    */
    FCEffectBlur_Radial (int nAngle, double offset_x=0, double offset_y=0)
    {
        m_angle = FClamp(nAngle, 0, 360) ;
        m_offset_x = FClamp(offset_x, -2.0, 2.0) ;
        m_offset_y = FClamp(offset_y, -2.0, 2.0) ;
    }
private:
    static void rotate_point(int& fx, int& fy, int fr)
    {
        int   cx=fx, cy=fy ;
        fx = cx - ((cy >> 8) * fr >> 8) - ((cx >> 14) * (fr * fr >> 11) >> 8) ;
        fy = cy + ((cx >> 8) * fr >> 8) - ((cy >> 14) * (fr * fr >> 11) >> 8) ;
    }

    enum
    {
        RADIUS_LENGTH = 64,
    };
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        m_fcx = (img.Width() * 32768) + (int)(m_offset_x * img.Width() * 32768) ;
        m_fcy = (img.Height() * 32768) + (int)(m_offset_y * img.Height() * 32768) ;
        m_fr = (int)(m_angle * LIB_PI * 65536.0 / 181.0) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int    sr=0, sg=0, sb=0, sa=0, sc=0 ;

        BYTE   * pSrc = m_bak.GetBits(x,y) ;
        int    ta = ((img.ColorBits() == 32) ? PCL_A(pSrc) : 0xFF) ;
        sr += PCL_R(pSrc) * ta ;
        sg += PCL_G(pSrc) * ta ;
        sb += PCL_B(pSrc) * ta ;
        sa += ta ;
        sc++ ;

        int   fx = (x * 65536) - m_fcx ;
        int   fy = (y * 65536) - m_fcy ;
        int   fsr = m_fr / RADIUS_LENGTH ;
        int   ox1 = fx ;
        int   ox2 = fx ;
        int   oy1 = fy ;
        int   oy2 = fy ;

        for (int i=0 ; i < RADIUS_LENGTH ; i++)
        {
            rotate_point(ox1, oy1, fsr) ;
            rotate_point(ox2, oy2, -fsr) ;

            int   u = (ox1 + m_fcx + 32768) / 65536 ;
            int   v = (oy1 + m_fcy + 32768) / 65536 ;
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

            u = (ox2 + m_fcx + 32768) / 65536 ;
            v = (oy2 + m_fcy + 32768) / 65536 ;
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
