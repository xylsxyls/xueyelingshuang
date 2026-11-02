#include "NetClientGatewayLifetime.h"

NetClientGatewayLifetime::NetClientGatewayLifetime() :
#ifdef _WIN32
m_window(nullptr),
m_class(0),
#endif
m_stopping(false),
m_failed(false)
{

}

NetClientGatewayLifetime::~NetClientGatewayLifetime()
{
#ifdef _WIN32
    if (m_window != nullptr)
    {
        SetWindowLongPtrW(m_window, GWLP_USERDATA, 0);
        DestroyWindow(m_window);
    }
    if (m_class != 0)
    {
        UnregisterClassW(L"XylsNetClientManagerLifetime", GetModuleHandleW(nullptr));
    }
#endif
}

bool NetClientGatewayLifetime::init(const std::function<void()>& shutdown)
{
    if (!shutdown || m_shutdown || m_stopping) { return false; }
    m_shutdown = shutdown;
#ifdef _WIN32
    WNDCLASSEXW type = {};
    type.cbSize = sizeof(type);
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpfnWndProc = &WindowProc;
    type.lpszClassName = L"XylsNetClientManagerLifetime";
    m_class = RegisterClassExW(&type);
    if (m_class == 0) { return false; }
    // 需要接收系统广播，不能使用HWND_MESSAGE；不带WS_VISIBLE且从不ShowWindow。
    m_window = CreateWindowExW(WS_EX_TOOLWINDOW, L"XylsNetClientManagerLifetime", L"", 0,
        0, 0, 0, 0, nullptr, nullptr, type.hInstance, this);
    return m_window != nullptr;
#else
    return true;
#endif
}

bool NetClientGatewayLifetime::poll()
{
#ifdef _WIN32
    MSG message = {};
    for (uint32_t count = 0; count < 32 && PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE); ++count)
    {
        if (message.message == WM_QUIT) { stop(); break; }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
#endif
    return !m_stopping;
}

bool NetClientGatewayLifetime::failed() const
{
    return m_failed;
}

void NetClientGatewayLifetime::stop()
{
    if (m_stopping) { return; }
    m_stopping = true;
    try { if (m_shutdown) { m_shutdown(); } }
    catch (...) { m_failed = true; }
}

#ifdef _WIN32
LRESULT CALLBACK NetClientGatewayLifetime::WindowProc(HWND window, UINT message, WPARAM first, LPARAM second)
{
    if (message == WM_NCCREATE)
    {
        const CREATESTRUCTW* created = reinterpret_cast<const CREATESTRUCTW*>(second);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(created->lpCreateParams));
    }
    NetClientGatewayLifetime* owner = reinterpret_cast<NetClientGatewayLifetime*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_QUERYENDSESSION) { return TRUE; }
    if (message == WM_ENDSESSION || message == WM_CLOSE)
    {
        if (owner != nullptr && (message == WM_CLOSE || first != FALSE)) { owner->stop(); }
        return 0;
    }
    return DefWindowProcW(window, message, first, second);
}

#endif