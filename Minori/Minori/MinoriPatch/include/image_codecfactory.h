
//-------------------------------------------------------------------------------------
/**
    Interface to create image codec.
*/
class FCImageCodecFactory
{
public:
    virtual ~FCImageCodecFactory() {}

    /**
        Get image's format by file's <b>ext name</b> default \n
        you can override this function to determine format from file's header.
    */
    virtual IMAGE_TYPE QueryImageFileType (const wchar_t* szFileName)
    {
        return GetTypeByFileExt(szFileName) ;
    }

    /// Get image's format by file's ext name.
    static IMAGE_TYPE GetTypeByFileExt (const wchar_t* szFileName)
    {
        if (!szFileName)
            return IMG_UNKNOW ;

        // get ext name of image file
        std::wstring   strExt ;
        {
            std::wstring   s (szFileName) ;
            size_t         nPos = s.find_last_of (L".") ;
            if (nPos != std::wstring::npos)
                strExt = s.substr (nPos + 1) ;
        }

        // convert to lowercase
        for (size_t i=0 ; i < strExt.length() ; i++)
        {
            if ((strExt[i] >= 'A') && (strExt[i] <= 'Z'))
                strExt[i] = (wchar_t)tolower(strExt[i]) ;
        }

        if (strExt == L"jpg")  return IMG_JPG ;
        if (strExt == L"jpeg") return IMG_JPG ;
        if (strExt == L"jpe")  return IMG_JPG ;
        if (strExt == L"jfif") return IMG_JPG ;
        if (strExt == L"gif")  return IMG_GIF ;
        if (strExt == L"bmp")  return IMG_BMP ;
        if (strExt == L"png")  return IMG_PNG ;
        if (strExt == L"tga")  return IMG_TGA ;
        if (strExt == L"tif")  return IMG_TIF ;
        if (strExt == L"tiff") return IMG_TIF ;
        if (strExt == L"ico")  return IMG_ICO ;
        if (strExt == L"psd")  return IMG_PSD ;
        if (strExt == L"pcx")  return IMG_PCX ;
        if (strExt == L"xpm")  return IMG_XPM ;
        if (strExt == L"oxo")  return IMG_PHOXO ;

        return IMG_UNKNOW ;
    }

    /**
        Create image codec by image type \n
        derived class must use <B>new</B> to create codec and return it \n
        user must use <B>delete</B> to delete returned codec.
    */
    virtual FCImageCodec* CreateImageCodec (IMAGE_TYPE imgType) =0 ;
};
