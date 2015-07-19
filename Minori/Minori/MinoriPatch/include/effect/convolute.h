
//-------------------------------------------------------------------------------------
/// Convolute (>= 24 bit)
class FCEffectConvolute : public FCImageEffect
{
    std::vector<int>   m_embuffer ;
    int   * m_element ;
    int   m_size, m_divisor, m_offset ;
    FCObjImage   m_bak ;
public:
    /// Constructor.
    FCEffectConvolute()
    {
        m_size=0 ; m_offset=0 ; m_divisor=1 ;
    }

    /**
        Set convolute kernel \n
        nElements - array from top-left of matrix, count nSize * nSize \n
        nSize - width of matrix
    */
    void SetKernel (const int* nElements, int nSize, int iDivisor, int nOffset=0)
    {
        if (!nElements || (nSize < 1))
            return ;

        m_embuffer.clear() ;
        for (int i=0 ; i < (nSize * nSize) ; i++)
        {
            m_embuffer.push_back (nElements[i]) ;
        }
        m_element = &m_embuffer[0] ;

        m_size = nSize ;
        m_divisor = ((iDivisor >= 1) ? iDivisor : 1) ;
        m_offset = nOffset ;
    }

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        RECT   rc ;
        rc.left = x - m_size/2 ;
        rc.top = y - m_size/2 ;
        rc.right = rc.left + m_size ;
        rc.bottom = rc.top + m_size ;

        // calculate the sum of sub-block
        int   r=0, g=0, b=0, i=0 ;
        for (int iy=rc.top ; iy < rc.bottom ; iy++)
        {
            for (int ix=rc.left ; ix < rc.right ; ix++)
            {
                int    sx = FClamp(ix, 0, img.Width()-1) ;
                int    sy = FClamp(iy, 0, img.Height()-1) ;
                BYTE   * pOld = m_bak.GetBits (sx,sy) ;
                b += (PCL_B(pOld) * m_element[i]) ;
                g += (PCL_G(pOld) * m_element[i]) ;
                r += (PCL_R(pOld) * m_element[i]) ;
                i++ ;
            }
        }

        // set pixel
        PCL_B(pPixel) = FClamp0255 (m_offset + b / m_divisor) ;
        PCL_G(pPixel) = FClamp0255 (m_offset + g / m_divisor) ;
        PCL_R(pPixel) = FClamp0255 (m_offset + r / m_divisor) ;
    }
};
//-------------------------------------------------------------------------------------
/// Sharp (laplacian template) (>=24 bit).
class FCEffectSharp : public FCEffectConvolute
{
public:
    /**
        Constructor \n
        nStep >= 1
    */
    FCEffectSharp (int nStep)
    {
        int   arKernel[] = {-1,-1,-1,-1,8+nStep,-1,-1,-1,-1} ;
        SetKernel (arKernel, 3, nStep, 0) ;
    }
};
