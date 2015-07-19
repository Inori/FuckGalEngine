
//-------------------------------------------------------------------------------------
/// Save a funny ASCII stream (>=24 bit).
class FCEffectExportAscII : public FCImageEffect
{
    char     m_index[95] ;
    int      m_gray[95] ;
    std::string   m_ascii ;
    FCObjImage    m_bak ;
public:
    /// Constructor.
    FCEffectExportAscII()
    {
        char   ch[95] =
        {
            ' ',
            '`','1','2','3','4','5','6','7','8','9','0','-','=','\\',
            'q','w','e','r','t','y','u','i','o','p','[',']',
            'a','s','d','f','g','h','j','k','l',';','\'',
            'z','x','c','v','b','n','m',',','.','/',
            '~','!','@','#','$','%','^','&','*','(',')','_','+','|',
            'Q','W','E','R','T','Y','U','I','O','P','{','}',
            'A','S','D','F','G','H','J','K','L',':','"',
            'Z','X','C','V','B','N','M','<','>','?'
        };
        int   gr[95] =
        {
            0,
            7,22,28,31,31,27,32,22,38,32,40, 6,12,20,38,32,26,20,24,40,
            29,24,28,38,32,32,26,22,34,24,44,33,32,32,24,16, 6,22,26,22,
            26,34,29,35,10, 6,20,14,22,47,42,34,40,10,35,21,22,22,16,14,
            26,40,39,29,38,22,28,36,22,36,30,22,22,36,26,36,25,34,38,24,
            36,22,12,12,26,30,30,34,39,42,41,18,18,22
        };

        // Bubble Sort
        for (int i=0 ; i < 94 ; i++)
            for (int j=i+1 ; j < 95 ; j++)
                if (gr[i] > gr[j])
                {
                    std::swap (ch[i], ch[j]) ;
                    std::swap (gr[i], gr[j]) ;
                }

        memcpy (m_index, ch, 95*sizeof(char)) ;
        memcpy (m_gray, gr, 95*sizeof(int)) ;
    }

    /// Get text after execute command.
    std::string GetASCII() const {return m_ascii;}

private:
    virtual PROCESS_TYPE QueryProcessType()
    {
        return PROCESS_TYPE_WHOLE ;
    }

    virtual void OnBeforeProcess (FCObjImage& img)
    {
        m_bak = img ;

        FCEffectInvert   c ; // most of image is brightness
        m_bak.ApplyEffect (c) ;
    }

    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress)
    {
        int   nTransWidth = img.Width() / 8,
              nTransHeight = img.Height() / 16 ;
        for (int y=0 ; y < nTransHeight ; y++)
        {
            for (int x=0 ; x < nTransWidth ; x++)
            {
                int     nGray = 0 ;
                for (int k=0 ; k < 16 ; k++)
                {
                    for(int h=0 ; h < 8 ; h++)
                    {
                        nGray += FCColor::GetGrayscale(m_bak.GetBits (8*x+h, y*16+k)) ;
                    }
                }

                nGray /= 16*8 ;
                nGray = m_gray[94] * nGray / 255 ;
                int   t = 0 ;
                while (m_gray[t+1] < nGray)
                    t++ ;
                m_ascii += m_index[t] ;
            }
            m_ascii += "\r\n" ;
        }
    }
};
