//----------------------------------------------------------------------------
//  File:   Utils.h
//
//  Desc:   DirectShow sample code
//          External (global) utilities specific for VMRXcl app
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//----------------------------------------------------------------------------

#include "project.h"
#include "utils.h"
#include "vmrutil.h"

//-----------------------------------------------------------------------------------------
//  Function:   hresultNameLookup
//  Purpose:    Returns a string value for the given hresult
//  Arguments:  HRESULT that needs verifying
//  Returns:    string
//-----------------------------------------------------------------------------------------*/
const TCHAR * hresultNameLookup(HRESULT hres)
{
    switch(hres)
    {
        case VFW_E_CANNOT_RENDER:
            return _T("VFW_E_CANNOT_RENDER\0");
            break;
        case VFW_E_INVALID_FILE_FORMAT:
            return _T("VFW_E_INVALID_FILE_FORMAT\0");
            break;
        case VFW_E_NOT_FOUND:
            return _T("VFW_E_NOT_FOUND\0");
            break;
        case VFW_E_NOT_IN_GRAPH:
            return _T("VFW_E_NOT_IN_GRAPH\0");
            break;
        case VFW_E_UNKNOWN_FILE_TYPE:
            return _T("VFW_E_UNKNOWN_FILE_TYPE\0");
            break;
        case VFW_E_UNSUPPORTED_STREAM:
            return _T("VFW_E_UNSUPPORTED_STREAM\0");
            break;
        case VFW_E_CANNOT_CONNECT:
            return _T("VFW_E_CANNOT_CONNECT\0");
            break;
        case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
            return _T("VFW_E_CANNOT_LOAD_SOURCE_FILTER\0");
            break;
        case VFW_S_PARTIAL_RENDER:
            return _T("VFW_S_PARTIAL_RENDER\0");
            break;
        case VFW_S_VIDEO_NOT_RENDERED:
            return _T("VFW_S_VIDEO_NOT_RENDERED\0");
            break;
        case VFW_S_AUDIO_NOT_RENDERED:
            return _T("VFW_S_AUDIO_NOT_RENDERED\0");
            break;
        case VFW_S_DUPLICATE_NAME:
            return _T("VFW_S_DUPLICATE_NAME\0");
            break;
        case VFW_S_MEDIA_TYPE_IGNORED:
            return _T("VFW_S_MEDIA_TYPE_IGNORED\0");
            break;
        case VFW_E_NO_DISPLAY_PALETTE:
            return _T("VFW_E_NO_DISPLAY_PALETTE\0");
            break;
        case VFW_E_NO_COLOR_KEY_FOUND:
            return _T("VFW_E_NO_COLOR_KEY_FOUND\0");
            break;
        case VFW_E_PALETTE_SET:
            return _T("VFW_E_PALETTE_SET\0");
            break;
        case VFW_E_BUFFERS_OUTSTANDING:
            return _T("VFW_E_BUFFERS_OUTSTANDING\0");
            break;
        case VFW_E_NO_ADVISE_SET:
            return _T("VFW_E_NO_ADVISE_SET\0");
            break;

        case DDERR_INCOMPATIBLEPRIMARY:
            return _T("DDERR_INCOMPATIBLEPRIMARY\0");
            break;
        case DDERR_INVALIDCAPS:
            return _T("DDERR_INVALIDCAPS\0");
            break;
        case DDERR_INVALIDOBJECT :
            return _T("DDERR_INVALIDOBJECT\0");
            break;
        case DDERR_INVALIDPIXELFORMAT:
            return _T("DDERR_INVALIDPIXELFORMAT\0");
            break;
        case DDERR_NOALPHAHW :
            return _T("DDERR_NOALPHAHW\0");
            break;
        case DDERR_NOCOOPERATIVELEVELSET :
            return _T("DDERR_NOCOOPERATIVELEVELSET\0");
            break;
        case DDERR_NODIRECTDRAWHW :
            return _T("DDERR_NODIRECTDRAWHW\0");
            break;
        case DDERR_NOEMULATION :
            return _T("DDERR_NOEMULATION\0");
            break;
        case DDERR_NOEXCLUSIVEMODE :
            return _T("DDERR_NOEXCLUSIVEMODE \0");
            break;
        case DDERR_NOFLIPHW:
            return _T("DDERR_NOFLIPHW\0");
            break;
        case DDERR_NOMIPMAPHW:
            return _T("DDERR_NOMIPMAPHW\0");
            break;
        case DDERR_NOOVERLAYHW :
            return _T("DDERR_NOOVERLAYHW \0");
            break;
        case E_OUTOFMEMORY:
            return _T("E_OUTOFMEMORY\0");
            break;
        case DDERR_NOZBUFFERHW :
            return _T("DDERR_NOZBUFFERHW \0");
            break;
        case DDERR_OUTOFVIDEOMEMORY :
            return _T("DDERR_OUTOFVIDEOMEMORY\0");
            break;
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            return _T("DDERR_PRIMARYSURFACEALREADYEXISTS \0");
            break;
        case DDERR_UNSUPPORTEDMODE:
            return _T("DDERR_UNSUPPORTEDMODE\0");
            break;

        case S_OK:
            return _T("S_OK\0");
            break;
        case S_FALSE:
            return _T("S_FALSE\0");
            break;
        case E_FAIL:
            return _T("E_FAIL\0");
            break;
        case E_INVALIDARG:
            return _T("E_INVALIDARG\0");
            break;
        case E_NOTIMPL:
            return _T("E_NOTIMPL\0");
            break;
        case E_NOINTERFACE:
            return _T("E_NOINTERFACE\0");
            break;
        case E_POINTER:
            return _T("E_POINTER\0");
            break;
        case E_UNEXPECTED:
            return _T("E_UNEXPECTED\0");
            break;
        case E_PROP_SET_UNSUPPORTED:
            return _T("E_PROP_SET_UNSUPPORTED\0");
            break;

        case VFW_S_CONNECTIONS_DEFERRED:
            return _T("VFW_S_CONNECTIONS_DEFERRED\0");
            break;

        case 0x80040154:
            return _T("Class not registered\0");
            break;
        case VFW_E_DVD_OPERATION_INHIBITED:
            return _T("VFW_E_DVD_OPERATION_INHIBITED\0");
            break;
        case VFW_E_DVD_INVALIDDOMAIN:
            return _T("VFW_E_DVD_INVALIDDOMAIN\0");
            break;
        case VFW_E_WRONG_STATE:
            return _T("VFW_E_WRONG_STATE\0");
            break;
        case VFW_E_NO_PALETTE_AVAILABLE:
            return _T("VFW_E_NO_PALETTE_AVAILABLE\0");
            break;
        case VFW_E_DVD_NO_BUTTON:
            return _T("VFW_E_DVD_NO_BUTTON\0");
            break;
        case VFW_E_DVD_GRAPHNOTREADY:
            return _T("VFW_E_DVD_GRAPHNOTREADY\0");
            break;
        case VFW_E_NOT_OVERLAY_CONNECTION:
            return _T("VFW_E_NOT_OVERLAY_CONNECTION\0");
            break;
        case VFW_E_DVD_RENDERFAIL:
            return _T("VFW_E_DVD_RENDERFAIL\0");
            break;
        case VFW_E_NOT_CONNECTED:
            return _T("VFW_E_NOT_CONNECTED\0");
            break;
        case VFW_E_NO_COLOR_KEY_SET :
            return _T("VFW_E_NO_COLOR_KEY_SET \0");
            break;
        case VFW_E_NO_INTERFACE:
            return _T("VFW_E_NO_INTERFACE\0");
            break;

        case 0x8004020c:
            return _T("VFW_E_BUFFER_NOTSET\0");
            break;
        case 0x80040225:
            return _T("VFW_E_NOT_PAUSED\0");
            break;
        case 0x80070002:
            return _T("System cannot find the file specified\0");
            break;
        case 0x80070003:
            return _T("System cannot find the path specified\0");
            break;

        case VFW_E_DVD_DECNOTENOUGH:
            return _T("VFW_E_DVD_DECNOTENOUGH\0");
            break;
        case VFW_E_ADVISE_ALREADY_SET:
            return _T("VFW_E_ADVISE_ALREADY_SET\0");
            break;
        case VFW_E_DVD_CMD_CANCELLED:
            return _T("VFW_E_DVD_CMD_CANCELLED\0");
            break;
        case VFW_E_DVD_MENU_DOES_NOT_EXIST:
            return _T("VFW_E_DVD_MENU_DOES_NOT_EXIST\0");
            break;
        case VFW_E_DVD_WRONG_SPEED:
            return _T("VFW_E_DVD_WRONG_SPEED\0");
            break;
        case VFW_S_DVD_NON_ONE_SEQUENTIAL:
            return _T("VFW_S_DVD_NON_ONE_SEQUENTIAL\0");
            break;
        case VFW_E_DVD_NOT_IN_KARAOKE_MODE:
            return _T("VFW_E_DVD_NOT_IN_KARAOKE_MODE\0");
            break;
        case VFW_E_DVD_INVALID_DISC:
            return _T("VFW_E_DVD_INVALID_DISC\0");
            break;
        case VFW_E_DVD_STREAM_DISABLED:
            return _T("VFW_E_DVD_STREAM_DISABLED\0");
            break;
        case VFW_E_NOT_STOPPED:
            return _T("VFW_E_NOT_STOPPED\0");
            break;

        default:
            return _T("Unrecognized\0");
            break;
    }
}


