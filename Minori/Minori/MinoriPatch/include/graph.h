/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2003-3-30
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef OBJ_GRAPH_2003_03_30_H
#define OBJ_GRAPH_2003_03_30_H
#include "color.h"

//-------------------------------------------------------------------------------------
/**
    Graphic object.
    provide coordinates operation.
*/
class FCObjGraph
{
private:
    POINT   m_pos ; // position on canvas

public:
    FCObjGraph() {m_pos.x = m_pos.y = 0;}
    virtual ~FCObjGraph() {}

    FCObjGraph& operator= (const FCObjGraph& obj)
    {
        m_pos = obj.m_pos ;
        return *this ;
    }

    /**
        Fit show for window \n
        if size of object exceed window, scale in window \n
        if size of object smaller than window, center in window.
    */
    static RECT CalcFitWindowSize (SIZE szObject, RECT rcWindow)
    {
        int   ww = rcWindow.right - rcWindow.left ;
        int   wh = rcWindow.bottom - rcWindow.top ;

        RECT   rc = {0,0,0,0} ;
        if (szObject.cx && szObject.cy && (ww > 0) && (wh > 0))
        {
            if ((szObject.cx > ww) || (szObject.cy > wh))
            {
                double   dx = ww / (double)szObject.cx ;
                double   dy = wh / (double)szObject.cy ;
                double   d = ((dx < dy) ? dx : dy) ;
                szObject.cx = (int)(szObject.cx * d) ;
                szObject.cy = (int)(szObject.cy * d) ;
                szObject.cx = ((szObject.cx > 1) ? szObject.cx : 1) ;
                szObject.cy = ((szObject.cy > 1) ? szObject.cy : 1) ;
            }

            rc.left = rcWindow.left + (ww - szObject.cx) / 2 ;
            rc.top = rcWindow.top + (wh - szObject.cy) / 2 ;
            rc.right = rc.left + szObject.cx ;
            rc.bottom = rc.top + szObject.cy ;
        }
        return rc ;
    }

    /// @name Coordinate.
    //@{
    /// Set position.
    void SetGraphObjPos (int x, int y) {m_pos.x=x; m_pos.y=y;}
    /// Set position.
    void SetGraphObjPos (POINT pt) {m_pos = pt;}
    /// Get position.
    POINT GetGraphObjPos() const {return m_pos;}
    //@}

    /// @name Coordinate transform
    //@{
    /// canvas --> layer
    void Canvas_to_Layer (POINT& pt) const
    {
        pt.x -= m_pos.x ;
        pt.y -= m_pos.y ;
    }
    /// canvas --> layer
    void Canvas_to_Layer (RECT& rc) const
    {
        rc.left -= m_pos.x ;
        rc.top -= m_pos.y ;
        rc.right -= m_pos.x ;
        rc.bottom -= m_pos.y ;
    }

    /// layer --> canvas
    void Layer_to_Canvas (POINT& pt) const
    {
        pt.x += m_pos.x ;
        pt.y += m_pos.y ;
    }
    /// layer --> canvas
    void Layer_to_Canvas (RECT& rc) const
    {
        rc.left += m_pos.x ;
        rc.top += m_pos.y ;
        rc.right += m_pos.x ;
        rc.bottom += m_pos.y ;
    }
    //@}
};

#endif
