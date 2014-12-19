/*-========================================================================-_
 |                                - XACT3D -                                |
 |        Copyright (c) Microsoft Corporation.  All rights reserved.        |
 |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
 |VERSION:  0.1                         MODEL:   Unmanaged User-mode        |
 |CONTRACT: N / A                       EXCEPT:  No Exceptions              |
 |PARENT:   N / A                       MINREQ:  Win2000, Xbox360           |
 |PROJECT:  XACT3D                      DIALECT: MS Visual C++ 7.0          |
 |>------------------------------------------------------------------------<|
 | DUTY: XACT 3D support                                                    |
 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
  NOTES:
    1.  See X3DAudio.h for information regarding X3DAudio types.            */


#ifndef __XACT3D_H__
#define __XACT3D_H__
//--------------<D-E-F-I-N-I-T-I-O-N-S>-------------------------------------//
    #include <x3daudio.h>
    #include <xact.h>

    // Supported speaker positions, represented as azimuth angles.
    //
    // Here's a picture of the azimuth angles for the 8 cardinal points,
    // seen from above.  The emitter's base position is at the origin 0.
    //
    //           FRONT
    //             | 0  <-- azimuth
    //             |
    //    7pi/4 \  |  / pi/4
    //           \ | /
    // LEFT       \|/      RIGHT
    // 3pi/2-------0-------pi/2
    //            /|\
    //           / | \
    //    5pi/4 /  |  \ 3pi/4
    //             |
    //             | pi
    //           BACK
    //
    #define LEFT_AZIMUTH                    (3*X3DAUDIO_PI/2)
    #define RIGHT_AZIMUTH                   (X3DAUDIO_PI/2)
    #define FRONT_LEFT_AZIMUTH              (7*X3DAUDIO_PI/4)
    #define FRONT_RIGHT_AZIMUTH             (X3DAUDIO_PI/4)
    #define FRONT_CENTER_AZIMUTH            0.0f
    #define LOW_FREQUENCY_AZIMUTH           X3DAUDIO_2PI
    #define BACK_LEFT_AZIMUTH               (5*X3DAUDIO_PI/4)
    #define BACK_RIGHT_AZIMUTH              (3*X3DAUDIO_PI/4)
    #define BACK_CENTER_AZIMUTH             X3DAUDIO_PI
    #define FRONT_LEFT_OF_CENTER_AZIMUTH    (15*X3DAUDIO_PI/8)
    #define FRONT_RIGHT_OF_CENTER_AZIMUTH   (X3DAUDIO_PI/8)


//--------------<D-A-T-A---T-Y-P-E-S>---------------------------------------//
    // Supported emitter channel layouts:
    static const float aStereoLayout[] =
    {
        LEFT_AZIMUTH,
        RIGHT_AZIMUTH
    };
    static const float a2Point1Layout[] =
    {
        LEFT_AZIMUTH,
        RIGHT_AZIMUTH,
        LOW_FREQUENCY_AZIMUTH
    };
    static const float aQuadLayout[] =
    {
        FRONT_LEFT_AZIMUTH,
        FRONT_RIGHT_AZIMUTH,
        BACK_LEFT_AZIMUTH,
        BACK_RIGHT_AZIMUTH
    };
    static const float a4Point1Layout[] =
    {
        FRONT_LEFT_AZIMUTH,
        FRONT_RIGHT_AZIMUTH,
        LOW_FREQUENCY_AZIMUTH,
        BACK_LEFT_AZIMUTH,
        BACK_RIGHT_AZIMUTH
    };
    static const float a5Point1Layout[] =
    {
        FRONT_LEFT_AZIMUTH,
        FRONT_RIGHT_AZIMUTH,
        FRONT_CENTER_AZIMUTH,
        LOW_FREQUENCY_AZIMUTH,
        BACK_LEFT_AZIMUTH,
        BACK_RIGHT_AZIMUTH
    };
    static const float a7Point1Layout[] =
    {
        FRONT_LEFT_AZIMUTH,
        FRONT_RIGHT_AZIMUTH,
        FRONT_CENTER_AZIMUTH,
        LOW_FREQUENCY_AZIMUTH,
        BACK_LEFT_AZIMUTH,
        BACK_RIGHT_AZIMUTH,
        LEFT_AZIMUTH,
        RIGHT_AZIMUTH
    };


