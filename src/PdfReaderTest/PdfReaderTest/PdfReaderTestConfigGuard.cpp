#include "PdfReaderTestConfigGuard.h"

PdfReaderTestConfigGuard::PdfReaderTestConfigGuard() :
m_core(g_config.m_core),
m_dialogTitleBarHeight(g_config.m_dialogTitleBarHeight),
m_titleCloseHeight(g_config.m_titleCloseHeight),
m_useNativeFileDialog(g_config.m_useNativeFileDialog),
m_windowSize(g_config.m_windowSize),
m_initialZoom(g_config.m_initialZoom),
m_minimumZoom(g_config.m_minimumZoom),
m_maximumZoom(g_config.m_maximumZoom),
m_zoomStep(g_config.m_zoomStep),
m_thumbnailWidth(g_config.m_thumbnailWidth),
m_sidebarWidth(g_config.m_sidebarWidth)
{

}

PdfReaderTestConfigGuard::~PdfReaderTestConfigGuard()
{
    g_config.m_core = m_core;
    g_config.m_dialogTitleBarHeight = m_dialogTitleBarHeight;
    g_config.m_titleCloseHeight = m_titleCloseHeight;
    g_config.m_useNativeFileDialog = m_useNativeFileDialog;
    g_config.m_windowSize = m_windowSize;
    g_config.m_initialZoom = m_initialZoom;
    g_config.m_minimumZoom = m_minimumZoom;
    g_config.m_maximumZoom = m_maximumZoom;
    g_config.m_zoomStep = m_zoomStep;
    g_config.m_thumbnailWidth = m_thumbnailWidth;
    g_config.m_sidebarWidth = m_sidebarWidth;
}