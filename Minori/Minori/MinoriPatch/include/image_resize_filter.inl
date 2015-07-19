
//-------------------------------------------------------------------------------------
class FCFilterBase
{
    double   m_width ;
public:
    FCFilterBase (double dWidth) : m_width (dWidth) {}
    virtual ~FCFilterBase() {}

    double GetWidth() const {return m_width;}

    virtual double Filter (double d) const =0 ;
};
//-------------------------------------------------------------------------------------
class FCFilterBox : public FCFilterBase
{
public:
    FCFilterBox() : FCFilterBase(0.5) {}

    virtual double Filter (double d) const
    {
        if (d < 0)
            d = -d ;
        return (d > GetWidth() ? 0 : 1) ;
    }
};
//-------------------------------------------------------------------------------------
class FCFilterBilinear : public FCFilterBase
{
public:
    FCFilterBilinear () : FCFilterBase(1) {}

    virtual double Filter (double d) const
    {
        if (d < 0)
            d = -d ;
        return (d < GetWidth() ? GetWidth() - d : 0) ;
    }
};
//-------------------------------------------------------------------------------------
class FCFilterBicubic : public FCFilterBase
{
    double p0, p2, p3;
    double q0, q1, q2, q3;

public:
    FCFilterBicubic (double b = 1/3.0, double c = 1/3.0) : FCFilterBase(2)
    {
        p0 = (6 - 2*b) / 6;
        p2 = (-18 + 12*b + 6*c) / 6;
        p3 = (12 - 9*b - 6*c) / 6;
        q0 = (8*b + 24*c) / 6;
        q1 = (-12*b - 48*c) / 6;
        q2 = (6*b + 30*c) / 6;
        q3 = (-b - 6*c) / 6;
    }

    virtual double Filter (double d) const
    { 
        if (d < 0)
            d = -d ;

        if(d < 1)
            return (p0 + d*d*(p2 + d*p3));
        if(d < 2)
            return (q0 + d*(q1 + d*(q2 + d*q3)));
        return 0;
    }
};
//-------------------------------------------------------------------------------------
class FCFilterCatmullRom : public FCFilterBase
{
public:
    FCFilterCatmullRom() : FCFilterBase(2) {}

    virtual double Filter (double d) const
    { 
        if(d < -2) return 0;
        if(d < -1) return (0.5*(4 + d*(8 + d*(5 + d))));
        if(d < 0)  return (0.5*(2 + d*d*(-5 - 3*d)));
        if(d < 1)  return (0.5*(2 + d*d*(-5 + 3*d)));
        if(d < 2)  return (0.5*(4 + d*(-8 + d*(5 - d))));
        return 0;
    }
};
