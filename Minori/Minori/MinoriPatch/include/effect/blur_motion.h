
//-------------------------------------------------------------------------------------
/// Blur motion (>=24 bit).
class FCEffectBlur_Motion : public FCImageEffect
{
    int   m_dx, m_dy ;
    int   m_s1, m_s2 ;
    int   m_err ;
    int   m_swapdir ;

    int         m_length ;
    FCObjImage  m_bak ;

public:
    /**
        Constructor \n
        nLength >= 1 \n
        0 <= nAngle <= 360
    */
    FCEffectBlur_Motion (int nLength, int nAngle)
    {
        m_length = ((nLength >= 1) ? nLength : 1) ;

        m_dx = (int)(nLength * cos (nAngle * LIB_PI / 180.0)) ;
        m_dy = (int)(nLength * sin (nAngle * LIB_PI / 180.0)) ;

        if (m_dx)
        {
            m_s1 = abs(m_dx) / m_dx ;
            m_dx = abs(m_dx) ;
        }
        else
            m_s1 = 0 ;

        if (m_dy)
        {
            m_s2 = abs(m_dy) / m_dy ;
            m_dy = abs(m_dy) ;
        }
        else
            m_s2 = 0 ;

        if (m_dy > m_dx)
        {
            std::swap (m_dx,m_dy) ;
            m_swapdir = 1 ;
        }
        else
            m_swapdir = 0 ;

        m_dy *= 2 ;
        m_err = m_dy - m_dx ;
        m_dx *= 2 ;
    }

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   sr=0, sg=0, sb=0, sa=0, sc=0 ;

        int   e=m_err ;

        for (int i=0 ; i < m_length ; i++)
        {
            BYTE   * pSrc = m_bak.GetBits(x,y) ;
            int    ta = ((img.ColorBits() == 32) ? PCL_A(pSrc) : 0xFF) ;
            sr += PCL_R(pSrc) * ta ;
            sg += PCL_G(pSrc) * ta ;
            sb += PCL_B(pSrc) * ta ;
            sa += ta ;
            sc++ ;

            while ((e >= 0) && m_dx)
            {
                if (m_swapdir)
                    x += m_s1 ;
                else
                    y += m_s2 ;
                e -= m_dx ;
            }

            if (m_swapdir)
                y += m_s2 ;
            else
                x += m_s1 ;

            e += m_dy ;

            if (!m_bak.IsInside(x,y))
                break ;
        }

        RGBQUAD   cr ;
        if (sa)
            cr = FCColor (FClamp0255(sr/sa), FClamp0255(sg/sa), FClamp0255(sb/sa), FClamp0255(sa/sc)) ;
        else
            cr = FCColor (0xFF,0xFF,0xFF,0) ;
        FCColor::CopyPixelBPP (pPixel, &cr, img.ColorBits()) ;
    }
};
