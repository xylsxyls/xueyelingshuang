

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Sep 01 17:51:40 2016
 */
/* Compiler settings for SetTipOcx.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
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


#ifndef __SetTipOcxidl_h__
#define __SetTipOcxidl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef ___DSetTipOcx_FWD_DEFINED__
#define ___DSetTipOcx_FWD_DEFINED__
typedef interface _DSetTipOcx _DSetTipOcx;
#endif 	/* ___DSetTipOcx_FWD_DEFINED__ */


#ifndef ___DSetTipOcxEvents_FWD_DEFINED__
#define ___DSetTipOcxEvents_FWD_DEFINED__
typedef interface _DSetTipOcxEvents _DSetTipOcxEvents;
#endif 	/* ___DSetTipOcxEvents_FWD_DEFINED__ */


#ifndef __SetTipOcx_FWD_DEFINED__
#define __SetTipOcx_FWD_DEFINED__

#ifdef __cplusplus
typedef class SetTipOcx SetTipOcx;
#else
typedef struct SetTipOcx SetTipOcx;
#endif /* __cplusplus */

#endif 	/* __SetTipOcx_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __SetTipOcxLib_LIBRARY_DEFINED__
#define __SetTipOcxLib_LIBRARY_DEFINED__

/* library SetTipOcxLib */
/* [control][version][uuid] */ 


EXTERN_C const IID LIBID_SetTipOcxLib;

#ifndef ___DSetTipOcx_DISPINTERFACE_DEFINED__
#define ___DSetTipOcx_DISPINTERFACE_DEFINED__

/* dispinterface _DSetTipOcx */
/* [uuid] */ 


EXTERN_C const IID DIID__DSetTipOcx;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("6C149F97-4F38-437D-8EE3-8734302A6F99")
    _DSetTipOcx : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DSetTipOcxVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DSetTipOcx * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DSetTipOcx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DSetTipOcx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DSetTipOcx * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DSetTipOcx * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DSetTipOcx * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DSetTipOcx * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DSetTipOcxVtbl;

    interface _DSetTipOcx
    {
        CONST_VTBL struct _DSetTipOcxVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DSetTipOcx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DSetTipOcx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DSetTipOcx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DSetTipOcx_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DSetTipOcx_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DSetTipOcx_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DSetTipOcx_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DSetTipOcx_DISPINTERFACE_DEFINED__ */


#ifndef ___DSetTipOcxEvents_DISPINTERFACE_DEFINED__
#define ___DSetTipOcxEvents_DISPINTERFACE_DEFINED__

/* dispinterface _DSetTipOcxEvents */
/* [uuid] */ 


EXTERN_C const IID DIID__DSetTipOcxEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("D80C0124-0300-4DC4-9D1C-55640235FFB4")
    _DSetTipOcxEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DSetTipOcxEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DSetTipOcxEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DSetTipOcxEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DSetTipOcxEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DSetTipOcxEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DSetTipOcxEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DSetTipOcxEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DSetTipOcxEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DSetTipOcxEventsVtbl;

    interface _DSetTipOcxEvents
    {
        CONST_VTBL struct _DSetTipOcxEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DSetTipOcxEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DSetTipOcxEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DSetTipOcxEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DSetTipOcxEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DSetTipOcxEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DSetTipOcxEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DSetTipOcxEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DSetTipOcxEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SetTipOcx;

#ifdef __cplusplus

class DECLSPEC_UUID("93BA3E69-5E03-4AEA-AC5B-99FC4E02B12F")
SetTipOcx;
#endif
#endif /* __SetTipOcxLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


