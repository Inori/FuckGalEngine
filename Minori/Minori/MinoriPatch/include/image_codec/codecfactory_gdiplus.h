/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2005-7-29
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#if (defined(_WIN32) || defined(__WIN32__))
#ifndef CODECFACTORY_GDIPLUS_2005_07_29_H
#define CODECFACTORY_GDIPLUS_2005_07_29_H
#include "codec_gdiplus.h"

//-------------------------------------------------------------------------------------
/**
    Create GDI+ codec (<B>Win only</B>).
@verbatim
              BMP      Jpg      Gif      Tif      Png      Ico
    Read       O        O        O        O        O        O
    Write      O        O        O        O        O        X
@endverbatim
*/
class FCImageCodecFactory_Gdiplus : public FCImageCodecFactory
{
    virtual FCImageCodec* CreateImageCodec (IMAGE_TYPE imgType)
    {
        switch (imgType)
        {
            case IMG_BMP :
            case IMG_JPG :
            case IMG_GIF :
            case IMG_TIF :
            case IMG_PNG :
            case IMG_ICO : return new FCImageCodec_Gdiplus ;
        }
        return 0 ;
    }
};

#endif
#endif
