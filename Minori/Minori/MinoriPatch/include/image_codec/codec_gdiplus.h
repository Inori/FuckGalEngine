/*
    Copyright (C) =USTC= Fu Li

    Author   :  Fu Li
    Create   :  2004-6-18
    Home     :  http://www.phoxo.com
    Mail     :  crazybitwps@hotmail.com

    This file is part of ImageStone

    The code distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Redistribution and use the source code, with or without modification,
    must retain the above copyright.
*/
#ifndef IMAGE_CODEC_GDIPLUS_2004_06_18_H
#define IMAGE_CODEC_GDIPLUS_2004_06_18_H

//-------------------------------------------------------------------------------------
/// Auto load / unload GDI+ module (<B>Win only</B>).
class FCAutoInitGDIPlus
{
    ULONG_PTR   m_token ;
public:
    /// Constructor \n call API <b>GdiplusStartup</b> to load GDI+ module
    FCAutoInitGDIPlus()
    {
        Gdiplus::GdiplusStartupInput   si ;
        Gdiplus::GdiplusStartup (&m_token, &si, NULL) ;
    }
    /// Destructor \n call API <b>GdiplusShutdown</b> to unload GDI+ module
    virtual ~FCAutoInitGDIPlus()
    {
        Gdiplus::GdiplusShutdown (m_token) ;
    }
};

//-------------------------------------------------------------------------------------
/**
    Read/Write image use GDI+ (<B>Win only</B>).
    If you load a gif image with transparent color, you will get a 32bpp image, the transparent color has been converted to alpha=0 pixel
*/
class FCImageCodec_Gdiplus : public FCImageCodec
{
    std::auto_ptr<FCAutoInitGDIPlus>   m_auto_init_gdiplus ;
public:
    /// Is auto init GDI+ module in constructor, default is TRUE.
    static BOOL& AUTO_INIT_GDIPLUS()
    {
        static BOOL   s = TRUE ;
        return s ;
    }

    FCImageCodec_Gdiplus()
    {
        if (AUTO_INIT_GDIPLUS())
        {
            m_auto_init_gdiplus.reset(new FCAutoInitGDIPlus()) ;
        }
    }

    /// Caller must <b>delete</b> returned bitmap.
    static Gdiplus::Bitmap* CreateBitmapFromMemory (const void* pMem, SIZE_T nSize)
    {
        Gdiplus::Bitmap   * pBmp = NULL ;

        IStream   * pStream = CreateStreamFromMemory(pMem, nSize) ;
        if (pStream)
        {
            pBmp = new Gdiplus::Bitmap(pStream) ;
            pStream->Release() ;
        }
        return pBmp ;
    }

    /// Create bitmap from file and not lock file, caller must <b>delete</b> returned bitmap.
    static Gdiplus::Bitmap* LoadUnlockFileBitmap (LPCTSTR image_file)
    {
        Gdiplus::Bitmap   * pBmp = NULL ;

        HANDLE   f = ::CreateFile (image_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) ;
        if (f != INVALID_HANDLE_VALUE)
        {
            DWORD   nLength = ::GetFileSize(f, NULL) ;
            if (nLength)
            {
                std::vector<BYTE>   buf (nLength) ;

                DWORD   dwRead ;
                ::ReadFile (f, &buf[0], nLength, &dwRead, NULL) ;
                pBmp = CreateBitmapFromMemory (&buf[0], nLength) ;
            }
            ::CloseHandle (f) ;
        }
        return pBmp ;
    }

    /// @name Gdiplus::PropertyItem <--> property buffer.
    //@{
    /// Create buffer from PropertyItem.
    static std::string CreatePropertyBuffer (PROPID nID, WORD nType, ULONG nLength, const VOID* pValue)
    {
        std::string   s ;
        if (nLength && pValue)
        {
            s.append ((const char*)&nID, sizeof(nID)) ;
            s.append ((const char*)&nType, sizeof(nType)) ;
            s.append ((const char*)&nLength, sizeof(nLength)) ;
            s.append ((const char*)pValue, nLength) ;
        }
        return s ;
    }

    /// Reference to a property buffer.
    static Gdiplus::PropertyItem ReferencePropertyBuffer (const std::string& buf)
    {
        const char   * pCurr = buf.c_str() ;

        Gdiplus::PropertyItem   it = {0} ;
        it.id = *(PROPID*)pCurr ; pCurr+=sizeof(PROPID) ;
        it.type = *(WORD*)pCurr ; pCurr+=sizeof(WORD) ;
        it.length = *(ULONG*)pCurr ; pCurr+=sizeof(ULONG) ;
        it.value = const_cast<char*>(pCurr) ;
        return it ;
    }
    //@}

