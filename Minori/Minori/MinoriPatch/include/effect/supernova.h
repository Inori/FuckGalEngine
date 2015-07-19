
//-------------------------------------------------------------------------------------
/// Supernova (>=24 bit).
class FCEffectSupernova : public FCImageEffect
{
    POINT    m_pt ;
    double   m_radius ;
    int      m_count ;

    std::vector<double>   m_spoke ;
    std::vector<RGBQUAD>  m_spokecolor ;

public:
    /**
        Constructor \n
        pt - coordinate on image \n
        cr - color of ray \n
        nRadius >= 1 \n
        nRayCount >= 1
    */
    FCEffectSupernova (POINT pt, RGBQUAD cr, int nRadius, int nRayCount) : m_spoke(BoundParam1(nRayCount)), m_spokecolor(BoundParam1(nRayCount))
    {
        m_pt = pt ;
        m_radius = BoundParam1(nRadius) ;
        m_count = BoundParam1(nRayCount) ;

        for (int i=0 ; i < m_count ; i++)
        {
            m_spoke[i] = get_gauss() ;
            m_spokecolor[i] = cr ;
        }
    }

private:
    static double get_gauss()
    {
        double   s = 0 ;
        for (int i=0 ; i < 6 ; i++)
            s = s + rand() / (double)(RAND_MAX + 1.0) ;
        return s / 6.0 ;
    }

    static int BoundParam1 (int n)
    {
        return ((n >= 1) ? n : 1) ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   u = (x - m_pt.x + 0.001) / m_radius ;
        double   v = (y - m_pt.y + 0.001) / m_radius ;

        double   t = (atan2 (u, v) / (2 * LIB_PI) + 0.51) * m_count ;
        int      i = (int)floor(t) ;
        t -= i ;
        i %= m_count ;

        double   w1 = m_spoke[i] * (1-t) + m_spoke[(i+1) % m_count] * t ;
        w1 = w1 * w1 ;

        double   w = 1.0 / sqrt (u*u + v*v) * 0.9 ;
        double   fRatio = FClamp (w, 0.0, 1.0) ;

        double   ws = FClamp (w1 * w, 0.0, 1.0) ;

        for (int m=0 ; m < 3 ; m++)
        {
            double   spokecol = ((BYTE*)&m_spokecolor[i])[m]/255.0 * (1-t) + ((BYTE*)&m_spokecolor[(i+1) % m_count])[m]/255.0 * t ;

            double   r ;
            if (w > 1.0)
                r = FClamp (spokecol * w, 0.0, 1.0);
            else
                r = pPixel[m]/255.0 * (1.0 - fRatio) + spokecol * fRatio ;

            r += ws ;
            pPixel[m] = FClamp0255 (r*255) ;
        }
    }
};
