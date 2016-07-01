

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Tue Feb 23 10:32:23 2016
 */
/* Compiler settings for CPPVideoControl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0595 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __CPPVideoControl_h__
#define __CPPVideoControl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICompositeControl_FWD_DEFINED__
#define __ICompositeControl_FWD_DEFINED__
typedef interface ICompositeControl ICompositeControl;

#endif 	/* __ICompositeControl_FWD_DEFINED__ */


#ifndef __CompositeControl_FWD_DEFINED__
#define __CompositeControl_FWD_DEFINED__

#ifdef __cplusplus
typedef class CompositeControl CompositeControl;
#else
typedef struct CompositeControl CompositeControl;
#endif /* __cplusplus */

#endif 	/* __CompositeControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ICompositeControl_INTERFACE_DEFINED__
#define __ICompositeControl_INTERFACE_DEFINED__

/* interface ICompositeControl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICompositeControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3E119535-D5AB-4520-B0E1-495B322E2A1A")
    ICompositeControl : public IDispatch
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct ICompositeControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICompositeControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICompositeControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICompositeControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICompositeControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICompositeControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICompositeControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICompositeControl * This,
            /* [annotation][in] */ 
            _In_  DISPID dispIdMember,
            /* [annotation][in] */ 
            _In_  REFIID riid,
            /* [annotation][in] */ 
            _In_  LCID lcid,
            /* [annotation][in] */ 
            _In_  WORD wFlags,
            /* [annotation][out][in] */ 
            _In_  DISPPARAMS *pDispParams,
            /* [annotation][out] */ 
            _Out_opt_  VARIANT *pVarResult,
            /* [annotation][out] */ 
            _Out_opt_  EXCEPINFO *pExcepInfo,
            /* [annotation][out] */ 
            _Out_opt_  UINT *puArgErr);
        
        END_INTERFACE
    } ICompositeControlVtbl;

    interface ICompositeControl
    {
        CONST_VTBL struct ICompositeControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICompositeControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICompositeControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICompositeControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICompositeControl_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ICompositeControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ICompositeControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ICompositeControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICompositeControl_INTERFACE_DEFINED__ */



#ifndef __CPPVIDEOCONTROLLib_LIBRARY_DEFINED__
#define __CPPVIDEOCONTROLLib_LIBRARY_DEFINED__

/* library CPPVIDEOCONTROLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CPPVIDEOCONTROLLib;

EXTERN_C const CLSID CLSID_CompositeControl;

#ifdef __cplusplus

class DECLSPEC_UUID("CDDFD429-EDFD-4C72-AE9C-B70FE6955051")
CompositeControl;
#endif
#endif /* __CPPVIDEOCONTROLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