//-----------------------------------------------------------------------------------------
//  Function:   MySleep
//  Purpose:    If the application is in automated mode, then sleep func is turned off
//  Arguments:  Checks m_bAutomatedStatus to see if the func is in automation
//  Returns:    true if automated, false otherwist
//-----------------------------------------------------------------------------------------*/
bool MySleep(DWORD  dwTime)
{
    HANDLE hNeverHappensEvent;

    hNeverHappensEvent = CreateEvent(NULL, FALSE, FALSE, _T("EVENTTHATNEVERHAPPENS\0"));
    if (NULL != hNeverHappensEvent)
    {
        WaitForSingleObject( hNeverHappensEvent, dwTime);
        CloseHandle(hNeverHappensEvent);
    }
    return false;

} // end of checkHResult method



void ReportDDrawSurfDesc( DDSURFACEDESC2 ddsd)
{
    TCHAR szFlags[4096];
    TCHAR szMsg[4096];

    OutputDebugString(_T("*** Surface description ***\n"));

    SurfaceDescHelper( ddsd.dwFlags, szFlags, NUMELMS(szFlags));
    OutputDebugString(szFlags);

    HRESULT hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("  dwWidth x dwHeight: %ld x %ld\n"), ddsd.dwWidth, ddsd.dwHeight);
    OutputDebugString(szMsg);

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("  lPitch: %ld\n"), ddsd.lPitch);
    OutputDebugString(szMsg);
    OutputDebugString(_T("  (dwLinearSize)\n"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("  dwBackBufferCount: %ld\n"), ddsd.dwBackBufferCount);
    OutputDebugString(szMsg);

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("  dwMipMapCount: %ld\n"), ddsd.dwMipMapCount);
    OutputDebugString(szMsg);
    OutputDebugString(_T("  (dwRefreshRate)"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg),  _T("  dwAlphaBitDepth: %ld\n"), (LONG)ddsd.dwAlphaBitDepth);
    OutputDebugString(szMsg);

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("  lpSurface: %x\n"), (LONG_PTR)(ddsd.lpSurface));
    OutputDebugString(szMsg);

    ReportPixelFormat( ddsd.ddpfPixelFormat );
    ReportDDSCAPS2( ddsd.ddsCaps );

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg),  _T("  dwTextureStage: %ld\n"), ddsd.dwTextureStage);
    OutputDebugString(szMsg);

    OutputDebugString(_T("***************************\n"));
}


