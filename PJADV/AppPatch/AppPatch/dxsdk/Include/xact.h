/*++

Copyright (C) Microsoft Corporation.  All rights reserved.

Module Name:

    xact.h

Abstract:

    XACT public interfaces, functions and data types

--*/

#pragma once

#ifndef _XACT_H_
#define _XACT_H_

//------------------------------------------------------------------------------
// XACT class and interface IDs (Version 2.4)
//------------------------------------------------------------------------------
#ifndef _XBOX // XACT COM support only exists on Windows
    #include <comdecl.h> // For DEFINE_CLSID, DEFINE_IID and DECLARE_INTERFACE
    DEFINE_CLSID(XACTEngine,         bc3e0fc6, 2e0d, 4c45, bc, 61, d9, c3, 28, 31, 9b, d8);
    DEFINE_CLSID(XACTAuditionEngine, 30bad9f7, 0018, 49e9, bf, 94, 4a, e8, 9c, c5, 4d, 64);
    DEFINE_CLSID(XACTDebugEngine,    74ee14d5, ca1d, 44ac, 8b, d3, fa, 94, f7, 34, 6e, 24);
    DEFINE_IID(IXACTEngine,          43a0d4a8, 9387, 4e06, 94, 33, 65, 41, 8f, e7, 0a, 67);
#endif

// Ignore the rest of this header if only the GUID definitions were requested:
#ifndef GUID_DEFS_ONLY

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------

#ifndef _XBOX
    #include <windows.h>
    #include <objbase.h>
    #include <float.h>
#else
    #include <xaudio.h>
#endif

//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------

typedef struct IXACTSoundBank       IXACTSoundBank;
typedef struct IXACTWaveBank        IXACTWaveBank;
typedef struct IXACTCue             IXACTCue;
typedef struct IXACTEngine          IXACTEngine;
typedef struct XACT_NOTIFICATION    XACT_NOTIFICATION;


//------------------------------------------------------------------------------
// Typedefs
//------------------------------------------------------------------------------

typedef WORD  XACTINDEX;            // All normal indices
typedef BYTE  XACTNOTIFICATIONTYPE; // Notification type
typedef FLOAT XACTVARIABLEVALUE;    // Variable value
typedef WORD  XACTVARIABLEINDEX;    // Variable index
typedef WORD  XACTCATEGORY;         // Sound category
typedef BYTE  XACTCHANNEL;          // Audio channel
typedef FLOAT XACTVOLUME;           // Volume value
typedef LONG  XACTTIME;             // Time (in ms)

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

static const XACTINDEX XACTINDEX_MIN     = 0x0;
static const XACTINDEX XACTINDEX_MAX     = 0xfffe;
static const XACTINDEX XACTINDEX_INVALID = 0xffff;

static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MIN  = 0x00;
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MAX  = 0xff;

static const XACTVARIABLEVALUE XACTVARIABLEVALUE_MIN = -FLT_MAX;
static const XACTVARIABLEVALUE XACTVARIABLEVALUE_MAX = FLT_MAX;

static const XACTVARIABLEINDEX XACTVARIABLEINDEX_MIN               = 0x0000;
static const XACTVARIABLEINDEX XACTVARIABLEINDEX_MAX               = 0xfffe;
static const XACTVARIABLEINDEX XACTVARIABLEINDEX_INVALID           = 0xffff;

static const XACTCATEGORY XACTCATEGORY_MIN     = 0x0;
static const XACTCATEGORY XACTCATEGORY_MAX     = 0xfffe;
static const XACTCATEGORY XACTCATEGORY_INVALID = 0xffff;

static const XACTCHANNEL XACTCHANNEL_MIN = 0;
static const XACTCHANNEL XACTCHANNEL_MAX = 0xFF;

static const XACTVOLUME XACTVOLUME_MIN = 0.0f;
static const XACTVOLUME XACTVOLUME_MAX = FLT_MAX;

static const XACTVARIABLEVALUE XACTPARAMETERVALUE_MIN = -FLT_MAX;
static const XACTVARIABLEVALUE XACTPARAMETERVALUE_MAX = FLT_MAX;

#ifdef _XBOX
static const XAUDIOVOICEINDEX XACTMAXOUTPUTVOICECOUNT = 3;
#endif // _XBOX

#define XACT_CONTENT_VERSION    41

//------------------------------------------------------------------------------
// XACT Parameters
//------------------------------------------------------------------------------

static const DWORD XACT_FLAG_GLOBAL_SETTINGS_MANAGEDATA = 0x00000001;

typedef BOOL (__stdcall * XACT_READFILE_CALLBACK)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
typedef BOOL (__stdcall * XACT_GETOVERLAPPEDRESULT_CALLBACK)(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);

typedef struct XACT_FILEIO_CALLBACKS
{
    XACT_READFILE_CALLBACK            readFileCallback;
    XACT_GETOVERLAPPEDRESULT_CALLBACK getOverlappedResultCallback;
} XACT_FILEIO_CALLBACKS, *PXACT_FILEIO_CALLBACKS;
typedef const XACT_FILEIO_CALLBACKS *PCXACT_FILEIO_CALLBACKS;

// The callback that receives the notifications.
typedef void (__stdcall * XACT_NOTIFICATION_CALLBACK)(const XACT_NOTIFICATION* pNotification);

#ifndef _XBOX

#define XACT_RENDERER_ID_LENGTH    0xff   // Maximum number of characters allowed in the renderer ID
#define XACT_RENDERER_NAME_LENGTH  0xff   // Maximum number of characters allowed in the renderer display name.

// Renderer details
typedef struct XACT_RENDERER_DETAILS
{
    WCHAR rendererID[XACT_RENDERER_ID_LENGTH];      // The string ID for the rendering device.
    WCHAR displayName[XACT_RENDERER_NAME_LENGTH];   // A friendly name suitable for display to a human.
    BOOL  defaultDevice;                            // Set to TRUE if this device is the primary audio device on the system.
} XACT_RENDERER_DETAILS, *LPXACT_RENDERER_DETAILS;
#endif

#define XACT_ENGINE_LOOKAHEAD_DEFAULT   250         // Default look-ahead time of 250ms can be used during XACT engine initialization.

// Runtime (engine) parameters
typedef struct XACT_RUNTIME_PARAMETERS
{
    DWORD                           lookAheadTime;                  // Time in ms
    void*                           pGlobalSettingsBuffer;          // Buffer containing the global settings file
    DWORD                           globalSettingsBufferSize;       // Size of global settings buffer
    DWORD                           globalSettingsFlags;            // Flags for global settings
    DWORD                           globalSettingsAllocAttributes;  // Global settings buffer allocation attributes (see XMemAlloc)
    XACT_FILEIO_CALLBACKS           fileIOCallbacks;                // File I/O callbacks
    XACT_NOTIFICATION_CALLBACK      fnNotificationCallback;         // Callback that receives notifications.
#ifndef _XBOX
    PWSTR                           pRendererID;                    // Ptr to the ID for the audio renderer the engine should connect to.
#endif
} XACT_RUNTIME_PARAMETERS, *LPXACT_RUNTIME_PARAMETERS;
typedef const XACT_RUNTIME_PARAMETERS *LPCXACT_RUNTIME_PARAMETERS;

//------------------------------------------------------------------------------
// Streaming Parameters
//------------------------------------------------------------------------------

