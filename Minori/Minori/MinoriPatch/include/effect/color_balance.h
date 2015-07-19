
//-------------------------------------------------------------------------------------
/// Color balance (>=24 bit).
class FCEffectColorBalance : public FCImageEffect
{
    BYTE   m_r[256], m_g[256], m_b[256] ;
    int    m_preserve_luminosity ;
public:
    /// region
    enum TONE_REGION
    {
        TONE_SHADOWS = 0, /*!< shadow */
        TONE_MIDTONES = 1, /*!< mid tone */
        TONE_HIGHLIGHTS = 2, /*!< high light */
    };

    /**
        Constructor \n
        param -100 <= cyan_red, magenta_green, yellow_blue <= 100
    */
    FCEffectColorBalance (bool bPreserveLuminosity, TONE_REGION nTone, int cyan_red, int magenta_green, int yellow_blue)
    {
        m_preserve_luminosity = (bPreserveLuminosity ? 1 : 0) ;

        int   cyan_red_rgn[3] = {0},
              magenta_green_rgn[3] = {0},
              yellow_blue_rgn[3] = {0} ;
        cyan_red_rgn[nTone] = cyan_red ;
        magenta_green_rgn[nTone] = magenta_green ;
        yellow_blue_rgn[nTone] = yellow_blue ;

        // add for lightening, sub for darkening
        std::vector<double>   highlights_add(256), midtones_add(256), shadows_add(256),
                              highlights_sub(256), midtones_sub(256), shadows_sub(256) ;
        for (int i=0 ; i < 256 ; i++)
        {
            double   dl = 1.075 - 1 / (i / 16.0 + 1) ;

            double   dm = (i - 127.0) / 127.0 ;
            dm = 0.667 * (1 - dm * dm) ;

            shadows_add[i] = shadows_sub[255 - i] = dl ;
            highlights_add[255 - i] = highlights_sub[i] = dl ;
            midtones_add[i] = midtones_sub[i] = dm ;
        }

        // Set the transfer arrays (for speed)
        double   * cyan_red_transfer[3], * magenta_green_transfer[3], * yellow_blue_transfer[3] ;
        cyan_red_transfer[TONE_SHADOWS] = (cyan_red_rgn[TONE_SHADOWS] > 0) ? &shadows_add[0] : &shadows_sub[0] ;
        cyan_red_transfer[TONE_MIDTONES] = (cyan_red_rgn[TONE_MIDTONES] > 0) ? &midtones_add[0] : &midtones_sub[0] ;
        cyan_red_transfer[TONE_HIGHLIGHTS] = (cyan_red_rgn[TONE_HIGHLIGHTS] > 0) ? &highlights_add[0] : &highlights_sub[0] ;
        magenta_green_transfer[TONE_SHADOWS] = (magenta_green_rgn[TONE_SHADOWS] > 0) ? &shadows_add[0] : &shadows_sub[0] ;
        magenta_green_transfer[TONE_MIDTONES] = (magenta_green_rgn[TONE_MIDTONES] > 0) ? &midtones_add[0] : &midtones_sub[0] ;
        magenta_green_transfer[TONE_HIGHLIGHTS] = (magenta_green_rgn[TONE_HIGHLIGHTS] > 0) ? &highlights_add[0] : &highlights_sub[0] ;
        yellow_blue_transfer[TONE_SHADOWS] = (yellow_blue_rgn[TONE_SHADOWS] > 0) ? &shadows_add[0] : &shadows_sub[0] ;
        yellow_blue_transfer[TONE_MIDTONES] = (yellow_blue_rgn[TONE_MIDTONES] > 0) ? &midtones_add[0] : &midtones_sub[0] ;
        yellow_blue_transfer[TONE_HIGHLIGHTS] = (yellow_blue_rgn[TONE_HIGHLIGHTS] > 0) ? &highlights_add[0] : &highlights_sub[0] ;

        for (int i=0 ; i < 256 ; i++)
        {
            int   r_n=i, g_n=i, b_n=i ;

            r_n += (int)(cyan_red_rgn[TONE_SHADOWS] * cyan_red_transfer[TONE_SHADOWS][r_n]);        r_n = FClamp0255(r_n);
            r_n += (int)(cyan_red_rgn[TONE_MIDTONES] * cyan_red_transfer[TONE_MIDTONES][r_n]);      r_n = FClamp0255(r_n);
            r_n += (int)(cyan_red_rgn[TONE_HIGHLIGHTS] * cyan_red_transfer[TONE_HIGHLIGHTS][r_n]);  r_n = FClamp0255(r_n);

            g_n += (int)(magenta_green_rgn[TONE_SHADOWS] * magenta_green_transfer[TONE_SHADOWS][g_n]);        g_n = FClamp0255(g_n);
            g_n += (int)(magenta_green_rgn[TONE_MIDTONES] * magenta_green_transfer[TONE_MIDTONES][g_n]);      g_n = FClamp0255(g_n);
            g_n += (int)(magenta_green_rgn[TONE_HIGHLIGHTS] * magenta_green_transfer[TONE_HIGHLIGHTS][g_n]);  g_n = FClamp0255(g_n);

            b_n += (int)(yellow_blue_rgn[TONE_SHADOWS] * yellow_blue_transfer[TONE_SHADOWS][b_n]);        b_n = FClamp0255(b_n);
            b_n += (int)(yellow_blue_rgn[TONE_MIDTONES] * yellow_blue_transfer[TONE_MIDTONES][b_n]);      b_n = FClamp0255(b_n);
            b_n += (int)(yellow_blue_rgn[TONE_HIGHLIGHTS] * yellow_blue_transfer[TONE_HIGHLIGHTS][b_n]);  b_n = FClamp0255(b_n);

            m_r[i] = (BYTE)r_n ;
            m_g[i] = (BYTE)g_n ;
            m_b[i] = (BYTE)b_n ;
        }
    }

private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        RGBQUAD   rgb = FCColor (m_r[PCL_R(pPixel)], m_g[PCL_G(pPixel)], m_b[PCL_B(pPixel)]) ;
        if (m_preserve_luminosity)
        {
            double   H, L, S ;
            FCColor::RGBtoHLS (&rgb, H, L, S) ;

            // calculate old L value
            int   px[] = {PCL_R(pPixel), PCL_G(pPixel), PCL_B(pPixel)} ;
            int   cmax = *std::max_element(px, px+3) ;
            int   cmin = *std::min_element(px, px+3) ;
            L = (cmax + cmin) / 2.0 / 255.0 ;

            rgb = FCColor::HLStoRGB (H, L, S) ;
        }
        FCColor::CopyPixelBPP (pPixel, &rgb, 24) ;
    }
};
