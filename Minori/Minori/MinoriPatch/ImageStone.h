/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  1997-9-1
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef IMAGESTONE_HEADER_9711_H
#define IMAGESTONE_HEADER_9711_H

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4100) // disable MSVC warning: warning C4100: 'p' : unreferenced formal parameter
#endif

//-------------------------------------------------------------------------------------
#include "include/image.h"

#include "include/image_codec/codecfactory_gdiplus.h"

#ifdef IMAGESTONE_USE_FREEIMAGE
    #include "include/image_codec/codecfactory_freeimage.h"
#endif

#include "include/effect/effect.h"

#include "include/post_implement.inl"
//-------------------------------------------------------------------------------------

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#endif