typedef struct XACT_WAVEBANK_STREAMING_PARAMETERS
{
    HANDLE  file;            // File handle associated with wavebank data
    DWORD   offset;          // Offset within file of wavebank header (must be sector aligned)
    DWORD   flags;           // Flags (none currently)
    WORD    packetSize;      // Stream packet size (in sectors) to use for each stream (min = 2)
                             //   number of sectors (DVD = 2048 bytes: 2 = 4096, 3 = 6144, 4 = 8192 etc.)
                             //   optimal DVD size is a multiple of 16 (DVD block = 16 DVD sectors)
} XACT_WAVEBANK_STREAMING_PARAMETERS, *LPXACT_WAVEBANK_STREAMING_PARAMETERS;
typedef const XACT_WAVEBANK_STREAMING_PARAMETERS *LPCXACT_WAVEBANK_STREAMING_PARAMETERS;

//------------------------------------------------------------------------------
// Cue Properties (Xbox Only)
//------------------------------------------------------------------------------
#ifdef _XBOX
typedef struct XACTCUEPROPERTIES
{
    XACTCATEGORY ActiveSoundCategory;    // Category of the active sound
    BYTE         ActiveSoundTrackCount;  // The number of tracks in the active sound
    BYTE         ActiveSoundPriority;    // The priority of the active sound
    XACTINDEX    FirstTrackWaveIndex;    // Index of the currently selected wave variation in the first track
    DWORD        FirstTrackWaveDuration; // Estimated duration (ms) of the selected wave in the first track (does not account for pitch changes)
    XACTCHANNEL  MaxTrackChannelCount;   // The highest number of channels in any track
    BYTE         MaxTrackLoopCount;      // The highest loop count of any track (255 is infinite)
} XACTCUEPROPERTIES, *LPXACTCUEPROPERTIES;
#endif // _XBOX

//------------------------------------------------------------------------------
// Channel Mapping / Speaker Panning
//------------------------------------------------------------------------------

typedef struct XACTCHANNELMAPENTRY
{
    XACTCHANNEL   InputChannel;
    XACTCHANNEL   OutputChannel;
    XACTVOLUME    Volume;
} XACTCHANNELMAPENTRY, *LPXACTCHANNELMAPENTRY;
typedef const XACTCHANNELMAPENTRY *LPCXACTCHANNELMAPENTRY;

typedef struct XACTCHANNELMAP
{
    XACTCHANNEL             EntryCount;
    XACTCHANNELMAPENTRY*    paEntries;
} XACTCHANNELMAP, *LPXACTCHANNELMAP;
typedef const XACTCHANNELMAP *LPCXACTCHANNELMAP;

typedef struct XACTCHANNELVOLUMEENTRY
{
    XACTCHANNEL   EntryIndex;
    XACTVOLUME    Volume;
} XACTCHANNELVOLUMEENTRY, *LPXACTCHANNELVOLUMEENTRY;
typedef const XACTCHANNELVOLUMEENTRY *LPCXACTCHANNELVOLUMEENTRY;

typedef struct XACTCHANNELVOLUME
{
    XACTCHANNEL             EntryCount;
    XACTCHANNELVOLUMEENTRY* paEntries;
} XACTCHANNELVOLUME, *LPXACTCHANNELVOLUME;
typedef const XACTCHANNELVOLUME *LPCXACTCHANNELVOLUME;

//------------------------------------------------------------------------------
// Notifications
//------------------------------------------------------------------------------

static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPREPARED                      = 1;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEPLAY                          = 2;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUESTOP                          = 3;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_CUEDESTROYED                     = 4;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_MARKER                           = 5;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED               = 6;  // None, SoundBank
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKDESTROYED                = 7;  // None, WaveBank
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED             = 8;  // None, SoundBank, SoundBank & cue index, cue instance
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED            = 9;  // None
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUICONNECTED                     = 10; // None
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_GUIDISCONNECTED                  = 11; // None
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEPLAY                         = 12; // None, SoundBank, SoundBank & cue index, cue instance, WaveBank
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVESTOP                         = 13; // None, SoundBank, SoundBank & cue index, cue instance, WaveBank
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKPREPARED                 = 14; // None, WaveBank
static const XACTNOTIFICATIONTYPE XACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT = 15; // None, WaveBank

static const BYTE XACT_FLAG_NOTIFICATION_PERSIST = 0x01;

// Pack the notification structures
#pragma pack(push, 1)

// Notification description used for registering, un-registering and flushing notifications
typedef struct XACT_NOTIFICATION_DESCRIPTION
{
    XACTNOTIFICATIONTYPE type;          // Notification type
    BYTE                 flags;         // Flags
    IXACTSoundBank*      pSoundBank;    // SoundBank instance
    IXACTWaveBank*       pWaveBank;     // WaveBank instance
    IXACTCue*            pCue;          // Cue instance
    XACTINDEX            cueIndex;      // Cue index
    PVOID                pvContext;     // User context (optional)

} XACT_NOTIFICATION_DESCRIPTION, *LPXACT_NOTIFICATION_DESCRIPTION;
typedef const XACT_NOTIFICATION_DESCRIPTION *LPCXACT_NOTIFICATION_DESCRIPTION;

// Notification structure for all XACTNOTIFICATIONTYPE_CUE* notifications
typedef struct XACT_NOTIFICATION_CUE
{
    XACTINDEX       cueIndex;   // Cue index
    IXACTSoundBank* pSoundBank; // SoundBank instance
    IXACTCue*       pCue;       // Cue instance

} XACT_NOTIFICATION_CUE, *LPXACT_NOTIFICATION_CUE;
typedef const XACT_NOTIFICATION_CUE *LPCXACT_NOTIFICATION_CUE;

// Notification structure for all XACTNOTIFICATIONTYPE_MARKER* notifications
typedef struct XACT_NOTIFICATION_MARKER
{
    XACTINDEX       cueIndex;   // Cue index
    IXACTSoundBank* pSoundBank; // SoundBank instance
    IXACTCue*       pCue;       // Cue instance
    DWORD           marker;     // Marker value

} XACT_NOTIFICATION_MARKER, *LPXACT_NOTIFICATION_MARKER;
typedef const XACT_NOTIFICATION_MARKER *LPCXACT_NOTIFICATION_MARKER;

// Notification structure for all XACTNOTIFICATIONTYPE_SOUNDBANK* notifications
typedef struct XACT_NOTIFICATION_SOUNDBANK
{
    IXACTSoundBank* pSoundBank; // SoundBank instance

} XACT_NOTIFICATION_SOUNDBANK, *LPXACT_NOTIFICATION_SOUNDBANK;
typedef const XACT_NOTIFICATION_SOUNDBANK *LPCXACT_NOTIFICATION_SOUNDBANK;

// Notification structure for all XACTNOTIFICATIONTYPE_WAVEBANK* notifications
typedef struct XACT_NOTIFICATION_WAVEBANK
{
    IXACTWaveBank*  pWaveBank;  // WaveBank instance

} XACT_NOTIFICATION_WAVEBANK, *LPXACT_NOTIFICATION_WAVEBANK;
typedef const XACT_NOTIFICATION_WAVEBANK *LPCXACT_NOTIFICATION_WAVEBANK;

// Notification structure for all XACTNOTIFICATIONTYPE_*VARIABLE* notifications
typedef struct XACT_NOTIFICATION_VARIABLE
{
    XACTINDEX           cueIndex;       // Cue index
    IXACTSoundBank*     pSoundBank;     // SoundBank instance
    IXACTCue*           pCue;           // Cue instance
    XACTVARIABLEINDEX   variableIndex;  // Variable index
    XACTVARIABLEVALUE   variableValue;  // Variable value
    BOOL                local;          // TRUE if a local variable

} XACT_NOTIFICATION_VARIABLE, *LPXACT_NOTIFICATION_VARIABLE;
typedef const XACT_NOTIFICATION_VARIABLE *LPCXACT_NOTIFICATION_VARIABLE;

