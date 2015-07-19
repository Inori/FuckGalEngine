
//-------------------------------------------------------------------------------------
/// Lens flare (>=24 bit).
class FCEffectLensFlare : public FCImageEffect
{
public:
    /**
        Constructor \n
        pt - coordinate on image
    */
    FCEffectLensFlare (POINT pt) : m_pt(pt) {}
private:
    struct FLOATRGB
    {
        double  r ;
        double  g ;
        double  b ;
    };
    struct Reflect
    {
        FLOATRGB ccol ;
        double   size ;
        int      xp ;
        int      yp ;
        int      type ;
    };

    void initref (int sx, int sy, int width, int height, int matt)
    {
        int   xh = width / 2,
              yh = height / 2,
              dx = xh - sx,
              dy = yh - sy ;
        m_numref = 19 ;
        m_ref[0].type=1; m_ref[0].size=matt*0.027;
        m_ref[0].xp=(int)(0.6699*dx+xh); m_ref[0].yp=(int)(0.6699*dy+yh);
        m_ref[0].ccol.r=0.0; m_ref[0].ccol.g=14.0/255.0; m_ref[0].ccol.b=113.0/255.0;
        m_ref[1].type=1; m_ref[1].size=matt*0.01;
        m_ref[1].xp=(int)(0.2692*dx+xh); m_ref[1].yp=(int)(0.2692*dy+yh);
        m_ref[1].ccol.r=90.0/255.0; m_ref[1].ccol.g=181.0/255.0; m_ref[1].ccol.b=142.0/255.0;
        m_ref[2].type=1; m_ref[2].size=matt*0.005;
        m_ref[2].xp=(int)(-0.0112*dx+xh); m_ref[2].yp=(int)(-0.0112*dy+yh);
        m_ref[2].ccol.r=56.0/255.0; m_ref[2].ccol.g=140.0/255.0; m_ref[2].ccol.b=106.0/255.0;
        m_ref[3].type=2; m_ref[3].size=matt*0.031;
        m_ref[3].xp=(int)(0.6490*dx+xh); m_ref[3].yp=(int)(0.6490*dy+yh);
        m_ref[3].ccol.r=9.0/255.0; m_ref[3].ccol.g=29.0/255.0; m_ref[3].ccol.b=19.0/255.0;
        m_ref[4].type=2; m_ref[4].size=matt*0.015;
        m_ref[4].xp=(int)(0.4696*dx+xh); m_ref[4].yp=(int)(0.4696*dy+yh);
        m_ref[4].ccol.r=24.0/255.0; m_ref[4].ccol.g=14.0/255.0; m_ref[4].ccol.b=0.0;
        m_ref[5].type=2; m_ref[5].size=matt*0.037;
        m_ref[5].xp=(int)(0.4087*dx+xh); m_ref[5].yp=(int)(0.4087*dy+yh);
        m_ref[5].ccol.r=24.0/255.0; m_ref[5].ccol.g=14.0/255.0; m_ref[5].ccol.b=0.0;
        m_ref[6].type=2; m_ref[6].size=matt*0.022;
        m_ref[6].xp=(int)(-0.2003*dx+xh); m_ref[6].yp=(int)(-0.2003*dy+yh);
        m_ref[6].ccol.r=42.0/255.0; m_ref[6].ccol.g=19.0/255.0; m_ref[6].ccol.b=0.0;
        m_ref[7].type=2; m_ref[7].size=matt*0.025;
        m_ref[7].xp=(int)(-0.4103*dx+xh); m_ref[7].yp=(int)(-0.4103*dy+yh);
        m_ref[7].ccol.b=17.0/255.0; m_ref[7].ccol.g=9.0/255.0; m_ref[7].ccol.r=0.0;
        m_ref[8].type=2; m_ref[8].size=matt*0.058;
        m_ref[8].xp=(int)(-0.4503*dx+xh); m_ref[8].yp=(int)(-0.4503*dy+yh);
        m_ref[8].ccol.b=10.0/255.0; m_ref[8].ccol.g=4.0/255.0; m_ref[8].ccol.r=0.0;
        m_ref[9].type=2; m_ref[9].size=matt*0.017;
        m_ref[9].xp=(int)(-0.5112*dx+xh); m_ref[9].yp=(int)(-0.5112*dy+yh);
        m_ref[9].ccol.r=5.0/255.0; m_ref[9].ccol.g=5.0/255.0; m_ref[9].ccol.b=14.0/255.0;
        m_ref[10].type=2; m_ref[10].size=matt*0.2;
        m_ref[10].xp=(int)(-1.496*dx+xh); m_ref[10].yp=(int)(-1.496*dy+yh);
        m_ref[10].ccol.r=9.0/255.0; m_ref[10].ccol.g=4.0/255.0; m_ref[10].ccol.b=0.0;
        m_ref[11].type=2; m_ref[11].size=matt*0.5;
        m_ref[11].xp=(int)(-1.496*dx+xh); m_ref[11].yp=(int)(-1.496*dy+yh);
        m_ref[11].ccol.r=9.0/255.0; m_ref[11].ccol.g=4.0/255.0; m_ref[11].ccol.b=0.0;
        m_ref[12].type=3; m_ref[12].size=matt*0.075;
        m_ref[12].xp=(int)(0.4487*dx+xh); m_ref[12].yp=(int)(0.4487*dy+yh);
        m_ref[12].ccol.r=34.0/255.0; m_ref[12].ccol.g=19.0/255.0; m_ref[12].ccol.b=0.0;
        m_ref[13].type=3; m_ref[13].size=matt*0.1;
        m_ref[13].xp=(int)(dx+xh); m_ref[13].yp=(int)(dy+yh);
        m_ref[13].ccol.r=14.0/255.0; m_ref[13].ccol.g=26.0/255.0; m_ref[13].ccol.b=0.0;
        m_ref[14].type=3; m_ref[14].size=matt*0.039;
        m_ref[14].xp=(int)(-1.301*dx+xh); m_ref[14].yp=(int)(-1.301*dy+yh);
        m_ref[14].ccol.r=10.0/255.0; m_ref[14].ccol.g=25.0/255.0; m_ref[14].ccol.b=13.0/255.0;
        m_ref[15].type=4; m_ref[15].size=matt*0.19;
        m_ref[15].xp=(int)(1.309*dx+xh); m_ref[15].yp=(int)(1.309*dy+yh);
        m_ref[15].ccol.r=9.0/255.0; m_ref[15].ccol.g=0.0; m_ref[15].ccol.b=17.0/255.0;
        m_ref[16].type=4; m_ref[16].size=matt*0.195;
        m_ref[16].xp=(int)(1.309*dx+xh); m_ref[16].yp=(int)(1.309*dy+yh);
        m_ref[16].ccol.r=9.0/255.0; m_ref[16].ccol.g=16.0/255.0; m_ref[16].ccol.b=5.0/255.0;
        m_ref[17].type=4; m_ref[17].size=matt*0.20;
        m_ref[17].xp=(int)(1.309*dx+xh); m_ref[17].yp=(int)(1.309*dy+yh);
        m_ref[17].ccol.r=17.0/255.0; m_ref[17].ccol.g=4.0/255.0; m_ref[17].ccol.b=0.0;
        m_ref[18].type=4; m_ref[18].size=matt*0.038;
        m_ref[18].xp=(int)(-1.301*dx+xh); m_ref[18].yp=(int)(-1.301*dy+yh);
        m_ref[18].ccol.r=17.0/255.0; m_ref[18].ccol.g=4.0/255.0; m_ref[18].ccol.b=0.0;
    }

