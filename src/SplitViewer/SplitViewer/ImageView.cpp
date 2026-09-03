#include "ImageView.h"

#include "Logger.h"
SplitViewerImageView::SplitViewerImageView() :
image(NULL),
    fastBitmap(NULL),
    fastDc(NULL),
    fastOldBitmap(NULL),
    fastBitmapWidth(0),
    fastBitmapHeight(0),
    scale(1.0),
    offsetX(0.0),
    offsetY(0.0),
    autoFit(true),
    embeddedWindow(NULL),
    embeddedOldParent(NULL),
    embeddedOldStyle(0),
    embeddedOldExStyle(0)
{
    ZeroMemory(&embeddedOriginalRect, sizeof(embeddedOriginalRect));
}

SplitViewerImageView::~SplitViewerImageView()
{
    Clear();
}

void SplitViewerImageView::Clear()
{
    DetachEmbeddedWindow();
    ClearFastBitmap();
    delete image;
    image = NULL;
    path.clear();
    scale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;
    autoFit = true;
}

bool SplitViewerImageView::HasImage() const
{
    return image != NULL;
}

bool SplitViewerImageView::HasFastBitmap() const
{
    return fastBitmap != NULL && fastDc != NULL && fastBitmapWidth > 0 && fastBitmapHeight > 0;
}

bool SplitViewerImageView::HasEmbeddedWindow() const
{
    return embeddedWindow != NULL && IsWindow(embeddedWindow);
}

bool SplitViewerImageView::HasContent() const
{
    return HasImage() || HasEmbeddedWindow();
}

bool SplitViewerImageView::Load(const std::wstring& file)
{
    const DWORD startTick = GetTickCount();
    SplitViewerDebugLogFormat(L"SplitViewerImageView::Load begin path=%s.", file.c_str());
    Gdiplus::Bitmap* loaded = Gdiplus::Bitmap::FromFile(file.c_str(), FALSE);
    const Gdiplus::Status status = loaded ? loaded->GetLastStatus() : Gdiplus::GenericError;
    const UINT loadedWidth = loaded && status == Gdiplus::Ok ? loaded->GetWidth() : 0;
    const UINT loadedHeight = loaded && status == Gdiplus::Ok ? loaded->GetHeight() : 0;
    if (!loaded || status != Gdiplus::Ok || loadedWidth == 0 || loadedHeight == 0)
    {
        delete loaded;
        SplitViewerDebugLogFormat(L"SplitViewerImageView::Load failed path=%s status=%d elapsedMs=%u.",
            file.c_str(),
            static_cast<int>(status),
            static_cast<unsigned int>(GetTickCount() - startTick));
        return false;
    }

    HBITMAP loadedFastBitmap = NULL;
    HDC loadedFastDc = NULL;
    HGDIOBJ loadedFastOldBitmap = NULL;
    const int loadedFastWidth = static_cast<int>(loadedWidth);
    const int loadedFastHeight = static_cast<int>(loadedHeight);
    if (loaded->GetHBITMAP(Gdiplus::Color(255, 128, 128, 128), &loadedFastBitmap) == Gdiplus::Ok && loadedFastBitmap)
    {
        loadedFastDc = CreateCompatibleDC(NULL);
        if (loadedFastDc)
        {
            loadedFastOldBitmap = SelectObject(loadedFastDc, loadedFastBitmap);
            if (!loadedFastOldBitmap || loadedFastOldBitmap == HGDI_ERROR)
            {
                loadedFastOldBitmap = NULL;
                DeleteDC(loadedFastDc);
                loadedFastDc = NULL;
                DeleteObject(loadedFastBitmap);
                loadedFastBitmap = NULL;
            }
        }
        else
        {
            DeleteObject(loadedFastBitmap);
            loadedFastBitmap = NULL;
        }
    }

    Clear();
    image = loaded;
    fastBitmap = loadedFastBitmap;
    fastDc = loadedFastDc;
    fastOldBitmap = loadedFastOldBitmap;
    fastBitmapWidth = fastBitmap ? loadedFastWidth : 0;
    fastBitmapHeight = fastBitmap ? loadedFastHeight : 0;
    path = file;
    scale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;
    autoFit = true;
    SplitViewerDebugLogFormat(L"SplitViewerImageView::Load success path=%s size=%ux%u fast=%d elapsedMs=%u.",
        path.c_str(),
        loadedWidth,
        loadedHeight,
        HasFastBitmap() ? 1 : 0,
        static_cast<unsigned int>(GetTickCount() - startTick));
    return true;
}

