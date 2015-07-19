
//-------------------------------------------------------------------------------------
/// Gauss blur (>=24 bit).
class FCEffectBlur_Gauss : public FCImageEffect
{
public:
    enum BLUR_COMPONENT
    {
        BLUR_COMPONENT_ALL = 1,
        BLUR_COMPONENT_ONLY_ALPHA = 2,
    };

    /**
        Constructor \n
        nSize >= 1 \n
        bCopyEdge - set false to blur border with alpha==0 \n
        blur_component - can be FCEffectBlur_Gauss::BLUR_COMPONENT_ALL or FCEffectBlur_Gauss::BLUR_COMPONENT_ONLY_ALPHA
    */
    FCEffectBlur_Gauss (int nSize, bool bCopyEdge, BLUR_COMPONENT blur_component=BLUR_COMPONENT_ALL)
    {
        m_copy_edge = (bCopyEdge ? 1 : 0) ;
        m_component = blur_component ;
        MakeKernel (((nSize >= 1) ? nSize : 1) / 2.0) ;
        m_padding = (int)m_kernel.size() / 2 ;
    }

protected:
    /// Derived class can modify pPixel that has been blured.
    virtual void OnFinishPixel (FCObjImage& img, int x, int y, BYTE* pPixel) {}

private:
    int      m_copy_edge ;
    BLUR_COMPONENT   m_component ;
    std::vector<double>   m_kernel ;
    int      m_padding ;

    int      m_bpp ; // 24 or 32
    RGBQUAD  * m_line_buf ;

private:
    enum
    {
        MAX_RADIUS = 100,
    };

    static void DivideWeight (std::vector<double>& ls)
    {
        double   t = 0 ;
        for (size_t i=0 ; i < ls.size() ; i++)
            t += ls[i] ;

        for (size_t i=0 ; i < ls.size() ; i++)
            ls[i] /= t ;
    }

    void MakeKernel (double fRadius)
    {
        m_kernel.reserve (2*MAX_RADIUS + 1) ;
        for (int i = -MAX_RADIUS ; i <= MAX_RADIUS ; i++)
        {
            double   t = i / fRadius ;
            m_kernel.push_back (exp (-t*t/2)) ;
        }

        // divide weight
        DivideWeight(m_kernel) ;

        // remove small weight
        double   max_precision = 1 / (2.0 * 255) ;
        double   fTotal = 0 ;
        for (int i=0 ; i < MAX_RADIUS ; i++)
        {
            fTotal += (2 * m_kernel.front()) ;

            if (fTotal < max_precision)
            {
                m_kernel.erase (m_kernel.begin()) ;
                m_kernel.pop_back() ;
            }
            else
            {
                break ;
            }
        }

        // correct sum
        DivideWeight(m_kernel) ;
    }

private:
    void BlurOnePixel (BYTE* pDest, const RGBQUAD* pSrc)
    {
        double   * pKer = &m_kernel[0] ;
        double   r=0, g=0, b=0, a=0 ;
        for (size_t i=0 ; i < m_kernel.size() ; i++, pSrc++)
        {
            if (PCL_A(pSrc))
            {
                double   aw = PCL_A(pSrc) * pKer[i] ;
                if (m_component != BLUR_COMPONENT_ONLY_ALPHA)
                {
                    b += (PCL_B(pSrc) * aw) ;
                    g += (PCL_G(pSrc) * aw) ;
                    r += (PCL_R(pSrc) * aw) ;
                }
                a += aw ;
            }
        }

        RGBQUAD   c ;
        PCL_A(&c) = FClamp0255(a) ;

        if (m_component == BLUR_COMPONENT_ONLY_ALPHA)
        {
            assert (m_bpp == 32) ;
            PCL_A(pDest) = PCL_A(&c) ;
        }
        else
        {
            if (PCL_A(&c))
            {
                PCL_B(&c) = FClamp0255(b/a) ;
                PCL_G(&c) = FClamp0255(g/a) ;
                PCL_R(&c) = FClamp0255(r/a) ;
            }
            FCColor::CopyPixelBPP (pDest, &c, m_bpp) ;
        }
    }

    RGBQUAD GetRGBQUAD (const void* pPixel)
    {
        RGBQUAD   c = FCColor(0,0,0,0xFF) ; // default alpha is 255
        FCColor::CopyPixelBPP (&c, pPixel, m_bpp) ;
        return c ;
    }

    void FillPadding (RGBQUAD*& d, const void* pPixel)
    {
        for (int i=0 ; i < m_padding ; i++, d++)
        {
            *d = (m_copy_edge ? GetRGBQUAD(pPixel) : FCColor(0xFF,0xFF,0xFF,0)) ;
        }
    }

    void HorizonGetLine (FCObjImage& img, int y, RGBQUAD* d)
    {
        FillPadding (d, img.GetBits(0,y)) ;
        for (int x=0 ; x < img.Width() ; x++, d++)
        {
            *d = GetRGBQUAD(img.GetBits(x,y)) ;
        }
        FillPadding (d, img.GetBits(img.Width()-1,y)) ;
    }

    void VerticalGetLine (FCObjImage& img, int x, RGBQUAD* d)
    {
        FillPadding (d, img.GetBits(x,0)) ;
        for (int y=0 ; y < img.Height() ; y++, d++)
        {
            *d = GetRGBQUAD(img.GetBits(x,y)) ;
        }
        FillPadding (d, img.GetBits(x,img.Height()-1)) ;
    }

    void BlurVertical (FCObjImage& img, FCProgressObserver* pProgress)
    {
        for (int x=0 ; x < img.Width() ; x++)
        {
            VerticalGetLine (img, x, m_line_buf) ;

            RGBQUAD   * s = m_line_buf ;
            for (int y=0 ; y < img.Height() ; y++, s++)
            {
                BlurOnePixel (img.GetBits(x,y), s) ;
            }

            if (pProgress)
            {
                if (!pProgress->UpdateProgress (0)) // need call it for cancel process
                    break ;
            }
        }
    }

    void BlurHorizon (FCObjImage& img, FCProgressObserver* pProgress)
    {
        for (int y=0 ; y < img.Height() ; y++)
        {
            HorizonGetLine (img, y, m_line_buf) ;

            RGBQUAD   * s = m_line_buf ;
            for (int x=0 ; x < img.Width() ; x++, s++)
            {
                BYTE   * pPixel = img.GetBits(x,y) ;
                BlurOnePixel (pPixel, s) ;
                OnFinishPixel (img, x, y, pPixel) ;
            }

            if (pProgress)
            {
                if (!pProgress->UpdateProgress (100 * (y+1) / img.Height()))
                    break ;
            }
        }
    }

private:
    virtual PROCESS_TYPE QueryProcessType()
    {
        return PROCESS_TYPE_WHOLE ;
    }

    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress)
    {
        if (img.IsValidImage())
        {
            // prepare param
            m_bpp = img.ColorBits() ;

            int   w = img.Width() ;
            int   h = img.Height() ;
            int   nCount = ((w > h) ? w : h) + 2*m_padding + 2 ; // +2 is not necessary
            m_line_buf = new RGBQUAD[nCount] ;

            BlurVertical (img, pProgress) ;
            BlurHorizon (img, pProgress) ;

            delete[] m_line_buf ;
        }
    }
};
