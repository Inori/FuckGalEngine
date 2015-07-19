
//-------------------------------------------------------------------------------------
/// Mosaic (>=24 bit).
class FCEffectMosaic : public FCImageEffect
{
    int   m_size ;
    FCObjImage   m_bak ;

public:
    /**
        Constructor \n
        nSize >= 2
    */
    FCEffectMosaic (int nSize)
    {
        m_size = ((nSize >= 2) ? nSize : 2) ;
    }

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if ((x % m_size == 0) && (y % m_size == 0))
        {
            RGBQUAD   cr = GetBlockAverage (x, y) ;
            FCColor::CopyPixelBPP(pPixel, &cr, img.ColorBits()) ;
        }
        else
        {
            FCColor::CopyPixelBPP(pPixel, img.GetBits(x/m_size*m_size, y/m_size*m_size), img.ColorBits()) ;
        }
    }

    RGBQUAD GetBlockAverage (int xStart, int yStart)
    {
        RECT   rc = {xStart, yStart, xStart+m_size, yStart+m_size} ;
        RECT   rcImg = {0, 0, m_bak.Width(), m_bak.Height()} ;
        IntersectRect (&rc, &rc, &rcImg) ;

        double  r=0, g=0, b=0, a=0 ;
        int     nNum=0 ;
        for (int y=rc.top ; y < rc.bottom ; y++)
        {
            for (int x=rc.left ; x < rc.right ; x++)
            {
                RGBQUAD   cr = FCColor(0,0,0,0xFF) ;
                FCColor::CopyPixelBPP (&cr, m_bak.GetBits(x,y), m_bak.ColorBits()) ;

                b += (PCL_B(&cr) * PCL_A(&cr)) ;
                g += (PCL_G(&cr) * PCL_A(&cr)) ;
                r += (PCL_R(&cr) * PCL_A(&cr)) ;
                a += PCL_A(&cr) ;
                nNum++ ;
            }
        }

        RGBQUAD   cRet = {0,0,0,0} ;

        if (nNum)
        {
            PCL_A(&cRet) = (BYTE)(a/nNum) ;
            if (PCL_A(&cRet))
            {
                cRet = FCColor((int)(r/a), (int)(g/a), (int)(b/a), PCL_A(&cRet)) ;
            }
        }
        return cRet ;
    }
};
