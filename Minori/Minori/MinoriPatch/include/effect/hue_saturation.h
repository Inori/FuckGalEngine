
//-------------------------------------------------------------------------------------
/// Hue and saturation (>=24 bit).
class FCEffectHueSaturation : public FCImageEffect
{
    int   hue_transfer[6][256] ;
    int   saturation_transfer[6][256] ;
public:
    /**
        Constructor \n
        param -100 <= nHue <= 100, 0 means not change \n
        param -100 <= nSaturation <= 100, 0 means not change
    */
    FCEffectHueSaturation (int nHue, int nSaturation)
    {
        nHue = 180 * FClamp(nHue, -100, 100) / 100 ;
        nSaturation = FClamp(nSaturation, -100, 100) ;

        for (int hue=0 ; hue < 6 ; hue++)
        {
            for (int i=0 ; i < 256 ; i++)
            {
                int   v = nHue * 255 / 360 ;
                if ((i + v) < 0)
                    hue_transfer[hue][i] = 255 + (i + v) ;
                else if ((i + v) > 255)
                    hue_transfer[hue][i] = i + v - 255 ;
                else
                    hue_transfer[hue][i] = i + v ;

                // saturation
                v = nSaturation * 255 / 100 ;
                saturation_transfer[hue][i] = FClamp0255 (i * (255 + v) / 255) ;
            }
        }
    }
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        double   h, l, s ;
        FCColor::RGBtoHLS (pPixel, h, l, s) ;
        h *= 255 ;
        s *= 255 ;

        int   hue = 0 ;
        if (h < 21)
            hue = 0 ;
        else if (h < 64)
            hue = 1 ;
        else if (h < 106)
            hue = 2 ;
        else if (h < 149)
            hue = 3 ;
        else if (h < 192)
            hue = 4 ;
        else if (h < 234)
            hue = 5 ;

        h = hue_transfer[hue][FClamp0255(h)] ;
        s = saturation_transfer[hue][FClamp0255(s)] ;

        RGBQUAD   cr = FCColor::HLStoRGB(h/255.0, l, s/255.0) ;
        FCColor::CopyPixelBPP (pPixel, &cr, 24) ;
    }
};
