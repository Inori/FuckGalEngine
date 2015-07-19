
//-------------------------------------------------------------------------------------
/// Smooth edge (32 bit).
class FCEffectSmoothEdge : public FCImageEffect
{
    int   m_radius ; // >=1
    FCObjImage   m_halo ;
public:
    /**
        Constructor \n
        nRadius >= 1
    */
    FCEffectSmoothEdge (int nRadius)
    {
        m_radius = ((nRadius >= 1) ? nRadius : 1) ;
        m_radius += 1 ;

        // create halo image
        CEffectCreateHalo   cmd(m_radius) ;
        m_halo.ApplyEffect (cmd) ;
    }

private:
    class CEffectFindEdge : public FCImageEffect
    {
        void AddPoint (int x, int y)
        {
            POINT   pt = {x,y} ;
            m_edge.push_back(pt) ;
        }

    public:
        std::vector<POINT>   m_edge ;

        virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
        {
            // edge
            if (PCL_A(pPixel))
            {
                if (y == 0)
                {
                    AddPoint (x, -1) ;
                }
                else if (y == img.Height()-1)
                {
                    AddPoint (x, img.Height()) ;
                }

                if (x == 0)
                {
                    AddPoint (-1, y) ;
                }
                else if (x == img.Width()-1)
                {
                    AddPoint (img.Width(), y) ;
                }
            }

            POINT   nDelta[4] = {{0,-1}, {0,1}, {-1,0}, {1,0}} ; // up-down-left-right
            for (int i=0 ; i < 4 ; i++)
            {
                POINT   pt = {x+nDelta[i].x, y+nDelta[i].y} ;

                int   nRoundAlpha ;
                if (img.IsInside(pt.x,pt.y))
                {
                    nRoundAlpha = PCL_A(img.GetBits(pt.x,pt.y)) ;
                }
                else
                {
                    nRoundAlpha = 0 ;
                }

                if (PCL_A(pPixel) < nRoundAlpha)
                {
                    AddPoint(x,y) ;
                    break;
                }
            }
        }
    };

    void GetAlphaChannel (const FCObjImage& img, FCObjImage& imgAlpha)
    {
        imgAlpha.Create (img.Width(), img.Height(), 8) ;

        for (int y=0 ; y < img.Height() ; y++)
        {
            for (int x=0 ; x < img.Width() ; x++)
            {
                *imgAlpha.GetBits(x,y) = PCL_A(img.GetBits(x,y)) ;
            }
        }
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

    void DrawHalo (FCObjImage& img, const FCObjImage& imgAlpha, POINT ptCenter)
    {
        RECT   rcOnImg = {ptCenter.x-m_radius, ptCenter.y-m_radius, 0, 0} ;
        rcOnImg.right = rcOnImg.left + m_halo.Width() ;
        rcOnImg.bottom = rcOnImg.top + m_halo.Height() ;

        RECT   rcImg = {0, 0, img.Width(), img.Height()} ;
        RECT   rc ;
        IntersectRect (&rc, &rcImg, &rcOnImg) ;
        if (IsRectEmpty(&rc))
            return ;

        int   nCenterAlpha = 0 ;
        if (img.IsInside(ptCenter.x,ptCenter.y))
        {
            nCenterAlpha = PCL_A(img.GetBits(ptCenter.x,ptCenter.y)) ;
        }

        for (int y=rc.top ; y < rc.bottom ; y++)
        {
            for (int x=rc.left ; x < rc.right ; x++)
            {
                BYTE   * pImg = img.GetBits(x,y) ;
                BYTE   * pAlpha = imgAlpha.GetBits(x,y) ;
                BYTE   * pHalo = m_halo.GetBits(x-rcOnImg.left, y-rcOnImg.top) ;
                if (*pAlpha > nCenterAlpha)
                {
                    int   new_a = nCenterAlpha + (*pAlpha - nCenterAlpha) * (0xFF - *pHalo) / 0xFF ;
                    if (new_a < PCL_A(pImg))
                        PCL_A(pImg) = (BYTE)new_a ;
                }
            }
        }
    }

    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress)
    {
        // get draw halo point
        CEffectFindEdge   edge_list ;
        img.ApplyEffect(edge_list) ;

        if (edge_list.m_edge.size())
        {
            // backup alpha channel
            FCObjImage   imgAlpha ;
            GetAlphaChannel (img, imgAlpha) ;

            for (size_t i=0 ; i < edge_list.m_edge.size() ; i++)
            {
                DrawHalo (img, imgAlpha, edge_list.m_edge[i]) ;
                if (pProgress)
                {
                    if (!pProgress->UpdateProgress ((int)(100*i/edge_list.m_edge.size())))
                        break;
                }
            }
        }

        if (pProgress)
            pProgress->UpdateProgress (100) ;
    }

private:
    class CEffectCreateHalo : public FCImageEffect
    {
        double   m_radius ;
    public:
        CEffectCreateHalo (int nRadius) : m_radius(nRadius) {}
    private:
        virtual bool IsSupport (const FCObjImage& img) {return true;}
        virtual void OnBeforeProcess (FCObjImage& img)
        {
            int   w = (int)(2*m_radius + 1) ;
            img.Create (w, w, 8) ;
        }
        virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel)
        {
            double   d1 = x - m_radius ;
            double   d2 = y - m_radius ;
            double   t = sqrt (d1*d1 + d2*d2) ;
            if (t < m_radius)
                *pPixel = (BYTE)(0xFF * (m_radius - t) / m_radius) ;
        }
    };
};
