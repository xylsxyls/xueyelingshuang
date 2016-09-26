

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_SetTipOcxLib,0x2EA857A6,0x6F8A,0x45A4,0xB7,0xF1,0x12,0x37,0x94,0x89,0x50,0xFC);


MIDL_DEFINE_GUID(IID, DIID__DSetTipOcx,0x6C149F97,0x4F38,0x437D,0x8E,0xE3,0x87,0x34,0x30,0x2A,0x6F,0x99);


MIDL_DEFINE_GUID(IID, DIID__DSetTipOcxEvents,0xD80C0124,0x0300,0x4DC4,0x9D,0x1C,0x55,0x64,0x02,0x35,0xFF,0xB4);


MIDL_DEFINE_GUID(CLSID, CLSID_SetTipOcx,0x93BA3E69,0x5E03,0x4AEA,0xAC,0x5B,0x99,0xFC,0x4E,0x02,0xB1,0x2F);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



