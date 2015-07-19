
//-------------------------------------------------------------------------------------
/// Video (>=24 bit).
class FCEffectVideo : public FCImageEffect
{
public:
    enum VIDEO_TYPE {VIDEO_STAGGERED=0, VIDEO_TRIPED=1, VIDEO_3X3=2, VIDEO_DOTS=3} ;
    /**
        Constructor \n
        nVideoType - FCEffectVideo::VIDEO_STAGGERED / FCEffectVideo::VIDEO_TRIPED / FCEffectVideo::VIDEO_3X3 / FCEffectVideo::VIDEO_DOTS
    */
    FCEffectVideo (VIDEO_TYPE nVideoType) : m_type(nVideoType) {}
private:
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
    {
        int   pattern_width[] = {2, 1, 3, 5} ;
        int   pattern_height[] = {6, 3, 3, 15} ;
        int   video_pattern[4][15 * 5] =
        {
            {
                0, 1,
                0, 2,
                1, 2,
                1, 0,
                2, 0,
                2, 1,
            },
            {
                0,
                1,
                2,
            },
            {
                0, 1, 2,
                2, 0, 1,
                1, 2, 0,
            },
            {
                0, 1, 2, 0, 0,
                1, 1, 1, 2, 0,
                0, 1, 2, 2, 2,
                0, 0, 1, 2, 0,
                0, 1, 1, 1, 2,
                2, 0, 1, 2, 2,
                0, 0, 0, 1, 2,
                2, 0, 1, 1, 1,
                2, 2, 0, 1, 2,
                2, 0, 0, 0, 1,
                1, 2, 0, 1, 1,
                2, 2, 2, 0, 1,
                1, 2, 0, 0, 0,
                1, 1, 2, 0, 1,
                1, 2, 2, 2, 0,
            }
        };

        int   nWidth = pattern_width[m_type],
              nHeight = pattern_height[m_type] ;
        for (int i=0 ; i < 3 ; i++)
        {
            if (video_pattern[m_type][nWidth * (y%nHeight) + (x%nWidth)] == i)
                pPixel[i] = FClamp0255 (2 * pPixel[i]) ;
        }
    }

    VIDEO_TYPE   m_type ;
};
