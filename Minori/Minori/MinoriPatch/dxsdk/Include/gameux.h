

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0486 */
/* Compiler settings for gameux.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __gameux_h__
#define __gameux_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGameExplorer_FWD_DEFINED__
#define __IGameExplorer_FWD_DEFINED__
typedef interface IGameExplorer IGameExplorer;
#endif 	/* __IGameExplorer_FWD_DEFINED__ */


#ifndef __GameExplorer_FWD_DEFINED__
#define __GameExplorer_FWD_DEFINED__

#ifdef __cplusplus
typedef class GameExplorer GameExplorer;
#else
typedef struct GameExplorer GameExplorer;
#endif /* __cplusplus */

#endif 	/* __GameExplorer_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_gameux_0000_0000 */
/* [local] */ 

#define ID_GDF_XML __GDF_XML
#define ID_GDF_THUMBNAIL __GDF_THUMBNAIL
#define ID_ICON_ICO __ICON_ICO
#define ID_GDF_XML_STR L"__GDF_XML"
#define ID_GDF_THUMBNAIL_STR L"__GDF_THUMBNAIL"
typedef /* [v1_enum] */ 
enum GAME_INSTALL_SCOPE
    {	GIS_NOT_INSTALLED	= 1,
	GIS_CURRENT_USER	= 2,
	GIS_ALL_USERS	= 3
    } 	GAME_INSTALL_SCOPE;



extern RPC_IF_HANDLE __MIDL_itf_gameux_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_gameux_0000_0000_v0_0_s_ifspec;

#ifndef __IGameExplorer_INTERFACE_DEFINED__
#define __IGameExplorer_INTERFACE_DEFINED__

/* interface IGameExplorer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGameExplorer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E7B2FB72-D728-49B3-A5F2-18EBF5F1349E")
    IGameExplorer : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddGame( 
            /* [in] */ __RPC__in BSTR bstrGDFBinaryPath,
            /* [in] */ __RPC__in BSTR bstrGameInstallDirectory,
            /* [in] */ GAME_INSTALL_SCOPE installScope,
            /* [out][in] */ __RPC__inout GUID *pguidInstanceID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RemoveGame( 
            /* [in] */ GUID guidInstanceID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE UpdateGame( 
            /* [in] */ GUID guidInstanceID) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE VerifyAccess( 
            /* [in] */ __RPC__in BSTR bstrGDFBinaryPath,
            /* [out] */ __RPC__out BOOL *pfHasAccess) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGameExplorerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGameExplorer * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGameExplorer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGameExplorer * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AddGame )( 
            IGameExplorer * This,
            /* [in] */ __RPC__in BSTR bstrGDFBinaryPath,
            /* [in] */ __RPC__in BSTR bstrGameInstallDirectory,
            /* [in] */ GAME_INSTALL_SCOPE installScope,
            /* [out][in] */ __RPC__inout GUID *pguidInstanceID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RemoveGame )( 
            IGameExplorer * This,
            /* [in] */ GUID guidInstanceID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *UpdateGame )( 
            IGameExplorer * This,
            /* [in] */ GUID guidInstanceID);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *VerifyAccess )( 
            IGameExplorer * This,
            /* [in] */ __RPC__in BSTR bstrGDFBinaryPath,
            /* [out] */ __RPC__out BOOL *pfHasAccess);
        
        END_INTERFACE
    } IGameExplorerVtbl;

    interface IGameExplorer
    {
        CONST_VTBL struct IGameExplorerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGameExplorer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IGameExplorer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IGameExplorer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IGameExplorer_AddGame(This,bstrGDFBinaryPath,bstrGameInstallDirectory,installScope,pguidInstanceID)	\
    ( (This)->lpVtbl -> AddGame(This,bstrGDFBinaryPath,bstrGameInstallDirectory,installScope,pguidInstanceID) ) 

#define IGameExplorer_RemoveGame(This,guidInstanceID)	\
    ( (This)->lpVtbl -> RemoveGame(This,guidInstanceID) ) 

#define IGameExplorer_UpdateGame(This,guidInstanceID)	\
    ( (This)->lpVtbl -> UpdateGame(This,guidInstanceID) ) 

#define IGameExplorer_VerifyAccess(This,bstrGDFBinaryPath,pfHasAccess)	\
    ( (This)->lpVtbl -> VerifyAccess(This,bstrGDFBinaryPath,pfHasAccess) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IGameExplorer_INTERFACE_DEFINED__ */



#ifndef __gameuxLib_LIBRARY_DEFINED__
#define __gameuxLib_LIBRARY_DEFINED__

/* library gameuxLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_gameuxLib;

EXTERN_C const CLSID CLSID_GameExplorer;

#ifdef __cplusplus

class DECLSPEC_UUID("9A5EA990-3034-4D6F-9128-01F3C61022BC")
GameExplorer;
#endif
#endif /* __gameuxLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


