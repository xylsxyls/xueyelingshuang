#include "ImageView.h"

namespace SplitViewer
{
    ImageView::ImageView() :
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

    ImageView::~ImageView()
    {
        Clear();
    }

    void ImageView::Clear()
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

    bool ImageView::HasImage() const
    {
        return image != NULL;
    }

    bool ImageView::HasFastBitmap() const
    {
        return fastBitmap != NULL && fastDc != NULL && fastBitmapWidth > 0 && fastBitmapHeight > 0;
    }

    bool ImageView::HasEmbeddedWindow() const
    {
        return embeddedWindow != NULL && IsWindow(embeddedWindow);
    }

    bool ImageView::HasContent() const
    {
        return HasImage() || HasEmbeddedWindow();
    }

    bool ImageView::Load(const std::wstring& file)
    {
        Gdiplus::Bitmap* loaded = Gdiplus::Bitmap::FromFile(file.c_str(), FALSE);
        if (!loaded || loaded->GetLastStatus() != Gdiplus::Ok || loaded->GetWidth() == 0 || loaded->GetHeight() == 0)
        {
            delete loaded;
            return false;
        }

        HBITMAP loadedFastBitmap = NULL;
        HDC loadedFastDc = NULL;
        HGDIOBJ loadedFastOldBitmap = NULL;
        const int loadedFastWidth = static_cast<int>(loaded->GetWidth());
        const int loadedFastHeight = static_cast<int>(loaded->GetHeight());
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
        return true;
    }

    void ImageView::TakeFrom(ImageView& other)
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

    bool ImageView::AttachEmbeddedWindow(HWND window, HWND newParent)
    {
        if (!window || !newParent || !IsWindow(window))
        {
            return false;
        }

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
        return true;
    }

    void ImageView::DetachEmbeddedWindow()
    {
        DetachEmbeddedWindowToRect(embeddedOriginalRect);
    }

    void ImageView::DetachEmbeddedWindowToRect(const RECT& screenRect)
    {
        if (embeddedWindow && IsWindow(embeddedWindow))
        {
            SetParent(embeddedWindow, embeddedOldParent);
            SetWindowLongPtrW(embeddedWindow, GWL_STYLE, embeddedOldStyle);
            SetWindowLongPtrW(embeddedWindow, GWL_EXSTYLE, embeddedOldExStyle);
            SetWindowPos(embeddedWindow,
                         NULL,
                         screenRect.left,
                         screenRect.top,
                         std::max(1, RectWidth(screenRect)),
                         std::max(1, RectHeight(screenRect)),
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            SetForegroundWindow(embeddedWindow);
        }

        ResetEmbeddedState();
    }

    void ImageView::ForgetEmbeddedWindow()
    {
        ResetEmbeddedState();
    }

    void ImageView::ClearFastBitmap()
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

    void ImageView::ResetEmbeddedState()
    {
        embeddedWindow = NULL;
        embeddedOldParent = NULL;
        embeddedOldStyle = 0;
        embeddedOldExStyle = 0;
        ZeroMemory(&embeddedOriginalRect, sizeof(embeddedOriginalRect));
    }
}
