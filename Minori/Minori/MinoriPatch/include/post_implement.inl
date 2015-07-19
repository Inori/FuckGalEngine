
//-------------------------------------------------------------------------------------
inline std::auto_ptr<FCImageCodecFactory>& FCObjImage::g_codec_factory()
{
#if (defined(_WIN32) || defined(__WIN32__))
    static std::auto_ptr<FCImageCodecFactory>   s_pFactory (new FCImageCodecFactory_Gdiplus) ;
#else
    static std::auto_ptr<FCImageCodecFactory>   s_pFactory (new FCImageCodecFactory_FreeImage) ; // linux/unix/mac...
#endif

    return s_pFactory ;
}
//-------------------------------------------------------------------------------------
inline bool FCImageEffect::IsSupport (const FCObjImage& img)
{
    return img.IsValidImage() && (img.ColorBits() >= 24) ;
}
//-------------------------------------------------------------------------------------
