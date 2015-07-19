
class FCObjImage ; // external class

//-------------------------------------------------------------------------------------
/// Receive progress of operation.
class FCProgressObserver
{
    int   m_canceled ;
public:
    FCProgressObserver() : m_canceled(0) {}
    virtual ~FCProgressObserver() {}

    bool IsCanceled() const {return (m_canceled != 0);}

    bool UpdateProgress (int nFinishPercentage)
    {
        if (!OnProgressUpdate(nFinishPercentage))
        {
            m_canceled = 1 ;
        }
        return !IsCanceled() ;
    }

protected:
    /**
        Override to handle progress \n
        0 <= nFinishPercentage <= 100 \n
        return false if you want to stop operation, return true to continue process.
    */
    virtual bool OnProgressUpdate (int nFinishPercentage) =0 ;
};

//-------------------------------------------------------------------------------------
/**
    Image effect processor interface.
*/
class FCImageEffect
{
public:
    virtual ~FCImageEffect() {}

    /// How to process the image.
    enum PROCESS_TYPE
    {
        PROCESS_TYPE_WHOLE, ///< process whole image
        PROCESS_TYPE_PIXEL, ///< process every pixel
    };

    /**
        Whether the image can be disposed by this processor \n
        default to test image's bpp >= 24
    */
    virtual bool IsSupport (const FCObjImage& img) ;

    /// Query process type, default to return \ref PROCESS_TYPE_PIXEL.
    virtual PROCESS_TYPE QueryProcessType() {return PROCESS_TYPE_PIXEL;}

    /// Event before process, default do nothing.
    virtual void OnBeforeProcess (FCObjImage& img) {}

    /// Process (x,y) pixel when \ref QueryProcessType return PROCESS_TYPE_PIXEL.
    virtual void ProcessPixel (FCObjImage& img, int x, int y, BYTE* pPixel) {}
    /// Process whole image when \ref QueryProcessType return PROCESS_TYPE_WHOLE.
    virtual void ProcessWholeImage (FCObjImage& img, FCProgressObserver* pProgress) {}
};
