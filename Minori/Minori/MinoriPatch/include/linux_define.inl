#include <algorithm>

//-------------------------------------------------------------------------------------
struct RECT
{
    long left;
    long top;
    long right;
    long bottom;
};

struct POINT
{
    long x;
    long y;
};

struct SIZE
{
    long cx;
    long cy;
};

//-------------------------------------------------------------------------------------
inline bool IsRectEmpty (const RECT* p)
{
    if (p)
    {
        return ((p->right <= p->left) || (p->bottom <= p->top)) ;
    }
    return true ;
}

inline void OffsetRect (RECT* p, int dx, int dy)
{
    if (p)
    {
        p->left+=dx ; p->top+=dy ; p->right+=dx ; p->bottom+=dy ;
    }
}

inline void InflateRect (RECT* p, int dx, int dy)
{
    if (p)
    {
        p->left-=dx ; p->top-=dy ; p->right+=dx ; p->bottom+=dy ;
    }
}

inline void UnionRect (RECT* pDst, const RECT* pSrc1, const RECT* pSrc2)
{
    const RECT   z = {0} ;

    if (pDst)
        *pDst = z ;

    if (pDst && pSrc1 && pSrc2)
    {
        pDst->left = std::min (pSrc1->left, pSrc2->left) ;
        pDst->top = std::min (pSrc1->top, pSrc2->top) ;
        pDst->right = std::max (pSrc1->right, pSrc2->right) ;
        pDst->bottom = std::max (pSrc1->bottom, pSrc2->bottom) ;

        if (IsRectEmpty(pDst))
        {
            *pDst = z ;
        }
    }
}

inline bool IntersectRect (RECT* pDst, const RECT* pSrc1, const RECT* pSrc2)
{
    const RECT   z = {0} ;

    if (pDst)
        *pDst = z ;

    if (pDst && pSrc1 && pSrc2)
    {
        pDst->left = std::max (pSrc1->left, pSrc2->left) ;
        pDst->top = std::max (pSrc1->top, pSrc2->top) ;
        pDst->right = std::min (pSrc1->right, pSrc2->right) ;
        pDst->bottom = std::min (pSrc1->bottom, pSrc2->bottom) ;

        if (IsRectEmpty(pDst))
        {
            *pDst = z ;
            return false ;
        }
        return true ;
    }
    return false ;
}
