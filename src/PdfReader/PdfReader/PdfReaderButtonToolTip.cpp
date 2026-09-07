#include "PdfReaderButtonToolTip.h"

#include "PdfReaderHelper.h"
#include "PdfReaderInstance.h"

PdfReaderButtonToolTip::PdfReaderButtonToolTip() :
m_instance(nullptr),
m_owner(nullptr),
m_hwnd(nullptr),
m_button(TOP_BUTTON_NONE)
{
}

PdfReaderButtonToolTip::~PdfReaderButtonToolTip()
{
    destroy();
}

bool PdfReaderButtonToolTip::registerWindowClass(HINSTANCE instance) const
{
    WNDCLASSEXW toolTipClass;
    ZeroMemory(&toolTipClass, sizeof(toolTipClass));
    toolTipClass.cbSize = sizeof(toolTipClass);
    toolTipClass.lpfnWndProc = PdfReaderButtonToolTip::WndProc;
    toolTipClass.hInstance = instance;
    toolTipClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    toolTipClass.lpszClassName = kButtonToolTipClass;
    if (!RegisterClassExW(&toolTipClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register button tooltip window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kButtonToolTipClass).c_str(),
                            GetLastError());
        return false;
    }
    return true;
}

bool PdfReaderButtonToolTip::create(HINSTANCE instance, HWND owner)
{
    if (m_hwnd != nullptr)
    {
        return true;
    }
    if (owner == nullptr)
    {
        PdfReaderInstance::instance().logError("Create tooltip failed, owner window is null");
        return false;
    }

    m_instance = instance;
    m_owner = owner;
    m_text = L" ";
    m_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                             kButtonToolTipClass,
                             L"",
                             WS_POPUP | WS_DISABLED,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             kToolTipMinWidth,
                             kToolTipMinHeight,
                             owner,
                             nullptr,
                             instance,
                             this);
    if (m_hwnd == nullptr)
    {
        PdfReaderInstance::instance().logError("Create tooltip failed, lastError=%lu", GetLastError());
        return false;
    }

    ShowWindow(m_hwnd, SW_HIDE);
    PdfReaderInstance::instance().logInfo("Tooltip created, hwnd=%p", m_hwnd);
    return true;
}

void PdfReaderButtonToolTip::destroy()
{
    if (m_hwnd != nullptr && IsWindow(m_hwnd))
    {
        DestroyWindow(m_hwnd);
    }
    m_hwnd = nullptr;
    m_owner = nullptr;
    m_button = TOP_BUTTON_NONE;
    m_text.clear();
}

void PdfReaderButtonToolTip::showAt(int button, const std::wstring& text, POINT ownerClientPoint)
{
    if (m_hwnd == nullptr || m_owner == nullptr || button == TOP_BUTTON_NONE || text.empty())
    {
        hide();
        return;
    }

    bool changed = m_button != button || m_text != text;
    m_button = button;
    m_text = text;
    moveTo(ownerClientPoint);
    InvalidateRect(m_hwnd, nullptr, FALSE);
    UpdateWindow(m_hwnd);

    if (changed)
    {
        PdfReaderInstance::instance().logInfo("Show button tooltip, button=%d, text=%s",
                           button,
                           PdfReaderHelper::WideToUtf8(m_text).c_str());
    }
}

void PdfReaderButtonToolTip::moveTo(POINT ownerClientPoint)
{
    if (m_hwnd == nullptr || m_owner == nullptr || m_button == TOP_BUTTON_NONE)
    {
        return;
    }

    POINT screenPoint = ownerClientPoint;
    ClientToScreen(m_owner, &screenPoint);
    screenPoint.x += kToolTipOffsetX;
    screenPoint.y += kToolTipOffsetY;

    SIZE size = measure();
    HMONITOR monitor = MonitorFromPoint(screenPoint, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo;
    ZeroMemory(&monitorInfo, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfoW(monitor, &monitorInfo))
    {
        RECT workRect = monitorInfo.rcWork;
        if (screenPoint.x + size.cx > workRect.right)
        {
            screenPoint.x = workRect.right - size.cx;
        }
        if (screenPoint.y + size.cy > workRect.bottom)
        {
            screenPoint.y = workRect.bottom - size.cy;
        }
        if (screenPoint.x < workRect.left)
        {
            screenPoint.x = workRect.left;
        }
        if (screenPoint.y < workRect.top)
        {
            screenPoint.y = workRect.top;
        }
    }

    SetWindowPos(m_hwnd,
                 HWND_TOPMOST,
                 screenPoint.x,
                 screenPoint.y,
                 size.cx,
                 size.cy,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void PdfReaderButtonToolTip::hide()
{
    if (m_hwnd != nullptr && IsWindow(m_hwnd))
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }
    m_button = TOP_BUTTON_NONE;
}

int PdfReaderButtonToolTip::currentButton() const
{
    return m_button;
}

LRESULT CALLBACK PdfReaderButtonToolTip::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderButtonToolTip* toolTip = reinterpret_cast<PdfReaderButtonToolTip*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        toolTip = reinterpret_cast<PdfReaderButtonToolTip*>(create->lpCreateParams);
        if (toolTip != nullptr)
        {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(toolTip));
            toolTip->m_hwnd = hwnd;
        }
    }

    if (toolTip != nullptr)
    {
        return toolTip->handleMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PdfReaderButtonToolTip::handleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    (void)wParam;
    (void)lParam;

    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hdc = BeginPaint(hwnd, &paintStruct);
        paint(hdc);
        EndPaint(hwnd, &paintStruct);
        return 0;
    }
    case WM_DESTROY:
        m_hwnd = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void PdfReaderButtonToolTip::paint(HDC hdc)
{
    RECT client;
    GetClientRect(m_hwnd, &client);
    PdfReaderHelper::FillSolidRect(hdc, client, kToolTipBackgroundColor);
    PdfReaderHelper::DrawSolidFrame(hdc, client, kToolTipBorderColor);

    HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kToolTipTextColor);

    RECT textRect = client;
    textRect.left += kToolTipPaddingX;
    textRect.top += kToolTipPaddingY;
    textRect.right -= kToolTipPaddingX;
    textRect.bottom -= kToolTipPaddingY;
    DrawTextW(hdc, m_text.c_str(), -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

SIZE PdfReaderButtonToolTip::measure() const
{
    SIZE size = {kToolTipMinWidth, kToolTipMinHeight};
    if (m_owner == nullptr || m_text.empty())
    {
        return size;
    }

    HDC hdc = GetDC(m_owner);
    if (hdc == nullptr)
    {
        return size;
    }

    HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
    SIZE textSize = {0, 0};
    GetTextExtentPoint32W(hdc, m_text.c_str(), static_cast<int>(m_text.size()), &textSize);
    SelectObject(hdc, oldFont);
    ReleaseDC(m_owner, hdc);

    size.cx = std::max(kToolTipMinWidth, static_cast<int>(textSize.cx) + kToolTipWidthPadding);
    size.cy = std::max(kToolTipMinHeight, static_cast<int>(textSize.cy) + kToolTipHeightPadding);
    return size;
}