// Notification structure for all XACTNOTIFICATIONTYPE_GUI* notifications
typedef struct XACT_NOTIFICATION_GUI
{
    DWORD   reserved; // Reserved
} XACT_NOTIFICATION_GUI, *LPXACT_NOTIFICATION_GUI;
typedef const XACT_NOTIFICATION_GUI *LPCXACT_NOTIFICATION_GUI;

// Notification structure for all XACTNOTIFICATIONTYPE_WAVE* notifications
typedef struct XACT_NOTIFICATION_WAVE
{
    IXACTWaveBank*  pWaveBank;  // WaveBank
    XACTINDEX       waveIndex;  // Wave index
    XACTINDEX       cueIndex;   // Cue index
    IXACTSoundBank* pSoundBank; // SoundBank instance
    IXACTCue*       pCue;       // Cue instance

} XACT_NOTIFICATION_WAVE, *LPXACT_NOTIFICATION_WAVE;
typedef const XACT_NOTIFICATION_WAVE *LPCXACT_NOTIFICATION_WAVE;

// General notification structure
typedef struct XACT_NOTIFICATION
{
    XACTNOTIFICATIONTYPE    type;        // Notification type
    LONG                    timeStamp;   // Timestamp of notification (milliseconds)
    PVOID                   pvContext;   // User context (optional)
    union
    {
        XACT_NOTIFICATION_CUE       cue;        // XACTNOTIFICATIONTYPE_CUE*
        XACT_NOTIFICATION_MARKER    marker;     // XACTNOTIFICATIONTYPE_MARKER*
        XACT_NOTIFICATION_SOUNDBANK soundBank;  // XACTNOTIFICATIONTYPE_SOUNDBANK*
        XACT_NOTIFICATION_WAVEBANK  waveBank;   // XACTNOTIFICATIONTYPE_WAVEBANK*
        XACT_NOTIFICATION_VARIABLE  variable;   // XACTNOTIFICATIONTYPE_VARIABLE*
        XACT_NOTIFICATION_GUI       gui;        // XACTNOTIFICATIONTYPE_GUI*
        XACT_NOTIFICATION_WAVE      wave;       // XACTNOTIFICATIONTYPE_WAVE*
    };

} XACT_NOTIFICATION, *LPXACT_NOTIFICATION;
typedef const XACT_NOTIFICATION *LPCXACT_NOTIFICATION;
#pragma pack(pop)

//------------------------------------------------------------------------------
// IXACTSoundBank
//------------------------------------------------------------------------------

static const DWORD XACT_FLAG_SOUNDBANK_STOP_IMMEDIATE = 0x00000001;

static const DWORD XACT_SOUNDBANKSTATE_INUSE = 0x00000001;  // Currently in-use

STDAPI IXACTSoundBank_Destroy(IXACTSoundBank* pSoundBank);
STDAPI_(XACTINDEX) IXACTSoundBank_GetCueIndex(IXACTSoundBank* pSoundBank, PCSTR szFriendlyName);
STDAPI IXACTSoundBank_Prepare(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue);
STDAPI IXACTSoundBank_Play(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue);
STDAPI IXACTSoundBank_Stop(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags);
STDAPI IXACTSoundBank_GetState(IXACTSoundBank* pSoundBank, DWORD* pdwState);

#undef INTERFACE
#define INTERFACE IXACTSoundBank

DECLARE_INTERFACE(IXACTSoundBank)
{
    STDMETHOD_(XACTINDEX, GetCueIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(Prepare)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue) PURE;
    STDMETHOD(Play)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue) PURE;
    STDMETHOD(Stop)(THIS_ XACTINDEX nCueIndex, DWORD dwFlags) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetState)(THIS_ DWORD* pdwState) PURE;
};

#ifdef __cplusplus

__inline HRESULT __stdcall IXACTSoundBank_Destroy(IXACTSoundBank* pSoundBank)
{
    return pSoundBank->Destroy();
}

__inline XACTINDEX __stdcall IXACTSoundBank_GetCueIndex(IXACTSoundBank* pSoundBank, PCSTR szFriendlyName)
{
    return pSoundBank->GetCueIndex(szFriendlyName);
}

__inline HRESULT __stdcall IXACTSoundBank_Prepare(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue)
{
    return pSoundBank->Prepare(nCueIndex, dwFlags, timeOffset, ppCue);
}

__inline HRESULT __stdcall IXACTSoundBank_Play(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue)
{
    return pSoundBank->Play(nCueIndex, dwFlags, timeOffset, ppCue);
}

__inline HRESULT __stdcall IXACTSoundBank_Stop(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags)
{
    return pSoundBank->Stop(nCueIndex, dwFlags);
}

__inline HRESULT __stdcall IXACTSoundBank_GetState(IXACTSoundBank* pSoundBank, DWORD* pdwState)
{
    return pSoundBank->GetState(pdwState);
}

#else // __cplusplus

__inline HRESULT __stdcall IXACTSoundBank_Destroy(IXACTSoundBank* pSoundBank)
{
    return pSoundBank->lpVtbl->Destroy(pSoundBank);
}

__inline XACTINDEX __stdcall IXACTSoundBank_GetCueIndex(IXACTSoundBank* pSoundBank, PCSTR szFriendlyName)
{
    return pSoundBank->lpVtbl->GetCueIndex(pSoundBank, szFriendlyName);
}

__inline HRESULT __stdcall IXACTSoundBank_Prepare(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue)
{
    return pSoundBank->lpVtbl->Prepare(pSoundBank, nCueIndex, dwFlags, timeOffset, ppCue);
}

__inline HRESULT __stdcall IXACTSoundBank_Play(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags, XACTTIME timeOffset, IXACTCue** ppCue)
{
    return pSoundBank->lpVtbl->Play(pSoundBank, nCueIndex, dwFlags, timeOffset, ppCue);
}

__inline HRESULT __stdcall IXACTSoundBank_Stop(IXACTSoundBank* pSoundBank, XACTINDEX nCueIndex, DWORD dwFlags)
{
    return pSoundBank->lpVtbl->Stop(pSoundBank, nCueIndex, dwFlags);
}

__inline HRESULT __stdcall IXACTSoundBank_GetState(IXACTSoundBank* pSoundBank, DWORD* pdwState)
{
    return pSoundBank->lpVtbl->GetState(pSoundBank, pdwState);
}

#endif // __cplusplus

//------------------------------------------------------------------------------
// IXACTWaveBank
//------------------------------------------------------------------------------

static const DWORD XACT_WAVEBANKSTATE_INUSE         = 0x00000001;  // Currently in-use
static const DWORD XACT_WAVEBANKSTATE_PREPARED      = 0x00000002;  // Prepared
static const DWORD XACT_WAVEBANKSTATE_PREPAREFAILED = 0x00000004;  // Prepare failed.

STDAPI IXACTWaveBank_Destroy(IXACTWaveBank* pWaveBank);
STDAPI IXACTWaveBank_GetState(IXACTWaveBank* pWaveBank, DWORD* pdwState);

#undef INTERFACE
#define INTERFACE IXACTWaveBank

DECLARE_INTERFACE(IXACTWaveBank)
{
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetState)(THIS_ DWORD* pdwState) PURE;
};

#ifdef __cplusplus

__inline HRESULT __stdcall IXACTWaveBank_Destroy(IXACTWaveBank* pWaveBank)
{
    return pWaveBank->Destroy();
}

