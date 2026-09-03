#include "PdfReaderHelper.h"

#include "PdfReaderConstants.h"

#include <CStringManager/CStringManagerAPI.h>

#include <cstdlib>

COLORREF PdfReaderHelper::Color(int red, int green, int blue)
{
    return RGB(red, green, blue);
}

int PdfReaderHelper::ClampInt(int value, int minimum, int maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

RECT PdfReaderHelper::MakeRect(int left, int top, int right, int bottom)
{
    RECT rect = {left, top, right, bottom};
    return rect;
}

int PdfReaderHelper::RectWidth(const RECT& rect)
{
    return rect.right - rect.left;
}

int PdfReaderHelper::RectHeight(const RECT& rect)
{
    return rect.bottom - rect.top;
}

bool PdfReaderHelper::PtInRectLocal(const RECT& rect, POINT point)
{
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

void PdfReaderHelper::FillSolidRect(HDC hdc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void PdfReaderHelper::DrawSolidFrame(HDC hdc, const RECT& rect, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

int PdfReaderHelper::WheelSteps(WPARAM wParam)
{
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta == 0)
    {
        return 0;
    }
    if (abs(delta) < WHEEL_DELTA)
    {
        return delta > 0 ? 1 : -1;
    }
    return delta / WHEEL_DELTA;
}

int PdfReaderHelper::ScrollFromCode(HWND hwnd, int bar, WPARAM wParam, int current)
{
    SCROLLINFO info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(hwnd, bar, &info);

    int position = current;
    switch (LOWORD(wParam))
    {
    case SB_LINEUP:
        position -= kScrollStep;
        break;
    case SB_LINEDOWN:
        position += kScrollStep;
        break;
    case SB_PAGEUP:
        position -= static_cast<int>(info.nPage);
        break;
    case SB_PAGEDOWN:
        position += static_cast<int>(info.nPage);
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        position = info.nTrackPos;
        break;
    default:
        break;
    }
    return position;
}

RECT PdfReaderHelper::GetPrimaryWorkArea()
{
    RECT workArea;
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0))
    {
        workArea = MakeRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
    return workArea;
}

bool PdfReaderHelper::GetWindowMonitorRects(HWND hwnd, RECT* monitorRect, RECT* workRect)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr)
    {
        return false;
    }

    MONITORINFO monitorInfo;
    ZeroMemory(&monitorInfo, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo))
    {
        return false;
    }

    if (monitorRect != nullptr)
    {
        *monitorRect = monitorInfo.rcMonitor;
    }
    if (workRect != nullptr)
    {
        *workRect = monitorInfo.rcWork;
    }
    return true;
}

RECT PdfReaderHelper::GetDefaultMainWindowRect(int contentTop,
                                               int splitterWidth,
                                               int minLeftWidth,
                                               int minRightWidth,
                                               int minWindowWidth,
                                               int minWindowHeight,
                                               int* leftWidth)
{
    RECT workArea = GetPrimaryWorkArea();
    int workWidth = std::max(1, RectWidth(workArea));
    int workHeight = std::max(1, RectHeight(workArea));
    int documentWidth = std::max(minRightWidth, workWidth / 2);
    int documentHeight = std::max(1, workHeight / 2);
    int sidebarWidth = std::max(minLeftWidth, documentWidth / 4);
    int windowWidth = std::max(minWindowWidth, sidebarWidth + splitterWidth + documentWidth);
    int windowHeight = std::max(minWindowHeight, contentTop + documentHeight);
    windowWidth = std::min(windowWidth, workWidth);
    windowHeight = std::min(windowHeight, workHeight);

    if (leftWidth != nullptr)
    {
        int maxLeftWidth = std::max(minLeftWidth, windowWidth - minRightWidth - splitterWidth);
        *leftWidth = ClampInt(sidebarWidth, minLeftWidth, maxLeftWidth);
    }

    int left = workArea.left + (workWidth - windowWidth) / 2;
    int top = workArea.top + (workHeight - windowHeight) / 2;
    return MakeRect(left, top, left + windowWidth, top + windowHeight);
}