    static void fixpix (BYTE* p, double procent, const FLOATRGB& colpro)
    {
        PCL_R(p) = (BYTE)(PCL_R(p) + (255 - PCL_R(p)) * procent * colpro.r) ;
        PCL_G(p) = (BYTE)(PCL_G(p) + (255 - PCL_G(p)) * procent * colpro.g) ;
        PCL_B(p) = (BYTE)(PCL_B(p) + (255 - PCL_B(p)) * procent * colpro.b) ;
    }
    void mcolor (BYTE* s, double h)
    {
        double   procent = 1 - h/m_scolor ;
        if (procent > 0)
        {
            procent *= procent ;
            fixpix (s, procent, m_color) ;
        }
    }
    void mglow (BYTE* s, double h)
    {
        double   procent = 1 - h/m_sglow ;
        if (procent > 0)
        {
            procent *= procent ;
            fixpix (s, procent, m_glow) ;
        }
    }
    void minner (BYTE* s, double h)
    {
        double   procent = 1 - h/m_sinner ;
        if (procent > 0)
        {
            procent *= procent ;
            fixpix (s, procent, m_inner) ;
        }
    }
    void mouter (BYTE* s, double h)
    {
        double   procent = 1 - h/m_souter ;
        if (procent > 0)
            fixpix (s, procent, m_outer) ;
    }
    void mhalo (BYTE* s, double h)
    {
        double   procent = fabs((h - m_shalo) / (m_shalo * 0.07)) ;
        if (procent < 1)
            fixpix (s, 1 - procent, m_halo) ;
    }
    static void mrt1 (BYTE* s, Reflect* r, int x, int y)
    {
        double   procent = 1 - FHypot (r->xp - x, r->yp - y) / r->size ;
        if (procent > 0)
        {
            procent *= procent ;
            fixpix (s, procent, r->ccol) ;
        }
    }
    static void mrt2 (BYTE* s, Reflect* r, int x, int y)
    {
        double   procent = r->size - FHypot (r->xp - x, r->yp - y) ;
        procent /= (r->size * 0.15) ;
        if (procent > 0)
        {
            if (procent > 1)
                procent = 1 ;
            fixpix (s, procent, r->ccol) ;
        }
    }
    static void mrt3 (BYTE* s, Reflect* r, int x, int y)
    {
        double   procent = r->size - FHypot (r->xp - x, r->yp - y) ;
        procent /= (r->size * 0.12) ;
        if (procent > 0)
        {
            if (procent > 1)
                procent = 1 - (procent * 0.12) ;
            fixpix (s, procent, r->ccol) ;
        }
    }
    static void mrt4 (BYTE* s, Reflect* r, int x, int y)
    {
        double   procent  = FHypot (r->xp - x, r->yp - y) - r->size ;
        procent /= (r->size * 0.04) ;
        procent  = fabs (procent) ;
        if (procent < 1)
            fixpix (s, 1 - procent, r->ccol) ;
    }

