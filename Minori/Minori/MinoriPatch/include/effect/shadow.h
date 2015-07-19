
//-------------------------------------------------------------------------------------
/**
    Add shadow (32 bit).
    this effect will change image's size.
*/
class FCEffectAddShadow : public FCImageEffect
{
    SHADOWDATA   m_shadow ;
    int          m_padding ;
public:
    /// Constructor.
    FCEffectAddShadow (SHADOWDATA sd)
    {
        m_shadow = sd ;
        m_shadow.m_smooth = ((sd.m_smooth >= 2) ? sd.m_smooth : 2) ;
        m_shadow.m_opacity = FClamp (m_shadow.m_opacity, 0, 100) ;
        m_shadow.m_offset_x = FClamp(m_shadow.m_offset_x, -50, 50) ;
        m_shadow.m_offset_y = FClamp(m_shadow.m_offset_y, -50, 50) ;
        m_padding = 0 ;
    }

    void SetPadding (int nPadding)
    {
        m_padding = nPadding ;
    }

private:
    virtual PROCESS_TYPE QueryProcessType()
    {
        return PROCESS_TYPE_WHOLE ;
    }

    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() && (img.ColorBits() == 32) ;
    }

    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress)
    {
        // calculate new image size
        RECT    rcImg = {0, 0, img.Width(), img.Height()},
                rcShadowOffset = rcImg ;
        ::OffsetRect (&rcShadowOffset, m_shadow.m_offset_x, m_shadow.m_offset_y) ;
        RECT    rcShadow = rcShadowOffset ;
        ::InflateRect (&rcShadow, m_padding+m_shadow.m_smooth, m_padding+m_shadow.m_smooth) ;
        RECT    rcResult ;
        ::UnionRect (&rcResult, &rcImg, &rcShadow) ;

        // backup image
        FCObjImage   imgOld ;
        img.SwapImage(imgOld) ;

        // create shadow background and box-blur it
        img.Create (rcResult.right - rcResult.left, rcResult.bottom - rcResult.top, 32) ;

        int   nStartX = rcShadowOffset.left - rcResult.left ;
        int   nStartY = rcShadowOffset.top - rcResult.top ;
        for (int y=0 ; y < img.Height() ; y++)
        {
            for (int x=0 ; x < img.Width() ; x++)
            {
                int   sx = x - nStartX ;
                int   sy = y - nStartY ;

                if (imgOld.IsInside(sx,sy))
                {
                    PCL_A(&m_shadow.m_color) = (BYTE)(PCL_A(imgOld.GetBits(sx,sy)) * m_shadow.m_opacity / 100) ;
                }
                else
                {
                    PCL_A(&m_shadow.m_color) = 0 ;
                }
                *(RGBQUAD*)img.GetBits(x,y) = m_shadow.m_color ;
            }
        }

        // box-blur shadow back
        FCEffectBlur_Gauss   blur_cmd (m_shadow.m_smooth, false, FCEffectBlur_Gauss::BLUR_COMPONENT_ONLY_ALPHA) ;
        img.ApplyEffect (blur_cmd, pProgress) ;

        // combine origin image
        img.CombineImage (imgOld, rcImg.left-rcResult.left, rcImg.top-rcResult.top) ;
    }
};