    /// get image property from Bitmap.
    static void GetPropertyFromBitmap (Gdiplus::Bitmap& gpBmp, FCImageProperty& rImageProp)
    {
        // get all property
        UINT   nBytes=0, nNum=0 ;
        gpBmp.GetPropertySize (&nBytes, &nNum) ;
        if (!nBytes || !nNum)
            return ;

        std::vector<BYTE>   temp_buf (nBytes) ;

        Gdiplus::PropertyItem   * gpItem = (Gdiplus::PropertyItem*)&temp_buf[0] ;
        gpBmp.GetAllPropertyItems (nBytes, nNum, gpItem) ;

        for (UINT i=0 ; i < nNum ; i++)
        {
            Gdiplus::PropertyItem   & it = gpItem[i] ;
            if (!it.length || !it.value)
                continue ;

            // get GIF frame delay
            if (it.id == PropertyTagFrameDelay)
            {
                for (ULONG j=0 ; j < it.length/4 ; j++)
                {
                    DWORD   nSpan = 10 * ((DWORD*)it.value)[j] ;
                    if (!nSpan)
                    {
                        nSpan = 100 ;
                    }
                    rImageProp.m_frame_delay.push_back (nSpan) ;
                }
                continue ;
            }

            if (it.id == PropertyTagEquipMake)
            {
                rImageProp.m_EquipMake.assign ((char*)it.value, it.length) ;
                continue ;
            }

            if (it.id == PropertyTagEquipModel)
            {
                rImageProp.m_EquipModel.assign ((char*)it.value, it.length) ;
                continue ;
            }

            if (it.id == PropertyTagExifDTOrig)
            {
                rImageProp.m_ExifDTOrig.assign ((char*)it.value, it.length) ;
                continue ;
            }

            // other GDI+ property
            std::string   ts = CreatePropertyBuffer (it.id, it.type, it.length, it.value) ;
            if (ts.size())
            {
                rImageProp.m_other_gdiplus_prop.push_back(ts) ;
            }
        }
    }

    /// Add property to bitmap object.
    static void AddPropertyToBitmap (const FCImageProperty& rImageProp, Gdiplus::Bitmap& gpBmp)
    {
        std::vector<std::string>   ls ;
        std::string   s ;

        s = rImageProp.m_EquipMake ;
        if (s.size())
        {
            size_t   nl = strlen(s.c_str()) + 1 ; // maybe include multiple 0
            ls.push_back (CreatePropertyBuffer (PropertyTagEquipMake, PropertyTagTypeASCII, (ULONG)nl, s.c_str())) ;
        }

        s = rImageProp.m_EquipModel ;
        if (s.size())
        {
            size_t   nl = strlen(s.c_str()) + 1 ;
            ls.push_back (CreatePropertyBuffer (PropertyTagEquipModel, PropertyTagTypeASCII, (ULONG)nl, s.c_str())) ;
        }

        s = rImageProp.m_ExifDTOrig ;
        if (s.size())
        {
            size_t   nl = strlen(s.c_str()) + 1 ;
            ls.push_back (CreatePropertyBuffer (PropertyTagExifDTOrig, PropertyTagTypeASCII, (ULONG)nl, s.c_str())) ;
        }

        // add standard property
        for (size_t i=0 ; i < ls.size() ; i++)
        {
            Gdiplus::PropertyItem   tmProp = ReferencePropertyBuffer(ls[i]) ;
            gpBmp.SetPropertyItem (&tmProp) ;
        }

        // add other GDI+ property
        for (size_t i=0 ; i < rImageProp.m_other_gdiplus_prop.size() ; i++)
        {
            Gdiplus::PropertyItem   tmProp = ReferencePropertyBuffer(rImageProp.m_other_gdiplus_prop[i]) ;
            gpBmp.SetPropertyItem (&tmProp) ;
        }
    }

private:
    // Load image file use GDI+.
    virtual bool LoadImageFile (const wchar_t* szFileName,
                                std::vector<FCObjImage*>& rImageList,
                                FCImageProperty& rImageProp)
    {
        Gdiplus::Bitmap   gpBmp (szFileName) ;
        return StoreMultiFrame (gpBmp, rImageList, rImageProp) ;
    }

    // Load image memory use GDI+.
    virtual bool LoadImageMemory (const void* pStart, int nMemSize,
                                  std::vector<FCObjImage*>& rImageList,
                                  FCImageProperty& rImageProp)
    {
        std::auto_ptr<Gdiplus::Bitmap>   gpBmp (CreateBitmapFromMemory(pStart, nMemSize)) ;
        return (gpBmp.get() ? StoreMultiFrame (*gpBmp, rImageList, rImageProp) : false) ;
    }

