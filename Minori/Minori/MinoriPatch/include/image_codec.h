
class FCObjImage ; // external class

//-------------------------------------------------------------------------------------
/**
    Interface to Read/Write image.
    all interface return false default, derived class should implement it.
*/
class FCImageCodec
{
public:
    virtual ~FCImageCodec() {}

    /// @name Load image.
    //@{
    /**
        Load image from file \n
        caller must <b>delete</b> all image in rImageList.
    */
    virtual bool LoadImageFile (const wchar_t* szFileName,
                                std::vector<FCObjImage*>& rImageList,
                                FCImageProperty& rImageProp)
    {
        assert(false) ;
        return false ;
    }

    /**
        Load image from memory \n
        caller must <b>delete</b> all image in rImageList.
    */
    virtual bool LoadImageMemory (const void* pStart, int nMemSize,
                                  std::vector<FCObjImage*>& rImageList,
                                  FCImageProperty& rImageProp)
    {
        assert(false) ;
        return false ;
    }
    //@}

    /// @name Save image.
    //@{
    /// Save image to file.
    virtual bool SaveImageFile (const wchar_t* szFileName,
                                const std::vector<const FCObjImage*>& rImageList,
                                const FCImageProperty& rImageProp)
    {
        assert(false) ;
        return false ;
    }
    //@}
};
