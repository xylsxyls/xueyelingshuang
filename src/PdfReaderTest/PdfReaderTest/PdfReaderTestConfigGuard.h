#pragma once
#include "../../PdfReader/PdfReader/Config.h"

/** Restore the desktop settings changed by one regression, even on failure
*/
class PdfReaderTestConfigGuard
{
public:
    /** Capture settings before creating any product widgets
    */
    PdfReaderTestConfigGuard();

    /** Restore settings after product widgets have been destroyed
    */
    ~PdfReaderTestConfigGuard();

private:
    // Saved per-instance Core startup configuration
    PdfReaderCoreCConfig m_core;
    // Saved dialogTitleBarHeight
    int32_t m_dialogTitleBarHeight;
    // Saved titleCloseHeight
    int32_t m_titleCloseHeight;
    // Saved useNativeFileDialog
    bool m_useNativeFileDialog;
    // Saved windowSize
    QSize m_windowSize;
    // Saved initialZoom
    double m_initialZoom;
    // Saved minimumZoom
    double m_minimumZoom;
    // Saved maximumZoom
    double m_maximumZoom;
    // Saved zoomStep
    double m_zoomStep;
    // Saved thumbnailWidth
    int m_thumbnailWidth;
    // Saved sidebarWidth
    int m_sidebarWidth;
};