    // Get all frames in gpBmp, add into rImageList.
    static bool StoreMultiFrame (Gdiplus::Bitmap& gpBmp,
                                 std::vector<FCObjImage*>& rImageList,
                                 FCImageProperty& rImageProp)
    {
        if (gpBmp.GetLastStatus() != Gdiplus::Ok)
        {
            assert(false); return false;
        }

        // get frame dimensions
        const UINT   nDim = gpBmp.GetFrameDimensionsCount() ;

        std::vector<GUID>   listID (nDim + 1) ;
        gpBmp.GetFrameDimensionsList (&listID[0], nDim) ;

        // get frame and store
        for (UINT i=0 ; i < nDim ; i++)
        {
            UINT   nFrame = gpBmp.GetFrameCount (&listID[i]) ;
            for (UINT j=0 ; j < nFrame ; j++)
            {
                gpBmp.SelectActiveFrame (&listID[i], j) ;

                FCObjImage   * pImg = new FCObjImage ;
                pImg->FromBitmap (gpBmp) ;
                if (pImg->IsValidImage())
                {
                    rImageList.push_back(pImg) ;
                }
                else
                {
                    delete pImg ; assert(false);
                }
            }
        }

        // get image property
        GetPropertyFromBitmap (gpBmp, rImageProp) ;
        return (rImageList.size() != 0) ;
    }

    // GDI+ save image.
    virtual bool SaveImageFile (const wchar_t* szFileName,
                                const std::vector<const FCObjImage*>& rImageList,
                                const FCImageProperty& rImageProp)
    {
        if (rImageList.empty() || !rImageList[0]->IsValidImage())
            return false ;
        const FCObjImage   & img = *rImageList[0] ;

        // get encoder's CLSID
        CLSID       clsID ;
        IMAGE_TYPE  imgType = FCImageCodecFactory::GetTypeByFileExt(szFileName) ;
        if (!GetImageEncoderClsid (imgType, clsID))
            return false ;

        // if image is jpeg format, set save quality
        std::auto_ptr<Gdiplus::EncoderParameters>   pEnParas ;
        ULONG     nJpegQuality = ((rImageProp.m_SaveFlag == -1) ? 90 : rImageProp.m_SaveFlag) ;
        if (imgType == IMG_JPG)
        {
            pEnParas.reset (new Gdiplus::EncoderParameters) ;
            pEnParas->Count = 1 ;
            pEnParas->Parameter[0].Guid = Gdiplus::EncoderQuality ;
            pEnParas->Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong ;
            pEnParas->Parameter[0].NumberOfValues = 1 ;
            pEnParas->Parameter[0].Value = &nJpegQuality ;
        }

        // gif transparency
        int   nTransparencyIndex = -1 ;
        if (imgType == IMG_GIF)
        {
            nTransparencyIndex = rImageProp.m_SaveFlag ;
        }

        // create a GDI+ bitmap and put property
        std::auto_ptr<Gdiplus::Bitmap>   pBmp (img.CreateBitmap(nTransparencyIndex)) ;
        if (!pBmp.get())
            return false ;

        AddPropertyToBitmap (rImageProp, *pBmp) ;
        return (pBmp->Save (szFileName, &clsID, pEnParas.get()) == Gdiplus::Ok) ;
    }

private:
    // caller must call Release on returned object.
    static IStream* CreateStreamFromMemory (const void* pMem, SIZE_T nSize)
    {
        IStream   * pStream = NULL ;
        if (pMem && nSize)
        {
            HGLOBAL   hBuffer = GlobalAlloc (GMEM_MOVEABLE, nSize) ;
            CopyMemory (GlobalLock(hBuffer), pMem, nSize) ;
            GlobalUnlock (hBuffer) ;

            CreateStreamOnHGlobal (hBuffer, TRUE, &pStream) ;

            if (!pStream)
            {
                GlobalFree(hBuffer) ;
            }
        }
        return pStream ;
    }

    // Get bmp/jpeg/gif/tiff/png image's CLSID.
    static BOOL GetImageEncoderClsid (IMAGE_TYPE imgType, CLSID& rClsid)
    {
        GUID   idType ;
        switch (imgType)
        {
            case IMG_BMP : idType = Gdiplus::ImageFormatBMP  ; break;
            case IMG_JPG : idType = Gdiplus::ImageFormatJPEG ; break;
            case IMG_GIF : idType = Gdiplus::ImageFormatGIF  ; break;
            case IMG_TIF : idType = Gdiplus::ImageFormatTIFF ; break;
            case IMG_PNG : idType = Gdiplus::ImageFormatPNG  ; break;
            default : return FALSE ;
        }

        UINT   nNum=0, nSize=0 ;
        Gdiplus::GetImageEncodersSize (&nNum, &nSize) ;
        if (!nSize)
            return FALSE ;

        std::vector<BYTE>   temp_buf (nSize) ;

        Gdiplus::ImageCodecInfo   * pICI = (Gdiplus::ImageCodecInfo*)&temp_buf[0] ;
        Gdiplus::GetImageEncoders (nNum, nSize, pICI) ;

        for (UINT i=0 ; i < nNum ; i++)
        {
            if (IsEqualGUID (pICI[i].FormatID, idType))
            {
                rClsid = pICI[i].Clsid ;
                return TRUE ;
            }
        }
        return FALSE ;
    }
};

#endif