    virtual void OnBeforeProcess (FCObjImage& img)
    {
        int   matt = img.Width() ;
        m_scolor = matt * 0.0375;
        m_sglow  = matt * 0.078125;
        m_sinner = matt * 0.1796875;
        m_souter = matt * 0.3359375;
        m_shalo  = matt * 0.084375;

        m_color.r = 239.0/255.0; m_color.g = 239.0/255.0; m_color.b = 239.0/255.0;
        m_glow.r  = 245.0/255.0; m_glow.g  = 245.0/255.0; m_glow.b  = 245.0/255.0;
        m_inner.r = 255.0/255.0; m_inner.g = 38.0/255.0;  m_inner.b = 43.0/255.0;
        m_outer.r = 69.0/255.0;  m_outer.g = 59.0/255.0;  m_outer.b = 64.0/255.0;
        m_halo.r  = 80.0/255.0;  m_halo.g  = 15.0/255.0;  m_halo.b  = 4.0/255.0;

        initref (m_pt.x, m_pt.y, img.Width(), img.Height(), matt) ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   hyp = FHypot (x-m_pt.x, y-m_pt.y) ;

        mcolor (pPixel, hyp); /* make color */
        mglow (pPixel, hyp);  /* make glow  */
        minner (pPixel, hyp); /* make inner */
        mouter (pPixel, hyp); /* make outer */
        mhalo (pPixel, hyp);  /* make halo  */

        for (int i=0 ; i < m_numref ; i++)
        {
            switch (m_ref[i].type)
            {
                case 1 : mrt1 (pPixel, m_ref + i, x, y); break;
                case 2 : mrt2 (pPixel, m_ref + i, x, y); break;
                case 3 : mrt3 (pPixel, m_ref + i, x, y); break;
                case 4 : mrt4 (pPixel, m_ref + i, x, y); break;
            }
        }
    }

    static double FHypot (double x, double y)
    {
        return sqrt (x*x + y*y) ;
    }

    POINT    m_pt ;
    Reflect  m_ref[19] ;
    int      m_numref ;
    double   m_scolor, m_sglow, m_sinner, m_souter, m_shalo ;
    FLOATRGB m_color, m_glow, m_inner, m_outer, m_halo ;
};