__inline HRESULT __stdcall IXACTWaveBank_GetState(IXACTWaveBank* pWaveBank, DWORD* pdwState)
{
    return pWaveBank->GetState(pdwState);
}

#else // __cplusplus

__inline HRESULT __stdcall IXACTWaveBank_Destroy(IXACTWaveBank* pWaveBank)
{
    return pWaveBank->lpVtbl->Destroy(pWaveBank);
}

__inline HRESULT __stdcall IXACTWaveBank_GetState(IXACTWaveBank* pWaveBank, DWORD* pdwState)
{
    return pWaveBank->lpVtbl->GetState(pWaveBank, pdwState);
}

#endif // __cplusplus

//------------------------------------------------------------------------------
// IXACTCue
//------------------------------------------------------------------------------

// Cue Flags
static const DWORD XACT_FLAG_CUE_STOP_RELEASE           = 0x00000000;
static const DWORD XACT_FLAG_CUE_STOP_IMMEDIATE         = 0x00000001;

// Mutually exclusive states
static const DWORD XACT_CUESTATE_CREATED   = 0x00000001;  // Created, but nothing else
static const DWORD XACT_CUESTATE_PREPARING = 0x00000002;  // In the middle of preparing
static const DWORD XACT_CUESTATE_PREPARED  = 0x00000004;  // Prepared, but not yet played
static const DWORD XACT_CUESTATE_PLAYING   = 0x00000008;  // Playing (though could be paused)
static const DWORD XACT_CUESTATE_STOPPING  = 0x00000010;  // Stopping
static const DWORD XACT_CUESTATE_STOPPED   = 0x00000020;  // Stopped

// Inclusive states
static const DWORD XACT_CUESTATE_PAUSED    = 0x00000040;  // Paused (can be combinded with other states)

STDAPI IXACTCue_Destroy(IXACTCue* pCue);
STDAPI IXACTCue_Play(IXACTCue* pCue);
STDAPI IXACTCue_Stop(IXACTCue* pCue, DWORD dwFlags);
STDAPI IXACTCue_GetState(IXACTCue* pCue, DWORD* pdwState);
STDAPI IXACTCue_GetChannelMap(IXACTCue*, LPXACTCHANNELMAP pChannelMap, DWORD BufferSize, LPDWORD pRequiredSize);
STDAPI IXACTCue_SetChannelMap(IXACTCue*, LPCXACTCHANNELMAP pChannelMap);
STDAPI IXACTCue_GetChannelVolume(IXACTCue*, LPXACTCHANNELVOLUME pVolume);
STDAPI IXACTCue_SetChannelVolume(IXACTCue*, LPCXACTCHANNELVOLUME pVolume);
STDAPI IXACTCue_SetMatrixCoefficients(IXACTCue*, UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float* pMatrixCoefficients);
STDAPI_(XACTVARIABLEINDEX) IXACTCue_GetVariableIndex(IXACTCue* pCue, PCSTR szFriendlyName);
STDAPI IXACTCue_SetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue);
STDAPI IXACTCue_GetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* nValue);
STDAPI IXACTCue_Pause(IXACTCue* pCue, BOOL fPause);
#ifdef _XBOX
STDAPI IXACTCue_SetVoiceOutput(IXACTCue*, LPCXAUDIOVOICEOUTPUT pVoiceOutput);
STDAPI IXACTCue_SetVoiceOutputVolume(IXACTCue*, LPCXAUDIOVOICEOUTPUTVOLUME pVolume);
STDAPI IXACTCue_GetProperties(IXACTCue*, LPXACTCUEPROPERTIES pProperties);
#endif // _XBOX

#undef INTERFACE
#define INTERFACE IXACTCue

DECLARE_INTERFACE(IXACTCue)
{
    STDMETHOD(Play)(THIS) PURE;
    STDMETHOD(Stop)(THIS_ DWORD dwFlags) PURE;
    STDMETHOD(GetState)(THIS_ DWORD* pdwState) PURE;
    STDMETHOD(Destroy)(THIS) PURE;
    STDMETHOD(GetChannelMap)(THIS_ LPXACTCHANNELMAP pChannelMap, DWORD BufferSize, LPDWORD pRequiredSize) PURE;
    STDMETHOD(SetChannelMap)(THIS_ LPCXACTCHANNELMAP pChannelMap) PURE;
    STDMETHOD(GetChannelVolume)(THIS_ LPXACTCHANNELVOLUME pVolume) PURE;
    STDMETHOD(SetChannelVolume)(THIS_ LPCXACTCHANNELVOLUME pVolume) PURE;
    STDMETHOD(SetMatrixCoefficients)(THIS_ UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float* pMatrixCoefficients) PURE;
    STDMETHOD_(XACTVARIABLEINDEX, GetVariableIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(SetVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue) PURE;
    STDMETHOD(GetVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* nValue) PURE;
    STDMETHOD(Pause)(THIS_ BOOL fPause) PURE;
#ifdef _XBOX
    STDMETHOD(SetVoiceOutput)(THIS_ LPCXAUDIOVOICEOUTPUT pVoiceOutput) PURE;
    STDMETHOD(SetVoiceOutputVolume)(THIS_ LPCXAUDIOVOICEOUTPUTVOLUME pVolume) PURE;
    STDMETHOD(GetProperties)(THIS_ LPXACTCUEPROPERTIES pProperties) PURE;
#endif // _XBOX
};

#ifdef __cplusplus

__inline HRESULT __stdcall IXACTCue_Play(IXACTCue* pCue)
{
    return pCue->Play();
}

__inline HRESULT __stdcall IXACTCue_Stop(IXACTCue* pCue, DWORD dwFlags)
{
    return pCue->Stop(dwFlags);
}

__inline HRESULT __stdcall IXACTCue_GetState(IXACTCue* pCue, DWORD* pdwState)
{
    return pCue->GetState(pdwState);
}

__inline HRESULT __stdcall IXACTCue_Destroy(IXACTCue* pCue)
{
    return pCue->Destroy();
}

__inline HRESULT __stdcall IXACTCue_GetChannelMap(IXACTCue* pCue, LPXACTCHANNELMAP pChannelMap, DWORD BufferSize, LPDWORD pRequiredSize)
{
    return pCue->GetChannelMap(pChannelMap, BufferSize, pRequiredSize);
}

__inline HRESULT __stdcall IXACTCue_SetChannelMap(IXACTCue* pCue, LPCXACTCHANNELMAP pChannelMap)
{
    return pCue->SetChannelMap(pChannelMap);
}

__inline HRESULT __stdcall IXACTCue_GetChannelVolume(IXACTCue* pCue, LPXACTCHANNELVOLUME pVolume)
{
    return pCue->GetChannelVolume(pVolume);
}

__inline HRESULT __stdcall IXACTCue_SetChannelVolume(IXACTCue* pCue, LPCXACTCHANNELVOLUME pVolume)
{
    return pCue->SetChannelVolume(pVolume);
}

__inline HRESULT __stdcall IXACTCue_SetMatrixCoefficients(IXACTCue* pCue, UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float* pMatrixCoefficients)
{
    return pCue->SetMatrixCoefficients(uSrcChannelCount, uDstChannelCount, pMatrixCoefficients);
}

__inline XACTVARIABLEINDEX __stdcall IXACTCue_GetVariableIndex(IXACTCue* pCue, PCSTR szFriendlyName)
{
    return pCue->GetVariableIndex(szFriendlyName);
}

__inline HRESULT __stdcall IXACTCue_SetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    return pCue->SetVariable(nIndex, nValue);
}