//--------------<F-U-N-C-T-I-O-N-S>-----------------------------------------//
    ////
    // DESCRIPTION:
    //  Initializes the 3D API's:
    //
    // REMARKS:
    //  This method only needs to be called once
    //  The number of bits set in SpeakerChannelMask should equal the number of
    //  channels expected on the final mix.
    //
    // PARAMETERS:
    //  SpeakerChannelMask - [in]  speaker geometry configuration on the final mix, specifies assignment of channels to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask, must be != 0
    //                             Currently only SPEAKER_STEREO and SPEAKER_5POINT1 is supported by X3DAudio.
    //  pEngine            - [in]  pointer to the XACT engine
    //  X3DInstance        - [out] Handle to the X3DAudio instance
    //
    // RETURN VALUE:
    //  HResult error code
    ////
    EXTERN_C HRESULT inline XACT3DInitialize (UINT32 SpeakerChannelMask, IXACTEngine* pEngine, X3DAUDIO_HANDLE X3DInstance)
    {
        HRESULT hr = S_OK;
        if (pEngine == NULL) {
            hr = E_POINTER;
        }

        XACTVARIABLEVALUE nSpeedOfSound = 0.0f;
        if (SUCCEEDED(hr)) {
            XACTVARIABLEINDEX xactSpeedOfSoundID = pEngine->GetGlobalVariableIndex("SpeedOfSound");
            hr = pEngine->GetGlobalVariable(xactSpeedOfSoundID, &nSpeedOfSound);
        }
        if (SUCCEEDED(hr)) {
            X3DAudioInitialize(SpeakerChannelMask, nSpeedOfSound, X3DInstance);
        }

        return hr;
    }


    ////
    // DESCRIPTION:
    //  Calculates DSP settings with respect to 3D parameters:
    //
    // PARAMETERS:
    //  X3DInstance        - [in]  X3DAudio instance (returned from XACT3DInitialize)
    //  pListener          - [in]  point of 3D audio reception
    //  pEmitter           - [in]  3D audio source
    //  pDSPSettings       - [out] receives calculation results, applied to an XACT cue via XACT3DApply
    //
    // RETURN VALUE:
    //  HResult error code
    ////
    EXTERN_C HRESULT inline XACT3DCalculate (X3DAUDIO_HANDLE X3DInstance, const X3DAUDIO_LISTENER* pListener, X3DAUDIO_EMITTER* pEmitter, X3DAUDIO_DSP_SETTINGS* pDSPSettings)
    {
        HRESULT hr = S_OK;
        if (pListener == NULL || pEmitter == NULL || pDSPSettings == NULL) {
            hr = E_POINTER;
        }

        if(SUCCEEDED(hr)) {
            if (pEmitter->ChannelCount > 1 && pEmitter->pChannelAzimuths == NULL) {
                pEmitter->ChannelRadius = 1.0f;

                switch (pEmitter->ChannelCount) {
                    case 2: pEmitter->pChannelAzimuths = (float*)&aStereoLayout[0]; break;
                    case 3: pEmitter->pChannelAzimuths = (float*)&a2Point1Layout[0]; break;
                    case 4: pEmitter->pChannelAzimuths = (float*)&aQuadLayout[0]; break;
                    case 5: pEmitter->pChannelAzimuths = (float*)&a4Point1Layout[0]; break;
                    case 6: pEmitter->pChannelAzimuths = (float*)&a5Point1Layout[0]; break;
                    case 8: pEmitter->pChannelAzimuths = (float*)&a7Point1Layout[0]; break;
                    default: hr = E_FAIL; break;
                }
            }
        }

        if(SUCCEEDED(hr)) {
            static X3DAUDIO_DISTANCE_CURVE_POINT DefaultCurvePoints[2] = { 0.0f, 1.0f, 1.0f, 1.0f };
            static X3DAUDIO_DISTANCE_CURVE       DefaultCurve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&DefaultCurvePoints[0], 2 };
            if (pEmitter->pVolumeCurve == NULL) {
                pEmitter->pVolumeCurve = &DefaultCurve;
            }
            if (pEmitter->pLFECurve == NULL) {
                pEmitter->pLFECurve = &DefaultCurve;
            }

            X3DAudioCalculate(X3DInstance, pListener, pEmitter, X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_EMITTER_ANGLE, pDSPSettings);
        }

        return hr;
    }


    ////
    // DESCRIPTION:
    //  Applies a 3D calculation returned by XACT3DCalculate to a cue:
    //
    // PARAMETERS:
    //  pDSPSettings - [in] calculation results generated by XACT3DCalculate
    //  pCue         - [in] cue to which to apply pDSPSettings
    //
    // RETURN VALUE:
    //  HResult error code
    ////
    EXTERN_C HRESULT inline XACT3DApply (X3DAUDIO_DSP_SETTINGS* pDSPSettings, IXACTCue* pCue)
    {
        HRESULT hr = S_OK;
        if (pDSPSettings == NULL || pCue == NULL) {
            hr = E_POINTER;
        }

        if (SUCCEEDED(hr)) {
            hr = pCue->SetMatrixCoefficients(pDSPSettings->SrcChannelCount, pDSPSettings->DstChannelCount, pDSPSettings->pMatrixCoefficients);
        }
        if (SUCCEEDED(hr)) {
            XACTVARIABLEINDEX xactDistanceID = pCue->GetVariableIndex("Distance");
            hr = pCue->SetVariable(xactDistanceID, pDSPSettings->EmitterToListenerDistance);
        }
        if (SUCCEEDED(hr)) {
            XACTVARIABLEINDEX xactDopplerID = pCue->GetVariableIndex("DopplerPitchScalar");
            hr = pCue->SetVariable(xactDopplerID, pDSPSettings->DopplerFactor);
        }
        if (SUCCEEDED(hr)) {
            XACTVARIABLEINDEX xactOrientationID = pCue->GetVariableIndex("OrientationAngle");
            hr = pCue->SetVariable(xactOrientationID, pDSPSettings->EmitterToListenerAngle * (180.0f / X3DAUDIO_PI));
        }

        return hr;
    }


#endif // __XACT3D_H__
//---------------------------------<-EOF->----------------------------------//
