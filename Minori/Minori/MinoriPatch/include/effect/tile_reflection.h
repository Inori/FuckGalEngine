
//-------------------------------------------------------------------------------------
/// Tile reflection (>=24 bit).
class FCEffectTileReflection : public FCImageEffect
{
    struct POINT_FLOAT
    {
        double   x ;
        double   y ;
    };

    // angle ==> radian
    double AngleToRadian (int nAngle) {return LIB_PI * nAngle / 180.0;}

public:
	/**
		Constructor \n
		param -45 <= nAngle <= 45 \n
		param 2 <= nSquareSize <= 200 \n
		param -20 <= nCurvature <= 20
	*/
    FCEffectTileReflection (int nSquareSize, int nCurvature, int nAngle=45)
    {
        nAngle = FClamp (nAngle, -45, 45) ;
        m_sin = sin (AngleToRadian(nAngle)) ;
        m_cos = cos (AngleToRadian(nAngle)) ;

        nSquareSize = FClamp (nSquareSize, 2, 200) ;
        m_scale = LIB_PI / nSquareSize ;

        nCurvature = FClamp (nCurvature, -20, 20) ;
        if (nCurvature == 0)
            nCurvature = 1 ;
        m_curvature = nCurvature * nCurvature / 10.0 * (abs(nCurvature)/nCurvature) ;

        for (int i=0 ; i < aasamples ; i++)
        {
            double  x = (i * 4) / (double)aasamples,
                    y = i / (double)aasamples ;
            x = x - (int)x ;
            m_aapt[i].x = m_cos * x + m_sin * y ;
            m_aapt[i].y = m_cos * y - m_sin * x ;
        }
    }

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   hw = img.Width() / 2.0,
                 hh = img.Height() / 2.0,
                 i = x - hw,
                 j = y - hh ;

        int   b=0, g=0, r=0, a=0 ;
        for (int mm=0 ; mm < aasamples ; mm++)
        {
            double   u = i + m_aapt[mm].x ;
            double   v = j - m_aapt[mm].y ;

            double   s =  m_cos * u + m_sin * v ;
            double   t = -m_sin * u + m_cos * v ;

            s += m_curvature * tan(s * m_scale) ;
            t += m_curvature * tan(t * m_scale) ;
            u = m_cos * s - m_sin * t ;
            v = m_sin * s + m_cos * t ;
            
            int   xSample = (int)(hw + u) ;
            int   ySample = (int)(hh + v) ;

            xSample = FClamp (xSample, 0, m_bak.Width()-1) ;
            ySample = FClamp (ySample, 0, m_bak.Height()-1) ;

            BYTE   * p = m_bak.GetBits(xSample, ySample) ;
            b += PCL_B(p) ;
            g += PCL_G(p) ;
            r += PCL_R(p) ;
            a += PCL_A(p) ;
        }

        PCL_B(pPixel) = FClamp0255 (b / aasamples) ;
        PCL_G(pPixel) = FClamp0255 (g / aasamples) ;
        PCL_R(pPixel) = FClamp0255 (r / aasamples) ;
        PCL_A(pPixel) = FClamp0255 (a / aasamples) ;
    }

    enum
    {
        aasamples = 17,
    };

    double   m_sin, m_cos ;
    double   m_scale ;
    double   m_curvature ;
    POINT_FLOAT   m_aapt[aasamples] ;
    FCObjImage    m_bak ;
};