__inline HRESULT __stdcall IXACTCue_GetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* pnValue)
{
    return pCue->GetVariable(nIndex, pnValue);
}

__inline HRESULT __stdcall IXACTCue_Pause(IXACTCue* pCue, BOOL fPause)
{
    return pCue->Pause(fPause);
}

#ifdef _XBOX
__inline HRESULT __stdcall IXACTCue_SetVoiceOutput(IXACTCue* pCue, LPCXAUDIOVOICEOUTPUT pVoiceOutput)
{
    return pCue->SetVoiceOutput(pVoiceOutput);
}

__inline HRESULT __stdcall IXACTCue_SetVoiceOutputVolume(IXACTCue* pCue, LPCXAUDIOVOICEOUTPUTVOLUME pVolume)
{
    return pCue->SetVoiceOutputVolume(pVolume);
}

__inline HRESULT __stdcall IXACTCue_GetProperties(IXACTCue* pCue, LPXACTCUEPROPERTIES pProperties)
{
    return pCue->GetProperties(pProperties);
}
#endif // _XBOX

#else // __cplusplus

__inline HRESULT __stdcall IXACTCue_Play(IXACTCue* pCue)
{
    return pCue->lpVtbl->Play(pCue);
}

__inline HRESULT __stdcall IXACTCue_Stop(IXACTCue* pCue, DWORD dwFlags)
{
    return pCue->lpVtbl->Stop(pCue, dwFlags);
}

__inline HRESULT __stdcall IXACTCue_GetState(IXACTCue* pCue, DWORD* pdwState)
{
    return pCue->lpVtbl->GetState(pCue, pdwState);
}

__inline HRESULT __stdcall IXACTCue_Destroy(IXACTCue* pCue)
{
    return pCue->lpVtbl->Destroy(pCue);
}

__inline HRESULT __stdcall IXACTCue_GetChannelMap(IXACTCue* pCue, LPXACTCHANNELMAP pChannelMap, DWORD BufferSize, LPDWORD pRequiredSize)
{
    return pCue->lpVtbl->GetChannelMap(pCue, pChannelMap, BufferSize, pRequiredSize);
}

__inline HRESULT __stdcall IXACTCue_SetChannelMap(IXACTCue* pCue, LPCXACTCHANNELMAP pChannelMap)
{
    return pCue->lpVtbl->SetChannelMap(pCue, pChannelMap);
}

__inline HRESULT __stdcall IXACTCue_GetChannelVolume(IXACTCue* pCue, LPXACTCHANNELVOLUME pVolume)
{
    return pCue->lpVtbl->GetChannelVolume(pCue, pVolume);
}

__inline HRESULT __stdcall IXACTCue_SetChannelVolume(IXACTCue* pCue, LPCXACTCHANNELVOLUME pVolume)
{
    return pCue->lpVtbl->SetChannelVolume(pCue, pVolume);
}

__inline HRESULT __stdcall IXACTCue_SetMatrixCoefficients(IXACTCue* pCue, UINT32 uSrcChannelCount, UINT32 uDstChannelCount, float* pMatrixCoefficients)
{
    return pCue->lpVtbl->SetMatrixCoefficients(pCue, uSrcChannelCount, uDstChannelCount, pMatrixCoefficients);
}

__inline XACTVARIABLEINDEX __stdcall IXACTCue_GetVariableIndex(IXACTCue* pCue, PCSTR szFriendlyName)
{
    return pCue->lpVtbl->GetVariableIndex(pCue, szFriendlyName);
}

__inline HRESULT __stdcall IXACTCue_SetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    return pCue->lpVtbl->SetVariable(pCue, nIndex, nValue);
}

__inline HRESULT __stdcall IXACTCue_GetVariable(IXACTCue* pCue, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* pnValue)
{
    return pCue->lpVtbl->GetVariable(pCue, nIndex, pnValue);
}

__inline HRESULT __stdcall IXACTCue_Pause(IXACTCue* pCue, BOOL fPause)
{
    return pCue->lpVtbl->Pause(pCue, fPause);
}

#ifdef _XBOX
__inline HRESULT __stdcall IXACTCue_SetVoiceOutput(IXACTCue* pCue, LPCXAUDIOVOICEOUTPUT pVoiceOutput)
{
    return pCue->lpVtbl->SetVoiceOutput(pCue, pVoiceOutput);
}

__inline HRESULT __stdcall IXACTCue_SetVoiceOutputVolume(IXACTCue* pCue, LPCXAUDIOVOICEOUTPUTVOLUME pVolume)
{
    return pCue->lpVtbl->SetVoiceOutputVolume(pCue, pVolume);
}

__inline HRESULT __stdcall IXACTCue_GetProperties(IXACTCue* pCue, LPXACTCUEPROPERTIES pProperties)
{
    return pCue->lpVtbl->GetProperties(pCue, pProperties);
}
#endif // _XBOX

#endif // __cplusplus

//------------------------------------------------------------------------------
// IXACTEngine
//------------------------------------------------------------------------------

// Engine flags
static const DWORD XACT_FLAG_ENGINE_CREATE_MANAGEDATA  = 0x00000001;
static const DWORD XACT_FLAG_ENGINE_STOP_IMMEDIATE     = 0x00000002;

STDAPI_(ULONG) IXACTEngine_AddRef(IXACTEngine* pEngine);
STDAPI_(ULONG) IXACTEngine_Release(IXACTEngine* pEngine);
#ifndef _XBOX
STDAPI IXACTEngine_GetRendererCount(IXACTEngine* pEngine, XACTINDEX* pnRendererCount);
STDAPI IXACTEngine_GetRendererDetails(IXACTEngine* pEngine, XACTINDEX nRendererIndex, LPXACT_RENDERER_DETAILS pRendererDetails);
#endif
STDAPI IXACTEngine_Initialize(IXACTEngine* pEngine, const XACT_RUNTIME_PARAMETERS* pParams);
STDAPI IXACTEngine_ShutDown(IXACTEngine* pEngine);
STDAPI IXACTEngine_DoWork(IXACTEngine* pEngine);
STDAPI IXACTEngine_CreateSoundBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTSoundBank** ppSoundBank);
STDAPI IXACTEngine_CreateInMemoryWaveBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTWaveBank** ppWaveBank);
STDAPI IXACTEngine_CreateStreamingWaveBank(IXACTEngine* pEngine, const XACT_WAVEBANK_STREAMING_PARAMETERS* pParms, IXACTWaveBank** ppWaveBank);
STDAPI IXACTEngine_RegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc);
STDAPI IXACTEngine_UnRegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc);
STDAPI_(XACTCATEGORY) IXACTEngine_GetCategory(IXACTEngine* pEngine, PCSTR szFriendlyName);
STDAPI IXACTEngine_Stop(IXACTEngine* pEngine, XACTCATEGORY nCategory, DWORD dwFlags);
STDAPI IXACTEngine_SetVolume(IXACTEngine* pEngine, XACTCATEGORY nCategory, XACTVOLUME nVolume);
STDAPI IXACTEngine_Pause(IXACTEngine* pEngine, XACTCATEGORY nCategory, BOOL fPause);
STDAPI_(XACTVARIABLEINDEX) IXACTEngine_GetGlobalVariableIndex(IXACTEngine* pEngine, PCSTR szFriendlyName);
STDAPI IXACTEngine_SetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue);
STDAPI IXACTEngine_GetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* pnValue);

#undef INTERFACE
#define INTERFACE IXACTEngine