void SplitViewerImageView::TakeFrom(SplitViewerImageView& other)
{
    Clear();
    image = other.image;
    fastBitmap = other.fastBitmap;
    fastDc = other.fastDc;
    fastOldBitmap = other.fastOldBitmap;
    fastBitmapWidth = other.fastBitmapWidth;
    fastBitmapHeight = other.fastBitmapHeight;
    path = other.path;
    scale = other.scale;
    offsetX = other.offsetX;
    offsetY = other.offsetY;
    autoFit = other.autoFit;
    embeddedWindow = other.embeddedWindow;
    embeddedOldParent = other.embeddedOldParent;
    embeddedOldStyle = other.embeddedOldStyle;
    embeddedOldExStyle = other.embeddedOldExStyle;
    embeddedOriginalRect = other.embeddedOriginalRect;

    other.image = NULL;
    other.fastBitmap = NULL;
    other.fastDc = NULL;
    other.fastOldBitmap = NULL;
    other.fastBitmapWidth = 0;
    other.fastBitmapHeight = 0;
    other.path.clear();
    other.scale = 1.0;
    other.offsetX = 0.0;
    other.offsetY = 0.0;
    other.autoFit = true;
    other.ResetEmbeddedState();
}

bool SplitViewerImageView::AttachEmbeddedWindow(HWND window, HWND newParent)
{
    if (!window || !newParent || !IsWindow(window))
    {
        SplitViewerDebugLogFormat(L"AttachEmbeddedWindow rejected window=0x%p parent=0x%p.", window, newParent);
        return false;
    }

    SplitViewerDebugLogFormat(L"AttachEmbeddedWindow begin window=0x%p parent=0x%p.", window, newParent);
    Clear();
    embeddedWindow = window;
    embeddedOldParent = GetParent(window);
    embeddedOldStyle = GetWindowLongPtrW(window, GWL_STYLE);
    embeddedOldExStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    GetWindowRect(window, &embeddedOriginalRect);

    SetLastError(ERROR_SUCCESS);
    HWND previousParent = SetParent(window, newParent);
    if (!previousParent && GetLastError() != ERROR_SUCCESS)
    {
        SplitViewerDebugLogFormat(L"AttachEmbeddedWindow SetParent failed window=0x%p error=%u.",
            window,
            static_cast<unsigned int>(GetLastError()));
        ResetEmbeddedState();
        return false;
    }

    LONG_PTR style = embeddedOldStyle;
    style &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    LONG_PTR exStyle = embeddedOldExStyle;
    exStyle &= ~(WS_EX_APPWINDOW | WS_EX_TOOLWINDOW | WS_EX_TOPMOST |
    WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

    SetWindowLongPtrW(window, GWL_STYLE, style);
    SetWindowLongPtrW(window, GWL_EXSTYLE, exStyle);
    SetWindowPos(window,
        NULL,
        0,
        0,
        1,
        1,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    SplitViewerDebugLogFormat(L"AttachEmbeddedWindow success window=0x%p oldParent=0x%p oldRect=%d,%d,%d,%d.",
        embeddedWindow,
        embeddedOldParent,
        embeddedOriginalRect.left,
        embeddedOriginalRect.top,
        embeddedOriginalRect.right,
        embeddedOriginalRect.bottom);
    return true;
}

void SplitViewerImageView::DetachEmbeddedWindow()
{
    DetachEmbeddedWindowToRect(embeddedOriginalRect);
}

void SplitViewerImageView::DetachEmbeddedWindowToRect(const RECT& screenRect)
{
    if (embeddedWindow && IsWindow(embeddedWindow))
    {
        SplitViewerDebugLogFormat(L"DetachEmbeddedWindow window=0x%p rect=%d,%d,%d,%d.",
            embeddedWindow,
            screenRect.left,
            screenRect.top,
            screenRect.right,
            screenRect.bottom);
        SetParent(embeddedWindow, embeddedOldParent);
        SetWindowLongPtrW(embeddedWindow, GWL_STYLE, embeddedOldStyle);
        SetWindowLongPtrW(embeddedWindow, GWL_EXSTYLE, embeddedOldExStyle);
        SetWindowPos(embeddedWindow,
            NULL,
            screenRect.left,
            screenRect.top,
            std::max(1, SplitViewerRectWidth(screenRect)),
            std::max(1, SplitViewerRectHeight(screenRect)),
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        SetForegroundWindow(embeddedWindow);
    }

    ResetEmbeddedState();
}

void SplitViewerImageView::ForgetEmbeddedWindow()
{
    ResetEmbeddedState();
}

void SplitViewerImageView::ClearFastBitmap()
{
    if (fastDc && fastOldBitmap)
    {
        SelectObject(fastDc, fastOldBitmap);
    }
    if (fastBitmap)
    {
        DeleteObject(fastBitmap);
    }
    if (fastDc)
    {
        DeleteDC(fastDc);
    }

    fastBitmap = NULL;
    fastDc = NULL;
    fastOldBitmap = NULL;
    fastBitmapWidth = 0;
    fastBitmapHeight = 0;
}

void SplitViewerImageView::ResetEmbeddedState()
{
    embeddedWindow = NULL;
    embeddedOldParent = NULL;
    embeddedOldStyle = 0;
    embeddedOldExStyle = 0;
    ZeroMemory(&embeddedOriginalRect, sizeof(embeddedOriginalRect));
}