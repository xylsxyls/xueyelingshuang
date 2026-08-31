#pragma once

#include "SplitViewerCommon.h"

namespace SplitViewer
{
    struct ImageView
    {
    public:
        Gdiplus::Image* image;
        HBITMAP fastBitmap;
        HDC fastDc;
        HGDIOBJ fastOldBitmap;
        int fastBitmapWidth;
        int fastBitmapHeight;
        std::wstring path;
        double scale;
        double offsetX;
        double offsetY;
        bool autoFit;
        HWND embeddedWindow;
        HWND embeddedOldParent;
        LONG_PTR embeddedOldStyle;
        LONG_PTR embeddedOldExStyle;
        RECT embeddedOriginalRect;

    public:
        ImageView();

        ~ImageView();

        void Clear();

        bool HasImage() const;

        bool HasFastBitmap() const;

        bool HasEmbeddedWindow() const;

        bool HasContent() const;

        bool Load(const std::wstring& file);

        void TakeFrom(ImageView& other);

        bool AttachEmbeddedWindow(HWND window, HWND newParent);

        void DetachEmbeddedWindow();

        void DetachEmbeddedWindowToRect(const RECT& screenRect);

        void ForgetEmbeddedWindow();

    private:
        void ClearFastBitmap();

        void ResetEmbeddedState();

    private:
        ImageView(const ImageView& other);
        ImageView& operator=(const ImageView& other);
    };
}
