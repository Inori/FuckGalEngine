#ifdef IMAGESTONE_USE_FREEIMAGE

//-------------------------------------------------------------------------------------
/// Quantize image to 8bpp image using FreeImage.
class FCEffectFreeImageQuantize : public FCImageEffect
{
    // Find a color unused in image.
    class FCEffectGetKeyColor : public FCImageEffect
    {
        enum
        {
            MAX_COLOR_COUNT = 0x1000000, // 1 << 24
        };
        std::vector<BYTE>   m_map ;
    public:
        FCEffectGetKeyColor() : m_map ((size_t)MAX_COLOR_COUNT+1, (BYTE)0) {}

        RGBQUAD GetKeyColor()
        {
            RGBQUAD   cr = {0,0,0,0} ;
            for (int i=0 ; i < MAX_COLOR_COUNT ; i++)
            {
                if (!m_map[i])
                {
                    *(DWORD*)&cr = i ;
                    break ;
                }
            }
            return cr ;
        }

    private:
        virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
        {
            DWORD   i=0 ;
            FCColor::CopyPixelBPP (&i, pPixel, 24) ;
            m_map[i] = 1 ;
        }
    };

    POINT   m_pt ;
    int     m_nTransparency ;
    FCObjImage   m_bak ;

public:
    /// Constructor.
    FCEffectFreeImageQuantize() : m_nTransparency(-1)
    {
        m_pt.x = m_pt.y = -1 ;
        FreeImage_Initialise(TRUE) ;
    }
    virtual ~FCEffectFreeImageQuantize()
    {
        FreeImage_DeInitialise() ;
    }

    int GetTransparencyIndex() const {return m_nTransparency;}

private:
    virtual PROCESS_TYPE QueryProcessType() {return PROCESS_TYPE_WHOLE;}
    virtual bool IsSupport (const FCObjImage& img)
    {
        return img.IsValidImage() ;
    }
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        if (img.ColorBits() == 32)
        {
            // find a key color
            FCEffectGetKeyColor   aCmd ;
            img.ApplyEffect (aCmd) ;

            RGBQUAD   crKey = aCmd.GetKeyColor() ;

            // set pixel (alpha=0) to key color
            for (int y=0 ; y < img.Height() ; y++)
            {
                for (int x=0 ; x < img.Width() ; x++)
                {
                    BYTE   * p = img.GetBits(x,y) ;
                    if (PCL_A(p) == 0)
                    {
                        FCColor::CopyPixelBPP (p, &crKey, 24) ;
                        m_pt.x=x ; m_pt.y=y ;
                        m_nTransparency = 1 ;
                    }
                }
            }
        }

        m_bak = img ;
        m_bak.ConvertTo24Bit() ; // easy to process
    }
    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress)
    {
        FIBITMAP   * pFI = AllocateFreeImage (m_bak) ;
        if (pFI)
        {
            FIBITMAP   * pQu = FreeImage_ColorQuantizeEx (pFI, FIQ_NNQUANT, 256) ;
            if (pQu)
            {
                FreeImage_to_FCImage (pQu, img) ;
                FreeImage_Unload (pQu) ;
            }
            FreeImage_Unload (pFI) ;
        }

        // set transparency
        if (img.IsValidImage() && (m_nTransparency == 1) && img.IsInside(m_pt.x,m_pt.y))
        {
            m_nTransparency = *img.GetBits(m_pt.x,m_pt.y) ;
        }
    }

private:
    static void FreeImage_to_FCImage (FIBITMAP* pFIimg, FCObjImage& img)
    {
        BITMAPINFOHEADER   * pInfo = FreeImage_GetInfoHeader(pFIimg) ;

        // create image
        if (pInfo && img.Create(pInfo->biWidth, pInfo->biHeight, pInfo->biBitCount))
        {
            // set palette
            if (img.ColorBits() <= 8)
            {
                RGBQUAD   pPal[256] = {0} ;
                memcpy (pPal, FreeImage_GetPalette(pFIimg), FreeImage_GetColorsUsed(pFIimg) * sizeof(RGBQUAD)) ;
                img.SetColorTable (pPal) ;
            }

            // set pixel
            memcpy (img.GetMemStart(), FreeImage_GetBits(pFIimg), img.GetPitch()*img.Height()) ;
        }
    }

    static FIBITMAP* AllocateFreeImage (const FCObjImage& img)
    {
        if (!img.IsValidImage())
            return 0 ;

        // create FreeImage object
        FIBITMAP   * pFIimg = FreeImage_Allocate (img.Width(), img.Height(), img.ColorBits()) ;
        if (!pFIimg)
            return 0 ;

        // set pixel
        memcpy (FreeImage_GetBits(pFIimg), img.GetMemStart(), img.GetPitch()*img.Height()) ;

        // clear, default is 2835 = 72dpi
        FreeImage_SetDotsPerMeterX (pFIimg, 0) ;
        FreeImage_SetDotsPerMeterY (pFIimg, 0) ;

        return pFIimg ;
    }
};

#endif