void ReportDDSCAPS2( DDSCAPS2 ddscaps )
{
    TCHAR sz[4096];

    HRESULT hr = StringCchCopy( sz, NUMELMS(sz), _T("  DDSCAPS2::dwCaps: "));
    if( ddscaps.dwCaps & DDSCAPS_3DDEVICE )     hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_3DDEVICE, "));
    if( ddscaps.dwCaps & DDSCAPS_ALLOCONLOAD )  hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_ALLOCONLOAD, "));
    if( ddscaps.dwCaps & DDSCAPS_ALPHA )        hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_ALPHA, "));
    if( ddscaps.dwCaps & DDSCAPS_BACKBUFFER )   hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_BACKBUFFER, "));
    if( ddscaps.dwCaps & DDSCAPS_COMPLEX )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_COMPLEX, "));
    if( ddscaps.dwCaps & DDSCAPS_FLIP )         hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_FLIP, "));
    if( ddscaps.dwCaps & DDSCAPS_FRONTBUFFER )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_FRONTBUFFER, "));
    if( ddscaps.dwCaps & DDSCAPS_HWCODEC )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_HWCODEC, "));
    if( ddscaps.dwCaps & DDSCAPS_LIVEVIDEO )    hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_LIVEVIDEO, "));
    if( ddscaps.dwCaps & DDSCAPS_LOCALVIDMEM )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_LOCALVIDMEM, "));
    if( ddscaps.dwCaps & DDSCAPS_MIPMAP )       hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_MIPMAP, "));
    if( ddscaps.dwCaps & DDSCAPS_MODEX )        hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_MODEX, "));
    if( ddscaps.dwCaps & DDSCAPS_NONLOCALVIDMEM ) hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_NONLOCALVIDMEM, "));
    if( ddscaps.dwCaps & DDSCAPS_OFFSCREENPLAIN ) hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_OFFSCREENPLAIN, "));
    if( ddscaps.dwCaps & DDSCAPS_OPTIMIZED )    hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_OPTIMIZED, "));
    if( ddscaps.dwCaps & DDSCAPS_OVERLAY )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_OVERLAY, "));
    if( ddscaps.dwCaps & DDSCAPS_OWNDC )        hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_OWNDC, "));
    if( ddscaps.dwCaps & DDSCAPS_PALETTE )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_PALETTE, "));
    if( ddscaps.dwCaps & DDSCAPS_PRIMARYSURFACE )  hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_PRIMARYSURFACE, "));
    if( ddscaps.dwCaps & DDSCAPS_STANDARDVGAMODE ) hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_STANDARDVGAMODE, "));
    if( ddscaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_SYSTEMMEMORY, "));
    if( ddscaps.dwCaps & DDSCAPS_TEXTURE )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_TEXTURE, "));
    if( ddscaps.dwCaps & DDSCAPS_VIDEOMEMORY )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_VIDEOMEMORY, "));
    if( ddscaps.dwCaps & DDSCAPS_VIDEOPORT )    hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_VIDEOPORT, "));
    if( ddscaps.dwCaps & DDSCAPS_VISIBLE )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_VISIBLE, "));
    if( ddscaps.dwCaps & DDSCAPS_WRITEONLY )    hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS_WRITEONLY, "));
    if( ddscaps.dwCaps & DDSCAPS_ZBUFFER )      hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS_ZBUFFER, "));

    hr = StringCchCat( sz, NUMELMS(sz), _T("\n"));
    OutputDebugString(sz);
    hr = StringCchCopy( sz, NUMELMS(sz),  _T("  DDSCAPS2::dwCaps2: "));

    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP )            hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS2_CUBEMAP, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEX )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_POSITIVEX, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEX )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_NEGATIVEX, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEY )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_POSITIVEY, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEY )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_NEGATIVEY, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_POSITIVEZ )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_POSITIVEZ, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_NEGATIVEZ, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES )   hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_CUBEMAP_ALLFACES, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_D3DTEXTUREMANAGE )   hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_D3DTEXTUREMANAGE, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_DONOTPERSIST )       hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_DONOTPERSIST, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_HARDWAREDEINTERLACE) hr = StringCchCat( sz, NUMELMS(sz),_T("DDSCAPS2_HARDWAREDEINTERLACE, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_HINTANTIALIASING )   hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_HINTANTIALIASING, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_HINTDYNAMIC )        hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_HINTDYNAMIC, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_HINTSTATIC )         hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_HINTSTATIC, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL )     hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_MIPMAPSUBLEVEL, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_OPAQUE )             hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_OPAQUE, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_STEREOSURFACELEFT )  hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_STEREOSURFACELEFT, "));
    if( ddscaps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE )      hr = StringCchCat( sz, NUMELMS(sz), _T("DDSCAPS2_TEXTUREMANAGE, "));

    hr = StringCchCat( sz, NUMELMS(sz), _T("\n"));
    OutputDebugString(sz);
}


