typedef struct psbinfo_s
{
    PDWORD psbfunctable;  //0
    DWORD unknow1; //4
    DWORD unknow2;  //8
    PBYTE pPSBData;  //C
    DWORD psbSize;  //10
    DWORD unknow3;  //14
    DWORD nType;  //18
    DWORD nPSBhead;  //1C
    DWORD nNameTree;  //20
    DWORD nStrOffList;  //24
    DWORD nStrRes;  //28
    DWORD nDibOffList;  //2C
    DWORD nDibSizeList;  //30
    DWORD nDibRes;  //34
    DWORD nResIndexTree;  //38
    DWORD unknow13;  //3C
}psbinfo_t;