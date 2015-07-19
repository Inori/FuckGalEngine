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
#ifndef CODECFACTORY_FREEIMAGE_2005_07_29_H
#define CODECFACTORY_FREEIMAGE_2005_07_29_H
#include "codec_freeimage.h"

//-------------------------------------------------------------------------------------
/**
    Create FreeImage codec (<B>Need FreeImage</B>).
@verbatim
              BMP      TGA      Jpg      Gif      Tif      Png      Pcx      Ico      Xpm      Psd
    Read       O        O        O        O        O        O        O        O        O        O
    Write      O        O        O        O        O        O        O        O        O        X
@endverbatim
*/
class FCImageCodecFactory_FreeImage : public FCImageCodecFactory
{
    virtual FCImageCodec* CreateImageCodec (IMAGE_TYPE imgType)
    {
        switch (imgType)
        {
            case IMG_BMP :
            case IMG_TGA :
            case IMG_GIF :
            case IMG_PCX :
            case IMG_PNG :
            case IMG_TIF :
            case IMG_JPG :
            case IMG_ICO :
            case IMG_XPM :
            case IMG_PSD : return new FCImageCodec_FreeImage ;
        }
        return 0 ;
    }
};

#endif
