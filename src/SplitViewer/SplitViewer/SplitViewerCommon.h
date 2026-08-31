#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_IE 0x0600
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objidl.h>
#define SPLITVIEWER_GDIPLUS_MINMAX
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#include <gdiplus.h>
#ifdef SPLITVIEWER_GDIPLUS_MINMAX
#undef min
#undef max
#undef SPLITVIEWER_GDIPLUS_MINMAX
#endif
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace SplitViewer
{
    struct TempFileGuard;
    const wchar_t kWindowClass[] = L"SplitViewerWindow";
    const wchar_t kDragFrameClass[] = L"SplitViewerDragFrame";
    const wchar_t kAppTitle[] = L"Split Viewer";

    const int kToolbarHeight = 42;
    const int kMargin = 10;
    const int kButtonHeight = 26;
    const int kButtonGap = 8;
    const int kSplitterSize = 2;
    const int kSplitterHoverSize = 4;
    const int kSplitterHitPadding = 5;
    const int kPlusButtonSize = 42;
    const int kMinPaneSize = 48;
    const int kMinLayerWidth = 90;
    const int kMinLayerHeight = 70;
    const int kLayerHitSize = 7;
    const int kConfigThumbnailMaxEdge = 320;
    const int kExternalDropMoveThreshold = 8;
    const UINT_PTR TIMER_TIP = 1;
    const UINT WM_APP_EMBED_EXTERNAL_WINDOW = WM_APP + 101;
    const UINT WM_APP_TOGGLE_FULLSCREEN = WM_APP + 102;
    const char kConfigMarker[] = "\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n";
    const BYTE kPngSignature[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    const BYTE kPngIendChunkType[] = { 'I', 'E', 'N', 'D' };
    const BYTE kPngConfigChunkType[] = { 's', 'v', 'C', 'f' };

    const COLORREF kWindowBackColor = RGB(238, 238, 238);
    const COLORREF kToolbarColor = RGB(246, 246, 246);
    const COLORREF kGrayColor = RGB(128, 128, 128);
    const COLORREF kSplitterColor = RGB(188, 188, 188);
    const COLORREF kSplitterHoverColor = RGB(214, 214, 214);
    const COLORREF kFrameColor = RGB(32, 32, 32);
    const COLORREF kSelectedFrameColor = RGB(255, 0, 0);
    const COLORREF kExternalDropFrameColor = RGB(0, 170, 230);
    const COLORREF kExternalDragOutFrameColor = RGB(255, 0, 0);
    const COLORREF kFrameTransparentColor = RGB(255, 0, 255);

    enum ControlId
    {
        IDC_SAVE_IMAGE = 1001,
        IDC_SAVE_CONFIG,
        IDC_NEW_LAYER,
        IDC_FULLSCREEN
    };

    enum MenuId
    {
        IDM_SPLIT_HORIZONTAL = 40001,
        IDM_SPLIT_VERTICAL,
        IDM_DELETE_SPLIT
    };

    enum NodeKind
    {
        NODE_LEAF = 0,
        NODE_SPLIT = 1
    };

    enum SplitDirection
    {
        SPLIT_HORIZONTAL = 0,
        SPLIT_VERTICAL = 1
    };

    enum DragMode
    {
        DRAG_NONE = 0,
        DRAG_IMAGE,
        DRAG_SPLITTER,
        DRAG_LAYER_MOVE,
        DRAG_LAYER_RESIZE
    };

    enum ResizeHit
    {
        RESIZE_NONE = 0,
        RESIZE_LEFT = 1,
        RESIZE_RIGHT = 2,
        RESIZE_TOP = 4,
        RESIZE_BOTTOM = 8
    };
    int RectWidth(const RECT& rc);

    int RectHeight(const RECT& rc);

    RECT MakeRect(int left, int top, int right, int bottom);

    bool IsRectUsable(const RECT& rc);

    bool AreRectsEqual(const RECT& left, const RECT& right);

    bool PointInRect(const RECT& rc, POINT pt);

    RECT InflatedRect(RECT rc, int dx, int dy);

    double ClampDouble(double value, double low, double high);

    int ClampInt(int value, int low, int high);

    std::wstring FormatInt(int value);

    std::wstring FormatDouble(double value);

    bool SameText(const wchar_t* left, const wchar_t* right);

    bool HasExtension(const std::wstring& path);

    std::wstring MakeTimestampName();

    std::wstring WithDefaultExtension(const std::wstring& path, const wchar_t* ext);

    bool IsConfigPath(const wchar_t* path);

    int GetEncoderClsid(const WCHAR* format, CLSID* clsid);

    const WCHAR* ImageMimeForPath(const std::wstring& path);

    std::wstring ProfileSectionName(const std::wstring& prefix, int id);

    void WriteProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const std::wstring& value);

    void WriteProfileInt(const std::wstring& file, const std::wstring& section, const wchar_t* key, int value);

    void WriteProfileDouble(const std::wstring& file, const std::wstring& section, const wchar_t* key, double value);

    std::wstring ReadProfileString(const std::wstring& file, const std::wstring& section, const wchar_t* key, const wchar_t* defaultValue);

    int ReadProfileIntValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, int defaultValue);

    double ReadProfileDoubleValue(const std::wstring& file, const std::wstring& section, const wchar_t* key, double defaultValue);

    bool CreateUtf16ProfileFile(const std::wstring& file);

    bool ReadFileBytes(const std::wstring& file, std::vector<BYTE>& bytes);

    bool WriteFileBytes(const std::wstring& file, const BYTE* bytes, size_t size);

    bool CreateTempProfilePath(std::wstring& path);

    bool StartsWithPng(const std::vector<BYTE>& bytes);

    bool FindBytes(const std::vector<BYTE>& bytes, const BYTE* pattern, size_t patternSize, size_t& index);

    DWORD ReadPngUInt32(const std::vector<BYTE>& bytes, size_t index);

    void AppendPngUInt32(std::vector<BYTE>& bytes, DWORD value);

    DWORD UpdatePngCrc32(DWORD crc, const BYTE* bytes, size_t size);

    DWORD PngChunkCrc32(const BYTE* chunkType, const BYTE* data, size_t dataSize);

    bool AppendPngChunk(std::vector<BYTE>& bytes, const BYTE* chunkType, const std::vector<BYTE>& data);

    bool BuildPngConfigPackage(const std::vector<BYTE>& thumbnailPng,
                                   const std::vector<BYTE>& configBytes,
                                   std::vector<BYTE>& packageBytes);

    bool ExtractPngChunkConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

    bool ExtractTrailingConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

    bool ExtractEmbeddedConfig(const std::vector<BYTE>& bytes, std::vector<BYTE>& configBytes);

    bool WriteConfigPackage(const std::wstring& file,
                                const std::vector<BYTE>& thumbnailPng,
                                const std::vector<BYTE>& configBytes);

    bool PrepareProfileReadFile(const std::wstring& sourcePath,
                                    std::wstring& profilePath,
                                    TempFileGuard& tempProfile);

    bool SetRegStringValue(HKEY root, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& value);

    bool IsVerifyExecutable(const std::wstring& exePath);

    void RegisterSvFileType();
}