void PdfReaderHelper::RefreshWindowFrame(HWND hwnd)
{
    if (hwnd == nullptr)
    {
        return;
    }

    SetWindowPos(hwnd,
                 nullptr,
                 0,
                 0,
                 0,
                 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RedrawWindow(hwnd,
                 nullptr,
                 nullptr,
                 RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

HFONT PdfReaderHelper::GetUiFont()
{
    static HFONT font = nullptr;
    if (font == nullptr)
    {
        font = CreateFontW(-16,
                           0,
                           0,
                           0,
                           FW_NORMAL,
                           FALSE,
                           FALSE,
                           FALSE,
                           DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE,
                           L"楷体");
    }

    if (font == nullptr)
    {
        return reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    }
    return font;
}

void PdfReaderHelper::ApplyDefaultFont(HWND hwnd)
{
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetUiFont()), TRUE);
}

std::string PdfReaderHelper::WideToUtf8(const std::wstring& value)
{
    return CStringManager::UnicodeToUtf8(value);
}

std::wstring PdfReaderHelper::Utf8ToWide(const std::string& value)
{
    return CStringManager::Utf8ToUnicode(value);
}

std::wstring PdfReaderHelper::IntToWide(int value)
{
    return CStringManager::Format(L"%d", value);
}

std::wstring PdfReaderHelper::GetFileName(const std::wstring& filePath)
{
    const wchar_t* name = PathFindFileNameW(filePath.c_str());
    return name != nullptr ? std::wstring(name) : filePath;
}

std::wstring PdfReaderHelper::GetFileStem(const std::wstring& filePath)
{
    std::wstring name = PdfReaderHelper::GetFileName(filePath);
    wchar_t buffer[MAX_PATH] = {0};
    StringCchCopyW(buffer, MAX_PATH, name.c_str());
    PathRemoveExtensionW(buffer);
    return std::wstring(buffer);
}

std::wstring PdfReaderHelper::JoinPath(const std::wstring& folderPath, const std::wstring& fileName)
{
    wchar_t buffer[MAX_PATH] = {0};
    StringCchCopyW(buffer, MAX_PATH, folderPath.c_str());
    PathAppendW(buffer, fileName.c_str());
    return std::wstring(buffer);
}

std::wstring PdfReaderHelper::MakePageFileName(const std::wstring& baseName, int pageNumber)
{
    wchar_t buffer[MAX_PATH] = {0};
    StringCchPrintfW(buffer, MAX_PATH, L"%s_%03d.PDF", baseName.c_str(), pageNumber);
    return std::wstring(buffer);
}

std::wstring PdfReaderHelper::MakeTempPdfFilePath(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        return std::wstring();
    }

    wchar_t folder[MAX_PATH] = {0};
    StringCchCopyW(folder, MAX_PATH, filePath.c_str());
    if (!PathRemoveFileSpecW(folder))
    {
        return std::wstring();
    }

    for (int i = 0; i < 100; ++i)
    {
        wchar_t fileName[MAX_PATH] = {0};
        StringCchPrintfW(fileName,
                         MAX_PATH,
                         L"PdfReader_%lu_%lu_%d.tmp.pdf",
                         GetCurrentProcessId(),
                         GetTickCount(),
                         i);

        wchar_t tempPath[MAX_PATH] = {0};
        StringCchCopyW(tempPath, MAX_PATH, folder);
        if (!PathAppendW(tempPath, fileName))
        {
            return std::wstring();
        }

        if (!PathFileExistsW(tempPath))
        {
            return std::wstring(tempPath);
        }
    }

    return std::wstring();
}

bool PdfReaderHelper::IsDebugArgument(const wchar_t* argument)
{
    if (argument == nullptr)
    {
        return false;
    }

    return lstrcmpiW(argument, L"debug") == 0 ||
           lstrcmpiW(argument, L"-debug") == 0 ||
           lstrcmpiW(argument, L"/debug") == 0 ||
           lstrcmpiW(argument, L"--debug") == 0;
}

bool PdfReaderHelper::HasDebugCommandLineArgument()
{
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments == nullptr)
    {
        return false;
    }

    bool hasDebug = false;
    for (int i = 1; i < argumentCount; ++i)
    {
        if (IsDebugArgument(arguments[i]))
        {
            hasDebug = true;
            break;
        }
    }
    LocalFree(arguments);
    return hasDebug;
}

std::wstring PdfReaderHelper::FirstCommandLineFilePath()
{
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments == nullptr)
    {
        return std::wstring();
    }

    std::wstring result;
    for (int i = 1; i < argumentCount; ++i)
    {
        if (arguments[i] != nullptr &&
            arguments[i][0] != L'\0' &&
            arguments[i][0] != L'-' &&
            arguments[i][0] != L'/' &&
            !IsDebugArgument(arguments[i]))
        {
            result = arguments[i];
            break;
        }
    }
    LocalFree(arguments);
    return result;
}
