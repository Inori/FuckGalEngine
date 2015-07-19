
//-------------------------------------------------------------------------------------
/// Base class of gradient fill (>=24 bit)
class FCEffectGradient : public FCImageEffect
{
public:
    /// repeat type
    enum REPEAT_MODE
    {
        REPEAT_NONE, /*!< repeat none */
        REPEAT_SAWTOOTH, /*!< repeat sawtooth */
        REPEAT_TRIANGULAR, /*!< repeat triangular */
    };

    /// Constructor.
    FCEffectGradient (RGBQUAD crStart, RGBQUAD crEnd, REPEAT_MODE nRepeat) : m_crStart(crStart), m_crEnd(crEnd), m_nRepeat(nRepeat) {}

protected:
    /// calculate factor of point(x,y)
    virtual double CalculateFactor (int x, int y) =0 ;

private:
    RGBQUAD      m_crStart, m_crEnd ;
    REPEAT_MODE  m_nRepeat ;

private:
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() >= 24) ;
    }
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   d = CalculateFactor (x, y) ;
        switch (m_nRepeat)
        {
            case REPEAT_NONE :
                d = FClamp (d, 0.0, 1.0) ;
                break;
            case REPEAT_SAWTOOTH :
                d = d - floor(d) ;
                break;
            case REPEAT_TRIANGULAR :
                if ( ((int)d) & 1 )
                    d = 1.0 - (d - floor(d)) ;
                else
                    d = d - floor(d) ;
                break;
        }
        PCL_B(pPixel) = (BYTE)(PCL_B(&m_crStart) + (PCL_B(&m_crEnd)-PCL_B(&m_crStart)) * d) ;
        PCL_G(pPixel) = (BYTE)(PCL_G(&m_crStart) + (PCL_G(&m_crEnd)-PCL_G(&m_crStart)) * d) ;
        PCL_R(pPixel) = (BYTE)(PCL_R(&m_crStart) + (PCL_R(&m_crEnd)-PCL_R(&m_crStart)) * d) ;
    }
};
//-------------------------------------------------------------------------------------
/// Gradient fill linear (>=24 bit).
class FCEffectGradientLinear : public FCEffectGradient
{
public:
    /// Constructor.
    FCEffectGradientLinear (POINT ptStart, POINT ptEnd, RGBQUAD crStart, RGBQUAD crEnd, REPEAT_MODE nRepeat=REPEAT_NONE) : FCEffectGradient (crStart, crEnd, nRepeat)
    {
        if ((ptStart.x == ptEnd.x) && (ptStart.y == ptEnd.y))
        {
            ptEnd.x+=200 ; ptEnd.y+=200;
        }

        m_ptStart = ptStart; m_ptEnd = ptEnd;
        double   dx = m_ptStart.x - m_ptEnd.x ;
        double   dy = m_ptStart.y - m_ptEnd.y ;
        m_dist = sqrt (dx*dx + dy*dy) ;
        m_rat_x = (m_ptEnd.x - m_ptStart.x) / m_dist ;
        m_rat_y = (m_ptEnd.y - m_ptStart.y) / m_dist ;
    }
protected:
    // 0 <= n
    virtual double CalculateFactor (int x, int y)
    {
        double   d = m_rat_x * (x-m_ptStart.x) + m_rat_y * (y-m_ptStart.y) ;
        d = d / m_dist ;
        return (d < 0.0) ? 0.0 : d ;
    }
protected:
    POINT    m_ptStart, m_ptEnd ;
    double   m_rat_x, m_rat_y ;
    double   m_dist ;
};
//-------------------------------------------------------------------------------------
/// Gradient fill bilinear (>=24 bit).
class FCEffectGradientBiLinear : public FCEffectGradientLinear
{
public:
    /// Constructor.
    FCEffectGradientBiLinear (POINT ptStart, POINT ptEnd, RGBQUAD crStart, RGBQUAD crEnd, REPEAT_MODE nRepeat=REPEAT_NONE) : FCEffectGradientLinear (ptStart, ptEnd, crStart, crEnd, nRepeat) {}
private:
    // 0 <= n
    virtual double CalculateFactor (int x, int y)
    {
        double   d = m_rat_x * (x-m_ptStart.x) + m_rat_y * (y-m_ptStart.y) ;
        d = d / m_dist ;
        return (d < 0.0) ? -d : d ;
    }
};
//-------------------------------------------------------------------------------------
/// Gradient fill symmetric conical (>=24 bit).
class FCEffectGradientConicalSym : public FCEffectGradientLinear
{
public:
    /// Constructor.
    FCEffectGradientConicalSym (POINT ptCenter, POINT ptOut, RGBQUAD crStart, RGBQUAD crEnd) : FCEffectGradientLinear (ptCenter, ptOut, crStart, crEnd, REPEAT_NONE) {}
private:
    virtual double CalculateFactor (int x, int y)
    {
        if ((m_ptStart.x == x) && (m_ptStart.y == y))
            return 0.5 ;

        double   dx = x-m_ptStart.x, dy = y-m_ptStart.y ;
        double   r = sqrt (dx*dx + dy*dy) ;
        double   d = m_rat_x * dx / r + m_rat_y * dy / r ;
        d = FClamp (d, -1.0, 1.0) ;
        d = acos(d) / LIB_PI ;
        return d ;
    }
};
//-------------------------------------------------------------------------------------
/// Gradient fill anti-symmetric conical (>=24 bit).
class FCEffectGradientConicalASym : public FCEffectGradientLinear
{
public:
    /// Constructor.
    FCEffectGradientConicalASym (POINT ptCenter, POINT ptOut, RGBQUAD crStart, RGBQUAD crEnd) : FCEffectGradientLinear (ptCenter, ptOut, crStart, crEnd, REPEAT_NONE) {}
private:
    virtual double CalculateFactor (int x, int y)
    {
        if ((m_ptStart.x == x) && (m_ptStart.y == y))
            return 0.5 ;

        double   dx = x-m_ptStart.x, dy = y-m_ptStart.y ;
        double   ang0 = atan2 (m_rat_x, m_rat_y) + LIB_PI ;
        double   ang1 = atan2 (dx, dy) + LIB_PI ;
        double   ang = ang1 - ang0 ;
        if (ang < 0.0)
            ang += (2 * LIB_PI) ;
        return ang / (2 * LIB_PI) ;
    }
};
//-------------------------------------------------------------------------------------
/// Gradient fill rect (>=24 bit).
class FCEffectGradientRect : public FCEffectGradient
{
    double   m_cen_x, m_cen_y ;
    double   m_radius_x, m_radius_y ;
public:
    /// Constructor.
    FCEffectGradientRect (RECT rcRect, RGBQUAD crCenter, RGBQUAD crOut, REPEAT_MODE nRepeat=REPEAT_NONE) : FCEffectGradient (crCenter, crOut, nRepeat)
    {
        RECT   rc = rcRect ;
        if (IsRectEmpty(&rc))
        {
            assert(false) ;
            rc.right = rc.left + 200 ;
            rc.bottom = rc.top + 200 ;
        }

        m_cen_x = (rc.left + rc.right) / 2.0 ;
        m_cen_y = (rc.top + rc.bottom) / 2.0 ;
        m_radius_x = (rc.right - rc.left) / 2.0 ;
        m_radius_y = (rc.bottom - rc.top) / 2.0 ;
    }
private:
    // 0 <= n
    virtual double CalculateFactor (int x, int y)
    {
        double   rx = fabs ((x-m_cen_x) / m_radius_x) ;
        double   ry = fabs ((y-m_cen_y) / m_radius_y) ;
        return (rx < ry ? ry : rx) ;
    }
};
//-------------------------------------------------------------------------------------
/// Gradient fill radial (>=24 bit).
class FCEffectGradientRadial : public FCEffectGradient
{
    double   m_cen_x, m_cen_y ;
    double   m_radius_x, m_radius_y ;
public:
    /// Constructor.
    FCEffectGradientRadial (RECT rcEllipse, RGBQUAD crCenter, RGBQUAD crOut, REPEAT_MODE nRepeat=REPEAT_NONE) : FCEffectGradient (crCenter, crOut, nRepeat)
    {
        RECT   rc = rcEllipse ;
        if (IsRectEmpty(&rc))
        {
            assert(false) ;
            rc.right = rc.left + 200 ;
            rc.bottom = rc.top + 200 ;
        }

        m_cen_x = (rc.left + rc.right) / 2.0 ;
        m_cen_y = (rc.top + rc.bottom) / 2.0 ;
        m_radius_x = (rc.right - rc.left) / 2.0 ;
        m_radius_y = (rc.bottom - rc.top) / 2.0 ;
    }
private:
    // 0 <= n
    virtual double CalculateFactor (int x, int y)
    {
        double   dx = (x - m_cen_x) / m_radius_x ;
        double   dy = (y - m_cen_y) / m_radius_y ;
        return sqrt (dx*dx + dy*dy) ;
    }
};
