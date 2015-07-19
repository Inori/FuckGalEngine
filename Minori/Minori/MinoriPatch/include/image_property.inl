
//-------------------------------------------------------------------------------------
/**
    Property of image.
*/
class FCImageProperty
{
public:
    /**
        JPG - compress quality 1 <= n <= 100, default is -1, means quality 90 \n
        GIF - transparent color's index in palette.
    */
    int     m_SaveFlag ;

    /// GIF frame delay, in millisecond.
    std::vector<int>   m_frame_delay ;

    /// manufacturer of the equipment.
    std::string   m_EquipMake ;
    /// model name or model number of the equipment.
    std::string   m_EquipModel ;
    /// Date and time when the original image data was generated, The format is YYYY:MM:DD HH:MM:SS.
    std::string   m_ExifDTOrig ;

    /**
        use FCImageCodec_Gdiplus::ReferencePropertyBuffer to parse its item \n
        you can search <span style='color:#FF0000'>Property Item Descriptions GDI+</span> in google for detail.
    */
    std::vector<std::string>   m_other_gdiplus_prop ;

public:
    FCImageProperty()
    {
        m_SaveFlag = -1 ;
    }

    virtual ~FCImageProperty() {}
};
