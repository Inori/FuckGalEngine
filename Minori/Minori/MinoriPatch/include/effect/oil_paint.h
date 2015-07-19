
//-------------------------------------------------------------------------------------
/// Oil paint (>=24 bit).
class FCEffectOilPaint : public FCImageEffect
{
    int         m_radius ;
    int         m_coarseness ;
    FCObjImage  m_gray ;
    FCObjImage  m_bak ;
public:
    /**
        Constructor \n
        nRadius >= 1 \n
        3 <= nCoarseness <= 255
    */
    FCEffectOilPaint (int nRadius, int nCoarseness)
    {
        m_radius = ((nRadius >= 1) ? nRadius : 1) ;
        m_coarseness = FClamp(nCoarseness, 3, 255) ;
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        // calculate gray
        m_gray.Create (img.Width(), img.Height(), 8) ;
        for (int y=0 ; y < img.Height() ; y++)
        {
            for (int x=0 ; x < img.Width() ; x++)
            {
                int   n = FCColor::GetGrayscale (img.GetBits(x,y)) ;
                *m_gray.GetBits(x,y) = (BYTE)(m_coarseness * n / 0xFF) ;
            }
        }
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   r[256]={0}, g[256]={0}, b[256]={0}, c[256]={0} ;

        // replace every pixel use most frequency
        for (int ny=y-m_radius ; ny <= y+m_radius ; ny++)
        {
            int   sy = FClamp(ny, 0, m_gray.Height()-1) ;
            for (int nx=x-m_radius ; nx <= x+m_radius ; nx++)
            {
                int    sx = FClamp(nx, 0, m_gray.Width()-1) ;
                BYTE   * pGray = m_gray.GetBits (sx, sy) ;
                BYTE   * pBak = m_bak.GetBits (sx, sy) ;
                c[*pGray]++ ;
                r[*pGray] += PCL_R(pBak) ;
                g[*pGray] += PCL_G(pBak) ;
                b[*pGray] += PCL_B(pBak) ;
            }
        }

        int   nMax=0, nIndex=0 ;
        for (int i=0 ; i <= m_coarseness ; i++)
        {
            if (c[i] > nMax)
            {
                nIndex = i ;
                nMax = c[i] ;
            }
        }

        // not change alpha
        if (nMax)
        {
            PCL_R(pPixel) = (BYTE)(r[nIndex] / nMax) ;
            PCL_G(pPixel) = (BYTE)(g[nIndex] / nMax) ;
            PCL_B(pPixel) = (BYTE)(b[nIndex] / nMax) ;
        }
    }
};