void ReportPixelFormat( DDPIXELFORMAT ddpf)
{
    TCHAR szFlags[4096];
    TCHAR szMsg[MAX_PATH];

    PixelFormatHelper( ddpf.dwFlags, szFlags, NUMELMS(szFlags));

    OutputDebugString(szFlags);

    HRESULT hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwFourCC: %ld\n"), ddpf.dwFourCC);
    OutputDebugString(szMsg);

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwRGBBitCount: %ld\n"), ddpf.dwRGBBitCount);
    OutputDebugString(szMsg);
    OutputDebugString(_T("    (dwYUVBitCount, dwZBufferBitDepth, dwAlphaBitDepth, dwLuminanceBitCount, dwBumpBitCount)\n"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwRBitMask: %ld\n"), ddpf.dwRBitMask);
    OutputDebugString(szMsg);
    OutputDebugString(_T("    (dwYBitMask, dwStencilBitDepth, dwLuminanceBitMask, dwBumpDuBitMask)\n"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwGBitMask: %ld\n"), ddpf.dwGBitMask);
    OutputDebugString(szMsg);
    OutputDebugString(_T("    (dwUBitMask, dwZBitMask, dwBumpDvBitMask)\n"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwBBitMask: %ld\n"), ddpf.dwBBitMask);
    OutputDebugString(szMsg);
    OutputDebugString(_T("    (dwVBitMask, dwStencilBitMask, dwBumpLuminanceBitMask)\n"));

    hr = StringCchPrintf( szMsg, NUMELMS(szMsg), _T("    dwRGBAlphaBitMask: %ld\n"), ddpf.dwRGBAlphaBitMask);
    OutputDebugString(szMsg);
    OutputDebugString(_T("    (dwYUVAlphaBitMask, dwLuminanceAlphaBitMask, dwRGBZBitMask, dwYUVZBitMask)\n"));
}


void SurfaceDescHelper( DWORD dwFlags, TCHAR * pszFlags, size_t len )
{
    if( !pszFlags )
        return;

    HRESULT hr = StringCchCopy( pszFlags, len, _T("  dwFlags: "));

    if( dwFlags & DDSD_ALL )
        hr = StringCchCat( pszFlags, len, _T("DDSD_ALL\n"));

    if( dwFlags & DDSD_ALPHABITDEPTH  )
        hr = StringCchCat( pszFlags, len,_T("DDSD_ALPHABITDEPTH\n"));

    if( dwFlags & DDSD_BACKBUFFERCOUNT  )
        hr = StringCchCat( pszFlags, len, _T("DDSD_BACKBUFFERCOUNT\n"));

    if( dwFlags & DDSD_CAPS  )
        hr = StringCchCat( pszFlags, len, _T("DDSD_CAPS\n"));

    if( dwFlags & DDSD_CKDESTBLT   )
        hr = StringCchCat( pszFlags, len, _T("DDSD_CKDESTBLT\n"));

    if( dwFlags & DDSD_CKDESTOVERLAY   )
        hr = StringCchCat( pszFlags, len, _T("DDSD_CKDESTOVERLAY\n"));

    if( dwFlags & DDSD_CKSRCBLT   )
        hr = StringCchCat( pszFlags, len, _T("DDSD_CKSRCBLT\n"));

    if( dwFlags & DDSD_CKSRCOVERLAY    )
        hr = StringCchCat( pszFlags, len, _T("DDSD_CKSRCOVERLAY\n"));

    if( dwFlags & DDSD_HEIGHT    )
        hr = StringCchCat( pszFlags, len, _T("DDSD_HEIGHT\n"));

    if( dwFlags & DDSD_LINEARSIZE    )
        hr = StringCchCat( pszFlags, len, _T("DDSD_LINEARSIZE\n"));

    if( dwFlags & DDSD_LPSURFACE    )
        hr = StringCchCat( pszFlags, len, _T("DDSD_LPSURFACE\n"));

    if( dwFlags & DDSD_MIPMAPCOUNT     )
        hr = StringCchCat( pszFlags, len, _T("DDSD_MIPMAPCOUNT\n"));

    if( dwFlags & DDSD_PITCH     )
        hr = StringCchCat( pszFlags, len, _T("DDSD_PITCH\n"));

    if( dwFlags & DDSD_PIXELFORMAT     )
        hr = StringCchCat( pszFlags, len, _T("DDSD_PIXELFORMAT\n"));

    if( dwFlags & DDSD_REFRESHRATE     )
        hr = StringCchCat( pszFlags, len, _T("DDSD_REFRESHRATE\n"));

    if( dwFlags & DDSD_TEXTURESTAGE      )
        hr = StringCchCat( pszFlags, len, _T("DDSD_TEXTURESTAGE\n"));

    if( dwFlags & DDSD_WIDTH      )
        hr = StringCchCat( pszFlags, len, _T("DDSD_WIDTH\n"));

    hr = StringCchCat( pszFlags, len, _T("\n"));
}


void PixelFormatHelper( DWORD dwFlags, TCHAR * pszFlags, size_t len)
{
    if( !pszFlags )
        return;

    HRESULT hr = StringCchCopy( pszFlags, len,  _T("    dwFlags: "));

    if( dwFlags & DDPF_ALPHA )
        hr = StringCchCat( pszFlags, len,_T("DDPF_ALPHA, "));

    if( dwFlags & DDPF_ALPHAPIXELS  )
        hr = StringCchCat( pszFlags, len, _T("DDPF_ALPHAPIXELS, "));

    if( dwFlags & DDPF_ALPHAPREMULT  )
        hr = StringCchCat( pszFlags, len, _T("DDPF_ALPHAPREMULT, "));

    if( dwFlags & DDPF_BUMPLUMINANCE  )
        hr = StringCchCat( pszFlags, len, _T("DDPF_BUMPLUMINANCE, "));

    if( dwFlags & DDPF_BUMPDUDV   )
        hr = StringCchCat( pszFlags, len, _T("DDPF_BUMPDUDV, "));

    if( dwFlags & DDPF_COMPRESSED   )
        hr = StringCchCat( pszFlags, len, _T("DDPF_COMPRESSED, "));

    if( dwFlags & DDPF_FOURCC   )
        hr = StringCchCat( pszFlags, len, _T("DDPF_FOURCC, "));

    if( dwFlags & DDPF_LUMINANCE    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_LUMINANCE, "));

    if( dwFlags & DDPF_PALETTEINDEXED1    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_PALETTEINDEXED1, "));

    if( dwFlags & DDPF_PALETTEINDEXED2    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_PALETTEINDEXED2, "));

    if( dwFlags & DDPF_PALETTEINDEXED4    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_PALETTEINDEXED4, "));

    if( dwFlags & DDPF_PALETTEINDEXED8    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_PALETTEINDEXED8, "));

    if( dwFlags & DDPF_PALETTEINDEXEDTO8  )
        hr = StringCchCat( pszFlags, len, _T("DDPF_PALETTEINDEXEDTO8, "));

    if( dwFlags & DDPF_RGB   )
        hr = StringCchCat( pszFlags, len, _T("DDPF_RGB, "));

    if( dwFlags & DDPF_RGBTOYUV    )
        hr = StringCchCat( pszFlags, len, _T("DDPF_RGBTOYUV, "));

    if( dwFlags & DDPF_STENCILBUFFER     )
        hr = StringCchCat( pszFlags, len, _T("DDPF_STENCILBUFFER, "));

    if( dwFlags & DDPF_YUV     )
        hr = StringCchCat( pszFlags, len, _T("DDPF_YUV, "));

    if( dwFlags & DDPF_ZBUFFER     )
        hr = StringCchCat( pszFlags, len, _T("DDPF_ZBUFFER, "));

    if( dwFlags & DDPF_ZPIXELS     )
        hr = StringCchCat( pszFlags, len, _T("DDPF_ZPIXELS, "));

    hr = StringCchCat( pszFlags, len, _T("\n"));
}