#ifdef _XBOX
DECLARE_INTERFACE(IXACTEngine)
{
#else
DECLARE_INTERFACE_(IXACTEngine, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, OUT void** ppvObj) PURE;
#endif

    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

#ifndef _XBOX
    STDMETHOD(GetRendererCount)(THIS_ XACTINDEX* pnRendererCount) PURE;
    STDMETHOD(GetRendererDetails)(THIS_ XACTINDEX nRendererIndex, LPXACT_RENDERER_DETAILS pRendererDetails) PURE;
#endif

    STDMETHOD(Initialize)(THIS_ const XACT_RUNTIME_PARAMETERS* pParams) PURE;
    STDMETHOD(ShutDown)(THIS) PURE;

    STDMETHOD(DoWork)(THIS) PURE;

    STDMETHOD(CreateSoundBank)(THIS_ const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTSoundBank** ppSoundBank) PURE;
    STDMETHOD(CreateInMemoryWaveBank)(THIS_ const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTWaveBank** ppWaveBank) PURE;
    STDMETHOD(CreateStreamingWaveBank)(THIS_ const XACT_WAVEBANK_STREAMING_PARAMETERS* pParms, IXACTWaveBank** ppWaveBank) PURE;

    STDMETHOD(RegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc) PURE;
    STDMETHOD(UnRegisterNotification)(THIS_ const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc) PURE;

    STDMETHOD_(XACTCATEGORY, GetCategory)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(Stop)(THIS_ XACTCATEGORY nCategory, DWORD dwFlags) PURE;
    STDMETHOD(SetVolume)(THIS_ XACTCATEGORY nCategory, XACTVOLUME nVolume) PURE;
    STDMETHOD(Pause)(THIS_ XACTCATEGORY nCategory, BOOL fPause) PURE;

    STDMETHOD_(XACTVARIABLEINDEX, GetGlobalVariableIndex)(THIS_ PCSTR szFriendlyName) PURE;
    STDMETHOD(SetGlobalVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue) PURE;
    STDMETHOD(GetGlobalVariable)(THIS_ XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* nValue) PURE;
};

#ifdef __cplusplus

__inline ULONG __stdcall IXACTEngine_AddRef(IXACTEngine* pEngine)
{
    return pEngine->AddRef();
}

__inline ULONG __stdcall IXACTEngine_Release(IXACTEngine* pEngine)
{
    return pEngine->Release();
}

#ifndef _XBOX
__inline HRESULT __stdcall IXACTEngine_GetRendererCount(IXACTEngine* pEngine, XACTINDEX* pnRendererCount)
{
    return pEngine->GetRendererCount(pnRendererCount);
}

__inline HRESULT __stdcall IXACTEngine_GetRendererDetails(IXACTEngine* pEngine, XACTINDEX nRendererIndex, LPXACT_RENDERER_DETAILS pRendererDetails)
{
    return pEngine->GetRendererDetails(nRendererIndex, pRendererDetails);
}
#endif

__inline HRESULT __stdcall IXACTEngine_Initialize(IXACTEngine* pEngine, const XACT_RUNTIME_PARAMETERS* pParams)
{
    return pEngine->Initialize(pParams);
}

__inline HRESULT __stdcall IXACTEngine_ShutDown(IXACTEngine* pEngine)
{
    return pEngine->ShutDown();
}

__inline HRESULT __stdcall IXACTEngine_DoWork(IXACTEngine* pEngine)
{
    return pEngine->DoWork();
}

__inline HRESULT __stdcall IXACTEngine_CreateSoundBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTSoundBank** ppSoundBank)
{
    return pEngine->CreateSoundBank(pvBuffer, dwSize, dwFlags, dwAllocAttributes, ppSoundBank);
}

__inline HRESULT __stdcall IXACTEngine_CreateInMemoryWaveBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTWaveBank** ppWaveBank)
{
    return pEngine->CreateInMemoryWaveBank(pvBuffer, dwSize, dwFlags, dwAllocAttributes, ppWaveBank);
}

__inline HRESULT __stdcall IXACTEngine_CreateStreamingWaveBank(IXACTEngine* pEngine, const XACT_WAVEBANK_STREAMING_PARAMETERS* pParms, IXACTWaveBank** ppWaveBank)
{
    return pEngine->CreateStreamingWaveBank(pParms, ppWaveBank);
}

__inline HRESULT __stdcall IXACTEngine_RegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc)
{
    return pEngine->RegisterNotification(pNotificationDesc);
}

__inline HRESULT __stdcall IXACTEngine_UnRegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc)
{
    return pEngine->UnRegisterNotification(pNotificationDesc);
}

__inline XACTCATEGORY __stdcall IXACTEngine_GetCategory(IXACTEngine* pEngine, PCSTR szFriendlyName)
{
    return pEngine->GetCategory(szFriendlyName);
}

__inline HRESULT __stdcall IXACTEngine_Stop(IXACTEngine* pEngine, XACTCATEGORY nCategory, DWORD dwFlags)
{
    return pEngine->Stop(nCategory, dwFlags);
}

__inline HRESULT __stdcall IXACTEngine_SetVolume(IXACTEngine* pEngine, XACTCATEGORY nCategory, XACTVOLUME nVolume)
{
    return pEngine->SetVolume(nCategory, nVolume);
}

__inline HRESULT __stdcall IXACTEngine_Pause(IXACTEngine* pEngine, XACTCATEGORY nCategory, BOOL fPause)
{
    return pEngine->Pause(nCategory, fPause);
}

__inline XACTVARIABLEINDEX __stdcall IXACTEngine_GetGlobalVariableIndex(IXACTEngine* pEngine, PCSTR szFriendlyName)
{
    return pEngine->GetGlobalVariableIndex(szFriendlyName);
}

__inline HRESULT __stdcall IXACTEngine_SetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    return pEngine->SetGlobalVariable(nIndex, nValue);
}

__inline HRESULT __stdcall IXACTEngine_GetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* nValue)
{
    return pEngine->GetGlobalVariable(nIndex, nValue);
}

#else // __cplusplus

__inline ULONG __stdcall IXACTEngine_AddRef(IXACTEngine* pEngine)
{
    return pEngine->lpVtbl->AddRef(pEngine);
}

__inline ULONG __stdcall IXACTEngine_Release(IXACTEngine* pEngine)
{
    return pEngine->lpVtbl->Release(pEngine);
}

#ifndef _XBOX
__inline HRESULT __stdcall IXACTEngine_GetRendererCount(IXACTEngine* pEngine, XACTINDEX* pnRendererCount)
{
    return pEngine->lpVtbl->GetRendererCount(pEngine, pnRendererCount);
}

__inline HRESULT __stdcall IXACTEngine_GetRendererDetails(IXACTEngine* pEngine, XACTINDEX nRendererIndex, LPXACT_RENDERER_DETAILS pRendererDetails)
{
    return pEngine->lpVtbl->GetRendererDetails(pEngine, nRendererIndex, pRendererDetails);
}
#endif

__inline HRESULT __stdcall IXACTEngine_Initialize(IXACTEngine* pEngine, const XACT_RUNTIME_PARAMETERS* pParams)
{
    return pEngine->lpVtbl->Initialize(pEngine, pParams);
}

__inline HRESULT __stdcall IXACTEngine_ShutDown(IXACTEngine* pEngine)
{
    return pEngine->lpVtbl->ShutDown(pEngine);
}

__inline HRESULT __stdcall IXACTEngine_DoWork(IXACTEngine* pEngine)
{
    return pEngine->lpVtbl->DoWork(pEngine);
}

