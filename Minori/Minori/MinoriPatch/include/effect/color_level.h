
//-------------------------------------------------------------------------------------
/// Color level (>=24 bit).
class FCEffectColorLevel : public FCImageEffect
{
public:
    /// color channel
    enum COLOR_CHANNEL
    {
        CHANNEL_RED = 1, /*!< red channel */
        CHANNEL_GREEN = 2, /*!< green channel */
        CHANNEL_BLUE = 4, /*!< blue channel */
        CHANNEL_RGB = CHANNEL_RED|CHANNEL_GREEN|CHANNEL_BLUE, /*!< RGB channel */
    };

    /// Constructor.
    FCEffectColorLevel (int input_low, int input_high, int output_low, int output_high, double fGamma, COLOR_CHANNEL nChannel)
    {
        for (int i=0 ; i < 3 ; i++)
        {
            m_input_low[i] = FClamp0255(input_low) ;
            m_input_high[i] = FClamp0255(input_high) ;
        }

        m_output_low = FClamp0255(output_low) ;
        m_output_high = FClamp0255(output_high) ;
        m_channel = nChannel ;
        m_gamma = fGamma ;
    }

protected:
    int     m_input_low[3], m_input_high[3] ;
    int     m_output_low, m_output_high ;
    double  m_gamma ;
    COLOR_CHANNEL   m_channel ;

private:
    BYTE CalculateValue (int n, int nLow, int nHigh)
    {
        double   d ;
        if (nLow != nHigh)
            d = (n - nLow) / (double)(nHigh - nLow) ;
        else
            d = n - nLow ;

        d = FClamp(d, 0.0, 1.0) ;

        if (m_gamma > 0)
        {
            d = pow (d, 1/m_gamma) ;
        }

        if (m_output_high >= m_output_low)
            d = d * (m_output_high - m_output_low) + m_output_low ;
        else
            d = m_output_low - d * (m_output_low - m_output_high) ;

        return FClamp0255(d) ;
    }

    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        if (m_channel & CHANNEL_BLUE)
        {
            PCL_B(pPixel) = CalculateValue (PCL_B(pPixel), m_input_low[0], m_input_high[0]) ;
        }
        if (m_channel & CHANNEL_GREEN)
        {
            PCL_G(pPixel) = CalculateValue (PCL_G(pPixel), m_input_low[1], m_input_high[1]) ;
        }
        if (m_channel & CHANNEL_RED)
        {
            PCL_R(pPixel) = CalculateValue (PCL_R(pPixel), m_input_low[2], m_input_high[2]) ;
        }
    }
};

//-------------------------------------------------------------------------------------
/// Auto color level (>=24 bit).
class FCEffectAutoColorLevel : public FCEffectColorLevel
{
public:
    /// Constructor.
    FCEffectAutoColorLevel() : FCEffectColorLevel(0, 255, 0, 255, 1.0, FCEffectColorLevel::CHANNEL_RGB) {}

private:
    virtual void OnBeforeProcess (FCObjImage& img)
    {
        FCEffectGetHistogram   c ;
        img.ApplyEffect(c) ;

        AutoColorLevelChannel (c.m_blue, 0) ;
        AutoColorLevelChannel (c.m_green, 1) ;
        AutoColorLevelChannel (c.m_red, 2) ;
    }

    void AutoColorLevelChannel (const std::vector<int>& histogram_tab, int nIndex)
    {
        // get count
        double   nCount = 0 ;
        for (int i=0 ; i < 256 ; i++)
        {
            nCount += histogram_tab[i] ;
        }

        if (nCount == 0)
        {
            m_input_low[nIndex] = m_input_high[nIndex] = 0 ;
        }
        else
        {
            m_input_low[nIndex] = 0 ;
            m_input_high[nIndex] = 255 ;

            // set low input
            double   new_count = 0 ;
            for (int i=0 ; i < 255 ; i++)
            {
                new_count += histogram_tab[i] ;
                double   percentage = new_count / nCount ;
                double   next_percentage = (new_count + histogram_tab[i+1]) / nCount ;
                if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
                {
                    m_input_low[nIndex] = i + 1 ;
                    break ;
                }
            }

            // set high input
            new_count = 0 ;
            for (int i=255 ; i > 0 ; i--)
            {
                new_count += histogram_tab[i] ;
                double   percentage = new_count / nCount ;
                double   next_percentage = (new_count + histogram_tab[i-1]) / nCount ;
                if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
                {
                    m_input_high[nIndex] = i - 1 ;
                    break ;
                }
            }
        }
    }
};
