
//-------------------------------------------------------------------------------------
/// Reduce noise (>=24 bit).
class FCEffectReduceNoise : public FCImageEffect
{
    int     m_nRadius ;
    double  m_dStrength ;
    int     m_nArea ;
    int     m_fr[256], m_fg[256], m_fb[256] ;
    int     m_cr[256], m_cg[256], m_cb[256] ;
    std::vector<POINT>   m_XAdd, m_XSub, m_YAdd, m_YSub ;
    FCObjImage   m_bak ;
public:
    /**
        Constructor \n
        nRadius >= 2 \n
        0 <= dStrength <= 1
    */
    FCEffectReduceNoise (int nRadius=10, double dStrength=0.4)
    {
        m_nRadius = ((nRadius >= 2) ? nRadius : 2) ;
        m_dStrength = FClamp(dStrength, 0.0, 1.0) ;
        m_nArea = 0 ;
        for (int i=0 ; i < 256 ; i++)
        {
            m_fr[i]=m_fg[i]=m_fb[i]=0 ;
        }
    }
private:
    void ChangeSum (int* hr, int* hg, int* hb, int x, int y, bool bAdd)
    {
        if (!m_bak.IsInside(x,y))
        {
            x = FClamp(x, 0, m_bak.Width()-1) ;
            y = FClamp(y, 0, m_bak.Height()-1) ;
        }

        BYTE   * p = m_bak.GetBits(x,y) ;
        if (bAdd)
        {
            hr[PCL_R(p)]++ ;
            hg[PCL_G(p)]++ ;
            hb[PCL_B(p)]++ ;
        }
        else
        {
            hr[PCL_R(p)]-- ;
            hg[PCL_G(p)]-- ;
            hb[PCL_B(p)]-- ;
        }
    }
private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        int   nR2 = (int)((m_nRadius + 0.5) * (m_nRadius + 0.5)) ;

        // calculate cut off image
        FCObjImage   imgCut ;
        imgCut.Create (2*m_nRadius+1, 2*m_nRadius+1, 8) ;
        for (int y=0 ; y < imgCut.Height() ; y++)
            for (int x=0 ; x < imgCut.Width() ; x++)
            {
                int   u = x - m_nRadius ;
                int   v = y - m_nRadius ;
                if (u*u + v*v <= nR2)
                {
                    *imgCut.GetBits(x,y) = 1 ;

                    // calculate first sum
                    ChangeSum (m_fr, m_fg, m_fb, u, v, true) ;
                    m_nArea++ ;
                }
            }

        // avoid divide zero
        m_nArea = ((m_nArea >= 1) ? m_nArea : 1) ;

        // pick up delta pixel
        for (int y=0 ; y < imgCut.Height() ; y++)
            for (int x=0 ; x < imgCut.Width() ; x++)
            {
                BYTE    * pUP = imgCut.GetBits(x,y) ;
                POINT   pt = {x-m_nRadius, y-m_nRadius} ; // (x,y) to (m_nRadius,m_nRadius)

                if ((x == 0) && *pUP)
                {
                    m_XSub.push_back(pt) ;
                }

                if ((x == imgCut.Width()-1) && *pUP)
                {
                    m_XAdd.push_back(pt) ;
                }

                // x walk
                if (imgCut.IsInside(x+1,y))
                {
                    BYTE   * pDown = imgCut.GetBits(x+1,y) ;
                    if (*pDown && !*pUP)
                    {
                        m_XSub.push_back(pt) ;
                    }
                    if (*pUP && !*pDown)
                    {
                        m_XAdd.push_back(pt) ;
                    }
                }

                if ((y == 0) && *pUP)
                {
                    m_YSub.push_back(pt) ;
                }

                if ((y == imgCut.Height()-1) && *pUP)
                {
                    m_YAdd.push_back(pt) ;
                }

                // y walk
                if (imgCut.IsInside(x,y+1))
                {
                    BYTE   * pDown = imgCut.GetBits(x,y+1) ;
                    if (*pDown && !*pUP)
                    {
                        m_YSub.push_back(pt) ;
                    }
                    if (*pUP && !*pDown)
                    {
                        m_YAdd.push_back(pt) ;
                    }
                }
            }
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (x == 0)
        {
            if (y == 0)
            {
            }
            else
            {
                for (size_t i=0 ; i < m_YAdd.size() ; i++)
                    ChangeSum(m_fr, m_fg, m_fb, x+m_YAdd[i].x, y+m_YAdd[i].y, true) ;

                for (size_t i=0 ; i < m_YSub.size() ; i++)
                    ChangeSum(m_fr, m_fg, m_fb, x+m_YSub[i].x, y+m_YSub[i].y, false) ;
            }

            for (int i=0 ; i < 256 ; i++)
            {
                m_cr[i]=m_fr[i]; m_cg[i]=m_fg[i]; m_cb[i]=m_fb[i];
            }
        }
        else
        {
            for (size_t i=0 ; i < m_XAdd.size() ; i++)
                ChangeSum(m_cr, m_cg, m_cb, x+m_XAdd[i].x, y+m_XAdd[i].y, true) ;

            for (size_t i=0 ; i < m_XSub.size() ; i++)
                ChangeSum(m_cr, m_cg, m_cb, x+m_XSub[i].x, y+m_XSub[i].y, false) ;
        }

        int   rc=0, gc=0, bc=0 ;
        for (int i=0 ; i < PCL_R(pPixel) ; i++)
        {
            rc += m_cr[i] ;
        }
        for (int i=0 ; i < PCL_G(pPixel) ; i++)
        {
            gc += m_cg[i] ;
        }
        for (int i=0 ; i < PCL_B(pPixel) ; i++)
        {
            bc += m_cb[i] ;
        }

        rc = rc * 255 / m_nArea ;
        gc = gc * 255 / m_nArea ;
        bc = bc * 255 / m_nArea ;

        double   dd = ((0.114 * PCL_B(pPixel)) + (0.587 * PCL_G(pPixel)) + (0.299 * PCL_R(pPixel))) / 255.0 ;
        double   lerp = -0.2 * m_dStrength * (1 - 0.75 * dd) ;
        PCL_R(pPixel) = FClamp0255((int)(PCL_R(pPixel) + lerp*(rc-PCL_R(pPixel)))) ;
        PCL_G(pPixel) = FClamp0255((int)(PCL_G(pPixel) + lerp*(gc-PCL_G(pPixel)))) ;
        PCL_B(pPixel) = FClamp0255((int)(PCL_B(pPixel) + lerp*(bc-PCL_B(pPixel)))) ;
    }
};