__inline HRESULT __stdcall IXACTEngine_CreateSoundBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTSoundBank** ppSoundBank)
{
    return pEngine->lpVtbl->CreateSoundBank(pEngine, pvBuffer, dwSize, dwFlags, dwAllocAttributes, ppSoundBank);
}

__inline HRESULT __stdcall IXACTEngine_CreateInMemoryWaveBank(IXACTEngine* pEngine, const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTWaveBank** ppWaveBank)
{
    return pEngine->lpVtbl->CreateInMemoryWaveBank(pEngine, pvBuffer, dwSize, dwFlags, dwAllocAttributes, ppWaveBank);
}

__inline HRESULT __stdcall IXACTEngine_CreateStreamingWaveBank(IXACTEngine* pEngine, const XACT_WAVEBANK_STREAMING_PARAMETERS* pParms, IXACTWaveBank** ppWaveBank)
{
    return pEngine->lpVtbl->CreateStreamingWaveBank(pEngine, pParms, ppWaveBank);
}

__inline HRESULT __stdcall IXACTEngine_RegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc)
{
    return pEngine->lpVtbl->RegisterNotification(pEngine, pNotificationDesc);
}

__inline HRESULT __stdcall IXACTEngine_UnRegisterNotification(IXACTEngine* pEngine, const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc)
{
    return pEngine->lpVtbl->UnRegisterNotification(pEngine, pNotificationDesc);
}

__inline XACTCATEGORY __stdcall IXACTEngine_GetCategory(IXACTEngine* pEngine, PCSTR szFriendlyName)
{
    return pEngine->lpVtbl->GetCategory(pEngine, szFriendlyName);
}

__inline HRESULT __stdcall IXACTEngine_Stop(IXACTEngine* pEngine, XACTCATEGORY nCategory, DWORD dwFlags)
{
    return pEngine->lpVtbl->Stop(pEngine, nCategory, dwFlags);
}

__inline HRESULT __stdcall IXACTEngine_SetVolume(IXACTEngine* pEngine, XACTCATEGORY nCategory, XACTVOLUME nVolume)
{
    return pEngine->lpVtbl->SetVolume(pEngine, nCategory, nVolume);
}

__inline HRESULT __stdcall IXACTEngine_Pause(IXACTEngine* pEngine, XACTCATEGORY nCategory, BOOL fPause)
{
    return pEngine->lpVtbl->Pause(pEngine, nCategory, fPause);
}

__inline XACTVARIABLEINDEX __stdcall IXACTEngine_GetGlobalVariableIndex(IXACTEngine* pEngine, PCSTR szFriendlyName)
{
    return pEngine->lpVtbl->GetGlobalVariableIndex(pEngine, szFriendlyName);
}

__inline HRESULT __stdcall IXACTEngine_SetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue)
{
    return pEngine->lpVtbl->SetGlobalVariable(pEngine, nIndex, nValue);
}

__inline HRESULT __stdcall IXACTEngine_GetGlobalVariable(IXACTEngine* pEngine, XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* nValue)
{
    return pEngine->lpVtbl->GetGlobalVariable(pEngine, nIndex, nValue);
}

#endif // __cplusplus

//------------------------------------------------------------------------------
// XACT API's (these are deprecated and will be removed in a future release)
//------------------------------------------------------------------------------

#ifdef _XBOX

static const DWORD XACT_FLAG_API_CREATE_MANAGEDATA  = 0x00000001;
static const DWORD XACT_FLAG_API_STOP_IMMEDIATE     = 0x00000002;

STDAPI XACTInitialize(const XACT_RUNTIME_PARAMETERS* pParams);
STDAPI XACTShutDown(void);
STDAPI XACTDoWork(void);

STDAPI XACTCreateSoundBank(const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTSoundBank** ppSoundBank);
STDAPI XACTCreateInMemoryWaveBank(const void* pvBuffer, DWORD dwSize, DWORD dwFlags, DWORD dwAllocAttributes, IXACTWaveBank** ppWaveBank);
STDAPI XACTCreateStreamingWaveBank(const XACT_WAVEBANK_STREAMING_PARAMETERS* pParms, IXACTWaveBank** ppWaveBank);

STDAPI XACTRegisterNotification(const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc);
STDAPI XACTUnRegisterNotification(const XACT_NOTIFICATION_DESCRIPTION* pNotificationDesc);

STDAPI_(XACTCATEGORY) XACTGetCategory(PCSTR szFriendlyName);
STDAPI XACTStop(XACTCATEGORY nCategory, DWORD dwFlags);
STDAPI XACTSetVolume(XACTCATEGORY nCategory, XACTVOLUME nVolume);
STDAPI XACTPause(XACTCATEGORY nCategory, BOOL fPause);

STDAPI_(XACTVARIABLEINDEX) XACTGetGlobalVariableIndex(PCSTR szFriendlyName);
STDAPI XACTSetGlobalVariable(XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE nValue);
STDAPI XACTGetGlobalVariable(XACTVARIABLEINDEX nIndex, XACTVARIABLEVALUE* pnValue);

#endif // #ifdef _XBOX

//------------------------------------------------------------------------------
// Create Engine
//------------------------------------------------------------------------------

// Flags used only in XACTCreateEngine below.  These flags are valid but ignored
// when building for Xbox 360; to enable auditioning on that platform you must
// link explicitly to an auditioning version of the XACT static library.
static const DWORD XACT_FLAG_API_AUDITION_MODE = 0x00000001;
static const DWORD XACT_FLAG_API_DEBUG_MODE    = 0x00000002;

STDAPI XACTCreateEngine(DWORD dwCreationFlags, IXACTEngine** ppEngine);

#ifndef _XBOX

#if defined (UNICODE)
#   define XACT_DEBUGENGINE_REGISTRY_KEY   L"Software\\Microsoft\\XACT"
#   define XACT_DEBUGENGINE_REGISTRY_VALUE L"DebugEngine"
#else
#   define XACT_DEBUGENGINE_REGISTRY_KEY   "Software\\Microsoft\\XACT"
#   define XACT_DEBUGENGINE_REGISTRY_VALUE "DebugEngine"
#endif

#ifdef __cplusplus

__inline HRESULT __stdcall XACTCreateEngine(DWORD dwCreationFlags, IXACTEngine** ppEngine)
{
    HRESULT hr;
    HKEY    key;
    DWORD   data;
    DWORD   type     = REG_DWORD;
    DWORD   dataSize = sizeof(DWORD);
    BOOL    debug    = (dwCreationFlags & XACT_FLAG_API_DEBUG_MODE) ? TRUE : FALSE;
    BOOL    audition = (dwCreationFlags & XACT_FLAG_API_AUDITION_MODE) ? TRUE : FALSE;

    // If neither the debug nor audition flags are set, see if the debug registry key is set
    if(!debug && !audition &&
       (RegOpenKeyEx(HKEY_LOCAL_MACHINE, XACT_DEBUGENGINE_REGISTRY_KEY, 0, KEY_READ, &key) == ERROR_SUCCESS))
    {
        if(RegQueryValueEx(key, XACT_DEBUGENGINE_REGISTRY_VALUE, NULL, &type, (LPBYTE)&data, &dataSize) == ERROR_SUCCESS)
        {
            if(data)
            {
                debug = TRUE;
            }
        }
        RegCloseKey(key);
    }

    // Priority order: Audition, Debug, Retail
    hr = CoCreateInstance(audition ? __uuidof(XACTAuditionEngine)
                          : (debug ? __uuidof(XACTDebugEngine) : __uuidof(XACTEngine)),
                          NULL, CLSCTX_INPROC_SERVER, __uuidof(IXACTEngine), (void**)ppEngine);

    // If debug engine does not exist fallback to retail version
    if(FAILED(hr) && debug && !audition)
    {
        hr = CoCreateInstance(__uuidof(XACTEngine), NULL, CLSCTX_INPROC_SERVER, __uuidof(IXACTEngine), (void**)ppEngine);
    }

    return hr;
}

#else

__inline HRESULT __stdcall XACTCreateEngine(DWORD dwCreationFlags, IXACTEngine** ppEngine)
{
    HRESULT hr;
    HKEY    key;
    DWORD   data;
    DWORD   type     = REG_DWORD;
    DWORD   dataSize = sizeof(DWORD);
    BOOL    debug    = (dwCreationFlags & XACT_FLAG_API_DEBUG_MODE) ? TRUE : FALSE;
    BOOL    audition = (dwCreationFlags & XACT_FLAG_API_AUDITION_MODE) ? TRUE : FALSE;

    // If neither the debug nor audition flags are set, see if the debug registry key is set
    if(!debug && !audition &&
       (RegOpenKeyEx(HKEY_LOCAL_MACHINE, XACT_DEBUGENGINE_REGISTRY_KEY, 0, KEY_READ, &key) == ERROR_SUCCESS))
    {
        if(RegQueryValueEx(key, XACT_DEBUGENGINE_REGISTRY_VALUE, NULL, &type, (LPBYTE)&data, &dataSize) == ERROR_SUCCESS)
        {
            if(data)
            {
                debug = TRUE;
            }
        }
        RegCloseKey(key);
    }

    // Priority order: Audition, Debug, Retail
    hr = CoCreateInstance(audition ? &CLSID_XACTAuditionEngine
                          : (debug ? &CLSID_XACTDebugEngine : &CLSID_XACTEngine),
                          NULL, CLSCTX_INPROC_SERVER, &IID_IXACTEngine, (void**)ppEngine);

    // If debug engine does not exist fallback to retail version
    if(FAILED(hr) && debug && !audition)
    {
        hr = CoCreateInstance(&CLSID_XACTEngine, NULL, CLSCTX_INPROC_SERVER, &IID_IXACTEngine, (void**)ppEngine);
    }

    return hr;
}

#endif // #ifdef __cplusplus
#endif // #ifndef _XBOX

//------------------------------------------------------------------------------
// XACT specific error codes
//------------------------------------------------------------------------------

#define FACILITY_XACTENGINE 0xAC7
#define XACTENGINEERROR(n) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XACTENGINE, n)

#define XACTENGINE_E_OUTOFMEMORY               E_OUTOFMEMORY      // Out of memory
#define XACTENGINE_E_INVALIDARG                E_INVALIDARG       // Invalid arg
#define XACTENGINE_E_NOTIMPL                   E_NOTIMPL          // Not implemented
#define XACTENGINE_E_FAIL                      E_FAIL             // Unknown error

#define XACTENGINE_E_ALREADYINITIALIZED        XACTENGINEERROR(0x001)   // The engine is already initialized
#define XACTENGINE_E_NOTINITIALIZED            XACTENGINEERROR(0x002)   // The engine has not been initialized
#define XACTENGINE_E_EXPIRED                   XACTENGINEERROR(0x003)   // The engine has expired (demo or pre-release version)
#define XACTENGINE_E_NONOTIFICATIONCALLBACK    XACTENGINEERROR(0x004)   // No notification callback
#define XACTENGINE_E_NOTIFICATIONREGISTERED    XACTENGINEERROR(0x005)   // Notification already registered
#define XACTENGINE_E_INVALIDUSAGE              XACTENGINEERROR(0x006)   // Invalid usage
#define XACTENGINE_E_INVALIDDATA               XACTENGINEERROR(0x007)   // Invalid data
#define XACTENGINE_E_INSTANCELIMITFAILTOPLAY   XACTENGINEERROR(0x008)   // Fail to play due to instance limit
#define XACTENGINE_E_NOGLOBALSETTINGS          XACTENGINEERROR(0x009)   // Global Settings not loaded
#define XACTENGINE_E_INVALIDVARIABLEINDEX      XACTENGINEERROR(0x00a)   // Invalid variable index
#define XACTENGINE_E_INVALIDCATEGORY           XACTENGINEERROR(0x00b)   // Invalid category
#define XACTENGINE_E_INVALIDCUEINDEX           XACTENGINEERROR(0x00c)   // Invalid cue index
#define XACTENGINE_E_INVALIDWAVEINDEX          XACTENGINEERROR(0x00d)   // Invalid wave index
#define XACTENGINE_E_INVALIDTRACKINDEX         XACTENGINEERROR(0x00e)   // Invalid track index
#define XACTENGINE_E_INVALIDSOUNDOFFSETORINDEX XACTENGINEERROR(0x00f)   // Invalid sound offset or index
#define XACTENGINE_E_READFILE                  XACTENGINEERROR(0x010)   // Error reading a file
#define XACTENGINE_E_UNKNOWNEVENT              XACTENGINEERROR(0x011)   // Unknown event type
#define XACTENGINE_E_INCALLBACK                XACTENGINEERROR(0x012)   // Invalid call of method of function from callback
#define XACTENGINE_E_NOWAVEBANK                XACTENGINEERROR(0x013)   // No wavebank exists for desired operation
#define XACTENGINE_E_SELECTVARIATION           XACTENGINEERROR(0x014)   // Unable to select a variation
#define XACTENGINE_E_MULTIPLEAUDITIONENGINES   XACTENGINEERROR(0x015)   // There can be only one audition engine
#define XACTENGINE_E_WAVEBANKNOTPREPARED       XACTENGINEERROR(0x016)   // The wavebank is not prepared
#define XACTENGINE_E_NORENDERER                XACTENGINEERROR(0x017)   // No audio device found on.
#define XACTENGINE_E_INVALIDENTRYCOUNT         XACTENGINEERROR(0x018)   // Invalid entry count for channel maps
#define XACTENGINE_E_SEEKTIMEBEYONDCUEEND      XACTENGINEERROR(0x019)   // Time offset for seeking is beyond the cue end.

#define XACTENGINE_E_AUDITION_WRITEFILE             XACTENGINEERROR(0x101)  // Error writing a file during auditioning
#define XACTENGINE_E_AUDITION_NOSOUNDBANK           XACTENGINEERROR(0x102)  // Missing a soundbank
#define XACTENGINE_E_AUDITION_INVALIDRPCINDEX       XACTENGINEERROR(0x103)  // Missing an RPC curve
#define XACTENGINE_E_AUDITION_MISSINGDATA           XACTENGINEERROR(0x104)  // Missing data for an audition command
#define XACTENGINE_E_AUDITION_UNKNOWNCOMMAND        XACTENGINEERROR(0x105)  // Unknown command
#define XACTENGINE_E_AUDITION_INVALIDDSPINDEX       XACTENGINEERROR(0x106)  // Missing a DSP parameter
#define XACTENGINE_E_AUDITION_MISSINGWAVE           XACTENGINEERROR(0x107)  // Wave does not exist in auditioned wavebank
#define XACTENGINE_E_AUDITION_CREATEDIRECTORYFAILED XACTENGINEERROR(0x108)  // Failed to create a directory for streaming wavebank data
#define XACTENGINE_E_AUDITION_INVALIDSESSION        XACTENGINEERROR(0x109)  // Invalid audition session
#endif // #ifndef GUID_DEFS_ONLY
#endif // #ifndef _XACT_H_
