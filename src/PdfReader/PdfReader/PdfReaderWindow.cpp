#include "PdfReaderWindow.h"

#include "PdfReaderHelper.h"
#include "PdfReaderInstance.h"

#include <CSystem/CSystemAPI.h>

#include <cstring>

/** 简单输入弹窗的临时窗口状态
*/
struct PromptState
{
public:
    // 当前模块实例句柄
    HINSTANCE instance;
    // 父窗口句柄
    HWND owner;
    // 弹窗窗口句柄
    HWND hwnd;
    // 输入框窗口句柄
    HWND edit;
    // 弹窗标题
    std::wstring title;
    // 输入提示文本
    std::wstring label;
    // 输入框初始文本
    std::wstring initialValue;
    // 用户最终输入文本
    std::wstring value;
    // 是否使用密码输入模式
    bool password;
    // 用户是否点击确认
    bool accepted;
    // 弹窗消息循环是否结束
    bool done;

public:
    /** 构造默认输入弹窗状态
    */
    PromptState();
};

PromptState::PromptState()
    : instance(nullptr)
    , owner(nullptr)
    , hwnd(nullptr)
    , edit(nullptr)
    , password(false)
    , accepted(false)
    , done(false)
{
}

/** 关于弹窗的临时窗口状态
*/
struct AboutDialogState
{
public:
    // 当前模块实例句柄
    HINSTANCE instance;
    // 弹窗窗口句柄
    HWND hwnd;
    // 弹窗消息循环是否结束
    bool done;

public:
    /** 构造默认关于弹窗状态
    */
    AboutDialogState();
};

AboutDialogState::AboutDialogState()
    : instance(nullptr)
    , hwnd(nullptr)
    , done(false)
{
}

/** 输入弹窗窗口过程
入参 hwnd 窗口句柄
入参 message 窗口消息
入参 wParam 消息参数
入参 lParam 消息参数
返回值 窗口过程返回值
*/
static LRESULT CALLBACK PromptWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PromptState* state = reinterpret_cast<PromptState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<PromptState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->hwnd = hwnd;
        return TRUE;
    }
    case WM_CREATE:
    {
        RECT client;
        GetClientRect(hwnd, &client);
        HWND label = CreateWindowExW(0,
                                     L"STATIC",
                                     state->label.c_str(),
                                     WS_CHILD | WS_VISIBLE,
                                     14,
                                     16,
                                     PdfReaderHelper::RectWidth(client) - 28,
                                     20,
                                     hwnd,
                                     nullptr,
                                     state->instance,
                                     nullptr);
        PdfReaderHelper::ApplyDefaultFont(label);

        state->edit = CreateWindowExW(WS_EX_CLIENTEDGE,
                                      L"EDIT",
                                      state->initialValue.c_str(),
                                      WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                      14,
                                      44,
                                      PdfReaderHelper::RectWidth(client) - 28,
                                      24,
                                      hwnd,
                                      reinterpret_cast<HMENU>(1001),
                                      state->instance,
                                      nullptr);
        PdfReaderHelper::ApplyDefaultFont(state->edit);
        if (state->password)
        {
            SendMessageW(state->edit, EM_SETPASSWORDCHAR, L'*', 0);
        }
        SendMessageW(state->edit, EM_SETSEL, 0, -1);

        HWND okButton = CreateWindowExW(0,
                                        L"BUTTON",
                                        L"OK",
                                        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                        PdfReaderHelper::RectWidth(client) - 174,
                                        82,
                                        74,
                                        26,
                                        hwnd,
                                        reinterpret_cast<HMENU>(IDOK),
                                        state->instance,
                                        nullptr);
        HWND cancelButton = CreateWindowExW(0,
                                            L"BUTTON",
                                            L"Cancel",
                                            WS_CHILD | WS_VISIBLE,
                                            PdfReaderHelper::RectWidth(client) - 92,
                                            82,
                                            74,
                                            26,
                                            hwnd,
                                            reinterpret_cast<HMENU>(IDCANCEL),
                                            state->instance,
                                            nullptr);
        PdfReaderHelper::ApplyDefaultFont(okButton);
        PdfReaderHelper::ApplyDefaultFont(cancelButton);
        SetFocus(state->edit);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK && state != nullptr)
        {
            int length = GetWindowTextLengthW(state->edit);
            std::vector<wchar_t> text(static_cast<size_t>(length) + 1);
            if (length > 0)
            {
                GetWindowTextW(state->edit, &text[0], length + 1);
            }
            state->value.assign(&text[0]);
            state->accepted = true;
            state->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL && state != nullptr)
        {
            state->accepted = false;
            state->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (state != nullptr)
        {
            state->accepted = false;
            state->done = true;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (state != nullptr)
        {
            state->hwnd = nullptr;
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

/** 关于弹窗窗口过程
入参 hwnd 窗口句柄
入参 message 窗口消息
入参 wParam 消息参数
入参 lParam 消息参数
返回值 窗口过程返回值
*/
static LRESULT CALLBACK AboutWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AboutDialogState* state = reinterpret_cast<AboutDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = reinterpret_cast<AboutDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        state->hwnd = hwnd;
        return TRUE;
    }
    case WM_CREATE:
    {
        RECT client;
        GetClientRect(hwnd, &client);
        HWND okButton = CreateWindowExW(0,
                                        L"BUTTON",
                                        L"确定",
                                        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                        PdfReaderHelper::RectWidth(client) - 108,
                                        PdfReaderHelper::RectHeight(client) - 44,
                                        84,
                                        26,
                                        hwnd,
                                        reinterpret_cast<HMENU>(IDOK),
                                        state->instance,
                                        nullptr);
        PdfReaderHelper::ApplyDefaultFont(okButton);
        SetFocus(okButton);
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hwnd, &paint);
        RECT client;
        GetClientRect(hwnd, &client);
        PdfReaderHelper::FillSolidRect(hdc, client, PdfReaderHelper::Color(255, 255, 255));

        HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, PdfReaderHelper::Color(48, 56, 70));

        std::wstring text;
        text += kAppTitle;
        text += L"\r\n版本号：";
        text += kAppVersion;
        text += L"\r\n\r\n功能说明\r\n";
        text += L"打开和阅读PDF文件。\r\n";
        text += L"左侧缩略图支持拖拽调整页面顺序。\r\n";
        text += L"右键缩略图可在当前页前后插入PDF。\r\n";
        text += L"右键左侧空白处可分页保存或按页码范围保存。\r\n";
        text += L"按住Ctrl并滚动鼠标滚轮可缩放缩略图或正文。";

        RECT textRect = PdfReaderHelper::MakeRect(24, 20, client.right - 24, client.bottom - 58);
        DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SelectObject(hdc, oldFont);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_COMMAND:
        if ((LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) && state != nullptr)
        {
            state->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (state != nullptr)
        {
            state->done = true;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (state != nullptr)
        {
            state->hwnd = nullptr;
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

PdfReaderWindow::PdfReaderWindow() :
m_instance(nullptr),
m_hwnd(nullptr),
m_thumbView(nullptr),
m_documentView(nullptr),
m_engineReady(false),
m_leftWidth(210),
m_thumbScalePercent(100),
m_documentZoomPercent(100),
m_thumbScrollY(0),
m_documentScrollX(0),
m_documentScrollY(0),
m_selectedPage(-1),
m_contextPage(-1),
m_draggingThumb(false),
m_dragSourceIndex(-1),
m_dragInsertIndex(-1),
m_hotButton(TOP_BUTTON_NONE),
m_downButton(TOP_BUTTON_NONE)
{
    m_dragPoint.x = 0;
    m_dragPoint.y = 0;
    ZeroMemory(m_topButtons, sizeof(m_topButtons));
    ZeroMemory(m_toolbarButtons, sizeof(m_toolbarButtons));
}

PdfReaderWindow::~PdfReaderWindow()
{
    clearDocuments();
    m_engine.uninit();
}

bool PdfReaderWindow::create(HINSTANCE instance, int showCommand)
{
    PdfReaderInstance::instance().logInfo("Create main window begin, instance=%p, showCommand=%d",
                       instance,
                       showCommand);

    m_instance = instance;
    if (!registerWindowClasses(instance))
    {
        PdfReaderInstance::instance().logError("Register window classes failed, lastError=%lu", GetLastError());
        return false;
    }

    m_hwnd = CreateWindowExW(WS_EX_APPWINDOW,
                             kWindowClass,
                             kAppTitle,
                             WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             980,
                             720,
                             nullptr,
                             nullptr,
                             instance,
                             this);
    if (m_hwnd == nullptr)
    {
        PdfReaderInstance::instance().logError("CreateWindowExW main window failed, lastError=%lu", GetLastError());
        return false;
    }

    PdfReaderInstance::instance().logInfo("Create main window succeeded, hwnd=%p", m_hwnd);
    ShowWindow(m_hwnd, showCommand);
    UpdateWindow(m_hwnd);
    return true;
}

int PdfReaderWindow::runMessageLoop()
{
    PdfReaderInstance::instance().logInfo("Message loop begin");
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    PdfReaderInstance::instance().logInfo("Message loop end, wParam=%d", static_cast<int>(message.wParam));
    return static_cast<int>(message.wParam);
}

bool PdfReaderWindow::registerWindowClasses(HINSTANCE instance)
{
    WNDCLASSEXW windowClass;
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = PdfReaderWindow::MainWndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.lpszClassName = kWindowClass;
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register main window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kWindowClass).c_str(),
                            GetLastError());
        return false;
    }

    WNDCLASSEXW thumbClass;
    ZeroMemory(&thumbClass, sizeof(thumbClass));
    thumbClass.cbSize = sizeof(thumbClass);
    thumbClass.style = CS_DBLCLKS;
    thumbClass.lpfnWndProc = PdfReaderWindow::ThumbWndProc;
    thumbClass.hInstance = instance;
    thumbClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    thumbClass.lpszClassName = kThumbClass;
    if (!RegisterClassExW(&thumbClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register thumbnail window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kThumbClass).c_str(),
                            GetLastError());
        return false;
    }

    WNDCLASSEXW documentClass;
    ZeroMemory(&documentClass, sizeof(documentClass));
    documentClass.cbSize = sizeof(documentClass);
    documentClass.style = CS_DBLCLKS;
    documentClass.lpfnWndProc = PdfReaderWindow::DocumentWndProc;
    documentClass.hInstance = instance;
    documentClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    documentClass.lpszClassName = kDocumentClass;
    if (!RegisterClassExW(&documentClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register document window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kDocumentClass).c_str(),
                            GetLastError());
        return false;
    }

    WNDCLASSEXW promptClass;
    ZeroMemory(&promptClass, sizeof(promptClass));
    promptClass.cbSize = sizeof(promptClass);
    promptClass.lpfnWndProc = PromptWndProc;
    promptClass.hInstance = instance;
    promptClass.hCursor = LoadCursorW(nullptr, IDC_IBEAM);
    promptClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    promptClass.lpszClassName = kPromptClass;
    if (!RegisterClassExW(&promptClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register prompt window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kPromptClass).c_str(),
                            GetLastError());
        return false;
    }

    WNDCLASSEXW aboutClass;
    ZeroMemory(&aboutClass, sizeof(aboutClass));
    aboutClass.cbSize = sizeof(aboutClass);
    aboutClass.lpfnWndProc = AboutWndProc;
    aboutClass.hInstance = instance;
    aboutClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    aboutClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    aboutClass.lpszClassName = kAboutClass;
    if (!RegisterClassExW(&aboutClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        PdfReaderInstance::instance().logError("Register about window class failed, class=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(kAboutClass).c_str(),
                            GetLastError());
        return false;
    }
    PdfReaderInstance::instance().logInfo("Register window classes succeeded");
    return true;
}

LRESULT CALLBACK PdfReaderWindow::MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderWindow* window = reinterpret_cast<PdfReaderWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = reinterpret_cast<PdfReaderWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    }

    if (window != nullptr)
    {
        return window->handleMainMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK PdfReaderWindow::ThumbWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderWindow* window = reinterpret_cast<PdfReaderWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = reinterpret_cast<PdfReaderWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    if (window != nullptr)
    {
        return window->handleThumbMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK PdfReaderWindow::DocumentWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PdfReaderWindow* window = reinterpret_cast<PdfReaderWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = reinterpret_cast<PdfReaderWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }

    if (window != nullptr)
    {
        return window->handleDocumentMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PdfReaderWindow::handleMainMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        PdfReaderInstance::instance().logInfo("WM_CREATE begin");
        std::string error;
        m_engineReady = m_engine.init(&error);
        if (!m_engineReady)
        {
            PdfReaderInstance::instance().logError("PdfEngine init failed, error=%s", error.c_str());
            MessageBoxW(hwnd, PdfReaderHelper::Utf8ToWide(error).c_str(), kAppTitle, MB_OK | MB_ICONERROR);
        }
        else
        {
            PdfReaderInstance::instance().logInfo("PdfEngine init succeeded");
        }
        createChildViews();
        layoutChildren();
        PdfReaderInstance::instance().logInfo("WM_CREATE end, thumbView=%p, documentView=%p", m_thumbView, m_documentView);
        return 0;
    }
    case WM_SIZE:
        layoutChildren();
        invalidateAll();
        return 0;
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = kWindowMinWidth;
        info->ptMinTrackSize.y = kWindowMinHeight;
        return 0;
    }
    case WM_NCCALCSIZE:
    {
        if (wParam != FALSE && IsZoomed(hwnd))
        {
            NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
            HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            ZeroMemory(&monitorInfo, sizeof(monitorInfo));
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (GetMonitorInfoW(monitor, &monitorInfo))
            {
                params->rgrc[0] = monitorInfo.rcWork;
            }
        }
        return 0;
    }
    case WM_NCHITTEST:
    {
        POINT point;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        ScreenToClient(hwnd, &point);

        RECT client;
        GetClientRect(hwnd, &client);
        if (hitTestTopButton(point) != TOP_BUTTON_NONE || hitTestToolbarButton(point) != TOP_BUTTON_NONE)
        {
            return HTCLIENT;
        }

        const int border = 6;
        bool left = point.x >= client.left && point.x < client.left + border;
        bool right = point.x < client.right && point.x >= client.right - border;
        bool top = point.y >= client.top && point.y < client.top + border;
        bool bottom = point.y < client.bottom && point.y >= client.bottom - border;
        if (top && left)
        {
            return HTTOPLEFT;
        }
        if (top && right)
        {
            return HTTOPRIGHT;
        }
        if (bottom && left)
        {
            return HTBOTTOMLEFT;
        }
        if (bottom && right)
        {
            return HTBOTTOMRIGHT;
        }
        if (left)
        {
            return HTLEFT;
        }
        if (right)
        {
            return HTRIGHT;
        }
        if (top)
        {
            return HTTOP;
        }
        if (bottom)
        {
            return HTBOTTOM;
        }
        if (point.y >= 0 && point.y < kTitleHeight)
        {
            return HTCAPTION;
        }
        return HTCLIENT;
    }
    case WM_MOUSEMOVE:
    {
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int hotButton = hitTestTopButton(point);
        if (hotButton == TOP_BUTTON_NONE)
        {
            hotButton = hitTestToolbarButton(point);
        }
        if (hotButton != m_hotButton)
        {
            m_hotButton = hotButton;
            InvalidateRect(hwnd, nullptr, FALSE);
            TRACKMOUSEEVENT track;
            ZeroMemory(&track, sizeof(track));
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            TrackMouseEvent(&track);
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        m_hotButton = TOP_BUTTON_NONE;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int button = hitTestTopButton(point);
        if (button == TOP_BUTTON_NONE)
        {
            button = hitTestToolbarButton(point);
        }
        if (button != TOP_BUTTON_NONE)
        {
            m_downButton = button;
            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int button = hitTestTopButton(point);
        if (button == TOP_BUTTON_NONE)
        {
            button = hitTestToolbarButton(point);
        }

        int downButton = m_downButton;
        m_downButton = TOP_BUTTON_NONE;
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        InvalidateRect(hwnd, nullptr, FALSE);

        if (downButton != TOP_BUTTON_NONE && downButton == button)
        {
            if (button == TOP_BUTTON_MINIMIZE)
            {
                ShowWindow(hwnd, SW_MINIMIZE);
            }
            else if (button == TOP_BUTTON_MAXIMIZE)
            {
                ShowWindow(hwnd, IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
            }
            else if (button == TOP_BUTTON_CLOSE)
            {
                SendMessageW(hwnd, WM_CLOSE, 0, 0);
            }
            else if (button == TOP_BUTTON_OPEN)
            {
                openPdfCommand();
            }
            else if (button == TOP_BUTTON_SAVE)
            {
                savePdfCommand();
            }
            else if (button == TOP_BUTTON_HELP)
            {
                showAboutDialog();
            }
        }
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hwnd, &paint);
        paintMain(hdc);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_DESTROY:
        PdfReaderInstance::instance().logInfo("WM_DESTROY begin, documents=%u, pages=%u",
                           static_cast<unsigned int>(m_documents.size()),
                           static_cast<unsigned int>(m_pages.size()));
        clearDocuments();
        m_engine.uninit();
        PdfReaderInstance::instance().logInfo("WM_DESTROY end");
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PdfReaderWindow::handleThumbMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        updateThumbScrollbar();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hwnd, &paint);
        paintThumbnailView(hdc);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_VSCROLL:
        m_thumbScrollY = PdfReaderHelper::ScrollFromCode(hwnd, SB_VERT, wParam, m_thumbScrollY);
        updateThumbScrollbar();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEWHEEL:
    {
        int steps = PdfReaderHelper::WheelSteps(wParam);
        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)
        {
            setThumbnailScale(m_thumbScalePercent + steps * kZoomStepThumb);
        }
        else
        {
            scrollThumbBy(-steps * kScrollStep * 3);
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        SetFocus(hwnd);
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int page = hitTestThumbnail(point);
        if (page >= 0)
        {
            m_selectedPage = page;
            beginThumbnailDrag(page, point);
            InvalidateRect(m_documentView, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEMOVE:
        if (m_draggingThumb)
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            updateThumbnailDrag(point);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (m_draggingThumb)
        {
            finishThumbnailDrag(true);
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();
            }
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        if (m_draggingThumb)
        {
            finishThumbnailDrag(false);
        }
        return 0;
    case WM_TIMER:
        if (wParam == kTimerDragScroll && m_draggingThumb)
        {
            autoScrollDrag();
            return 0;
        }
        break;
    case WM_RBUTTONUP:
    {
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        showThumbnailContextMenu(point);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT PdfReaderWindow::handleDocumentMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        updateDocumentScrollbars();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hwnd, &paint);
        paintDocumentView(hdc);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_VSCROLL:
        m_documentScrollY = PdfReaderHelper::ScrollFromCode(hwnd, SB_VERT, wParam, m_documentScrollY);
        updateDocumentScrollbars();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_HSCROLL:
        m_documentScrollX = PdfReaderHelper::ScrollFromCode(hwnd, SB_HORZ, wParam, m_documentScrollX);
        updateDocumentScrollbars();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_MOUSEWHEEL:
    {
        int steps = PdfReaderHelper::WheelSteps(wParam);
        if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL)
        {
            setDocumentZoom(m_documentZoomPercent + steps * kZoomStepDoc);
        }
        else
        {
            scrollDocumentBy(0, -steps * kScrollStep * 3);
        }
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        int page = hitTestDocumentPage(point);
        if (page >= 0)
        {
            m_selectedPage = page;
            InvalidateRect(m_thumbView, nullptr, FALSE);
            InvalidateRect(m_documentView, nullptr, FALSE);
        }
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void PdfReaderWindow::createChildViews()
{
    PdfReaderInstance::instance().logInfo("Create child views begin");

    m_thumbView = CreateWindowExW(0,
                                  kThumbClass,
                                  L"",
                                  WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                  0,
                                  0,
                                  1,
                                  1,
                                  m_hwnd,
                                  nullptr,
                                  m_instance,
                                  this);
    if (m_thumbView == nullptr)
    {
        PdfReaderInstance::instance().logError("Create thumbnail view failed, lastError=%lu", GetLastError());
    }

    m_documentView = CreateWindowExW(0,
                                     kDocumentClass,
                                     L"",
                                     WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
                                     0,
                                     0,
                                     1,
                                     1,
                                     m_hwnd,
                                     nullptr,
                                     m_instance,
                                     this);
    if (m_documentView == nullptr)
    {
        PdfReaderInstance::instance().logError("Create document view failed, lastError=%lu", GetLastError());
    }
    PdfReaderInstance::instance().logInfo("Create child views end, thumbView=%p, documentView=%p", m_thumbView, m_documentView);
}

void PdfReaderWindow::layoutChildren()
{
    if (m_hwnd == nullptr || m_thumbView == nullptr || m_documentView == nullptr)
    {
        return;
    }

    RECT client;
    GetClientRect(m_hwnd, &client);
    updateTitleButtons(client);
    updateToolbarButtons(client);

    int contentTop = kTitleHeight + kToolbarHeight;
    int width = PdfReaderHelper::RectWidth(client);
    int height = PdfReaderHelper::RectHeight(client) - contentTop;
    if (height < 0)
    {
        height = 0;
    }

    int maxLeftWidth = std::max(kLeftMinWidth, width - kRightMinWidth - kSplitterWidth);
    m_leftWidth = PdfReaderHelper::ClampInt(m_leftWidth, kLeftMinWidth, maxLeftWidth);

    MoveWindow(m_thumbView, 0, contentTop, m_leftWidth, height, TRUE);
    MoveWindow(m_documentView, m_leftWidth + kSplitterWidth, contentTop, std::max(1, width - m_leftWidth - kSplitterWidth), height, TRUE);
    updateScrollbars();
}

void PdfReaderWindow::updateTitleButtons(const RECT& clientRect)
{
    int top = 0;
    int right = clientRect.right;

    m_topButtons[0].id = TOP_BUTTON_MINIMIZE;
    m_topButtons[0].rect = PdfReaderHelper::MakeRect(right - kTitleButtonWidth * kTitleButtonCount, top, right - kTitleButtonWidth * 2, kTitleHeight);
    m_topButtons[0].enabled = true;

    m_topButtons[1].id = TOP_BUTTON_MAXIMIZE;
    m_topButtons[1].rect = PdfReaderHelper::MakeRect(right - kTitleButtonWidth * 2, top, right - kTitleButtonWidth, kTitleHeight);
    m_topButtons[1].enabled = true;

    m_topButtons[2].id = TOP_BUTTON_CLOSE;
    m_topButtons[2].rect = PdfReaderHelper::MakeRect(right - kTitleButtonWidth, top, right, kTitleHeight);
    m_topButtons[2].enabled = true;
}

void PdfReaderWindow::updateToolbarButtons(const RECT& clientRect)
{
    int top = kTitleHeight + (kToolbarHeight - kToolButtonSize) / 2;
    m_toolbarButtons[0].id = TOP_BUTTON_OPEN;
    m_toolbarButtons[0].rect = PdfReaderHelper::MakeRect(12, top, 12 + kToolButtonSize, top + kToolButtonSize);
    m_toolbarButtons[0].enabled = true;

    m_toolbarButtons[1].id = TOP_BUTTON_SAVE;
    m_toolbarButtons[1].rect = PdfReaderHelper::MakeRect(44, top, 44 + kToolButtonSize, top + kToolButtonSize);
    m_toolbarButtons[1].enabled = !m_pages.empty();

    m_toolbarButtons[2].id = TOP_BUTTON_HELP;
    m_toolbarButtons[2].rect = PdfReaderHelper::MakeRect(clientRect.right - 12 - kToolButtonSize,
                                                         top,
                                                         clientRect.right - 12,
                                                         top + kToolButtonSize);
    m_toolbarButtons[2].enabled = true;
}

void PdfReaderWindow::updateScrollbars()
{
    updateThumbScrollbar();
    updateDocumentScrollbars();
}

void PdfReaderWindow::updateThumbScrollbar()
{
    if (m_thumbView == nullptr)
    {
        return;
    }

    RECT client;
    GetClientRect(m_thumbView, &client);
    int viewHeight = std::max(0, PdfReaderHelper::RectHeight(client));
    int totalHeight = getThumbTotalHeight();
    int maxPosition = std::max(0, totalHeight - viewHeight);
    m_thumbScrollY = PdfReaderHelper::ClampInt(m_thumbScrollY, 0, maxPosition);

    SCROLLINFO info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = std::max(0, totalHeight - 1);
    info.nPage = static_cast<UINT>(viewHeight);
    info.nPos = m_thumbScrollY;
    SetScrollInfo(m_thumbView, SB_VERT, &info, TRUE);
}

void PdfReaderWindow::updateDocumentScrollbars()
{
    if (m_documentView == nullptr)
    {
        return;
    }

    RECT client;
    GetClientRect(m_documentView, &client);
    int viewWidth = std::max(0, PdfReaderHelper::RectWidth(client));
    int viewHeight = std::max(0, PdfReaderHelper::RectHeight(client));
    int totalWidth = getDocumentTotalWidth();
    int totalHeight = getDocumentTotalHeight();

    m_documentScrollX = PdfReaderHelper::ClampInt(m_documentScrollX, 0, std::max(0, totalWidth - viewWidth));
    m_documentScrollY = PdfReaderHelper::ClampInt(m_documentScrollY, 0, std::max(0, totalHeight - viewHeight));

    SCROLLINFO horizontal;
    ZeroMemory(&horizontal, sizeof(horizontal));
    horizontal.cbSize = sizeof(horizontal);
    horizontal.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    horizontal.nMin = 0;
    horizontal.nMax = std::max(0, totalWidth - 1);
    horizontal.nPage = static_cast<UINT>(viewWidth);
    horizontal.nPos = m_documentScrollX;
    SetScrollInfo(m_documentView, SB_HORZ, &horizontal, TRUE);

    SCROLLINFO vertical;
    ZeroMemory(&vertical, sizeof(vertical));
    vertical.cbSize = sizeof(vertical);
    vertical.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    vertical.nMin = 0;
    vertical.nMax = std::max(0, totalHeight - 1);
    vertical.nPage = static_cast<UINT>(viewHeight);
    vertical.nPos = m_documentScrollY;
    SetScrollInfo(m_documentView, SB_VERT, &vertical, TRUE);
}

void PdfReaderWindow::clampScrollPositions()
{
    updateScrollbars();
}

void PdfReaderWindow::invalidateAll()
{
    if (m_hwnd != nullptr)
    {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
    if (m_thumbView != nullptr)
    {
        InvalidateRect(m_thumbView, nullptr, FALSE);
    }
    if (m_documentView != nullptr)
    {
        InvalidateRect(m_documentView, nullptr, FALSE);
    }
}

void PdfReaderWindow::clearDocuments()
{
    if (!m_documents.empty() || !m_pages.empty())
    {
        PdfReaderInstance::instance().logInfo("Clear documents, documents=%u, pages=%u, thumbCache=%u, documentCache=%u",
                           static_cast<unsigned int>(m_documents.size()),
                           static_cast<unsigned int>(m_pages.size()),
                           static_cast<unsigned int>(m_thumbCache.size()),
                           static_cast<unsigned int>(m_documentCache.size()));
    }

    for (size_t i = 0; i < m_documents.size(); ++i)
    {
        delete m_documents[i];
    }
    m_documents.clear();
    m_pages.clear();
    m_mainFilePath.clear();
    m_selectedPage = -1;
    m_contextPage = -1;
    m_thumbScrollY = 0;
    m_documentScrollX = 0;
    m_documentScrollY = 0;
    clearRenderCaches();
}

void PdfReaderWindow::clearRenderCaches()
{
    if (!m_thumbCache.empty() || !m_documentCache.empty())
    {
        PdfReaderInstance::instance().logInfo("Clear render caches, thumbCache=%u, documentCache=%u",
                           static_cast<unsigned int>(m_thumbCache.size()),
                           static_cast<unsigned int>(m_documentCache.size()));
    }

    m_thumbCache.clear();
    m_documentCache.clear();
}

void PdfReaderWindow::paintMain(HDC hdc)
{
    RECT client;
    GetClientRect(m_hwnd, &client);
    updateTitleButtons(client);
    updateToolbarButtons(client);

    RECT titleRect = PdfReaderHelper::MakeRect(client.left, client.top, client.right, kTitleHeight);
    RECT toolbarRect = PdfReaderHelper::MakeRect(client.left, kTitleHeight, client.right, kTitleHeight + kToolbarHeight);
    PdfReaderHelper::FillSolidRect(hdc, titleRect, PdfReaderHelper::Color(35, 39, 45));
    PdfReaderHelper::FillSolidRect(hdc, toolbarRect, PdfReaderHelper::Color(247, 248, 250));

    SetBkMode(hdc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
    SetTextColor(hdc, PdfReaderHelper::Color(245, 247, 250));
    std::wstring title = kAppTitle;
    if (!m_mainFilePath.empty())
    {
        title += L" - ";
        title += PdfReaderHelper::GetFileName(m_mainFilePath);
    }
    RECT textRect = PdfReaderHelper::MakeRect(12, 0, client.right - kTitleButtonWidth * kTitleButtonCount - 8, kTitleHeight);
    DrawTextW(hdc, title.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    for (int i = 0; i < kTitleButtonCount; ++i)
    {
        drawTopButton(hdc, m_topButtons[i], m_hotButton == m_topButtons[i].id, m_downButton == m_topButtons[i].id);
    }
    for (int i = 0; i < kToolbarButtonCount; ++i)
    {
        drawToolbarButton(hdc, m_toolbarButtons[i], m_hotButton == m_toolbarButtons[i].id, m_downButton == m_toolbarButtons[i].id);
    }

    RECT split = PdfReaderHelper::MakeRect(m_leftWidth, kTitleHeight + kToolbarHeight, m_leftWidth + kSplitterWidth, client.bottom);
    PdfReaderHelper::FillSolidRect(hdc, split, PdfReaderHelper::Color(225, 228, 233));

    HPEN pen = CreatePen(PS_SOLID, 1, PdfReaderHelper::Color(224, 227, 232));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, client.left, kTitleHeight + kToolbarHeight - 1, nullptr);
    LineTo(hdc, client.right, kTitleHeight + kToolbarHeight - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    SelectObject(hdc, oldFont);
}

void PdfReaderWindow::drawTopButton(HDC hdc, const TopButtonState& button, bool hover, bool down)
{
    COLORREF fill = PdfReaderHelper::Color(35, 39, 45);
    COLORREF glyph = PdfReaderHelper::Color(233, 236, 240);
    if (button.id == TOP_BUTTON_CLOSE && hover)
    {
        fill = down ? PdfReaderHelper::Color(171, 31, 31) : PdfReaderHelper::Color(210, 49, 49);
        glyph = PdfReaderHelper::Color(255, 255, 255);
    }
    else if (hover || down)
    {
        fill = down ? PdfReaderHelper::Color(65, 70, 78) : PdfReaderHelper::Color(52, 57, 66);
    }

    PdfReaderHelper::FillSolidRect(hdc, button.rect, fill);

    HPEN pen = CreatePen(PS_SOLID, 2, glyph);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    int cx = (button.rect.left + button.rect.right) / 2;
    int cy = (button.rect.top + button.rect.bottom) / 2;
    if (button.id == TOP_BUTTON_MINIMIZE)
    {
        MoveToEx(hdc, cx - 7, cy + 5, nullptr);
        LineTo(hdc, cx + 8, cy + 5);
    }
    else if (button.id == TOP_BUTTON_MAXIMIZE)
    {
        if (IsZoomed(m_hwnd))
        {
            Rectangle(hdc, cx - 2, cy - 8, cx + 9, cy + 3);
            Rectangle(hdc, cx - 8, cy - 2, cx + 3, cy + 9);
        }
        else
        {
            Rectangle(hdc, cx - 7, cy - 6, cx + 8, cy + 8);
        }
    }
    else if (button.id == TOP_BUTTON_CLOSE)
    {
        MoveToEx(hdc, cx - 6, cy - 6, nullptr);
        LineTo(hdc, cx + 7, cy + 7);
        MoveToEx(hdc, cx + 6, cy - 6, nullptr);
        LineTo(hdc, cx - 7, cy + 7);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void PdfReaderWindow::drawToolbarButton(HDC hdc, const TopButtonState& button, bool hover, bool down)
{
    COLORREF fill = PdfReaderHelper::Color(247, 248, 250);
    COLORREF border = PdfReaderHelper::Color(197, 203, 213);
    COLORREF glyph = PdfReaderHelper::Color(61, 69, 82);
    if (button.id == TOP_BUTTON_HELP)
    {
        fill = PdfReaderHelper::Color(240, 247, 255);
        border = PdfReaderHelper::Color(120, 174, 236);
        glyph = PdfReaderHelper::Color(31, 112, 220);
    }

    if (!button.enabled)
    {
        glyph = PdfReaderHelper::Color(165, 171, 181);
        border = PdfReaderHelper::Color(222, 225, 230);
    }
    else if (down)
    {
        fill = button.id == TOP_BUTTON_HELP ? PdfReaderHelper::Color(205, 226, 251) : PdfReaderHelper::Color(216, 225, 240);
        border = button.id == TOP_BUTTON_HELP ? PdfReaderHelper::Color(66, 135, 220) : PdfReaderHelper::Color(118, 143, 184);
    }
    else if (hover)
    {
        fill = button.id == TOP_BUTTON_HELP ? PdfReaderHelper::Color(224, 239, 255) : PdfReaderHelper::Color(232, 238, 248);
        border = button.id == TOP_BUTTON_HELP ? PdfReaderHelper::Color(83, 151, 232) : PdfReaderHelper::Color(144, 165, 199);
    }

    PdfReaderHelper::FillSolidRect(hdc, button.rect, fill);
    PdfReaderHelper::DrawSolidFrame(hdc, button.rect, border);

    HPEN pen = CreatePen(PS_SOLID, 2, glyph);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    int left = button.rect.left;
    int top = button.rect.top;
    if (button.id == TOP_BUTTON_OPEN)
    {
        POINT points[6] = {
            {left + 5, top + 9},
            {left + 9, top + 9},
            {left + 11, top + 6},
            {left + 18, top + 6},
            {left + 20, top + 9},
            {left + 20, top + 18}
        };
        Polyline(hdc, points, 6);
        MoveToEx(hdc, left + 5, top + 9, nullptr);
        LineTo(hdc, left + 5, top + 18);
        LineTo(hdc, left + 20, top + 18);
        MoveToEx(hdc, left + 6, top + 12, nullptr);
        LineTo(hdc, left + 21, top + 12);
    }
    else if (button.id == TOP_BUTTON_SAVE)
    {
        Rectangle(hdc, left + 6, top + 4, left + 19, top + 20);
        MoveToEx(hdc, left + 9, top + 4, nullptr);
        LineTo(hdc, left + 9, top + 10);
        LineTo(hdc, left + 16, top + 10);
        LineTo(hdc, left + 16, top + 4);
        Rectangle(hdc, left + 8, top + 15, left + 17, top + 20);
        MoveToEx(hdc, left + 10, top + 17, nullptr);
        LineTo(hdc, left + 15, top + 17);
    }
    else if (button.id == TOP_BUTTON_HELP)
    {
        Ellipse(hdc, left + 5, top + 4, left + 20, top + 19);

        HGDIOBJ oldFont = SelectObject(hdc, PdfReaderHelper::GetUiFont());
        COLORREF oldTextColor = SetTextColor(hdc, glyph);
        SetBkMode(hdc, TRANSPARENT);
        RECT textRect = PdfReaderHelper::MakeRect(left, top + 1, left + kToolButtonSize, top + kToolButtonSize - 1);
        DrawTextW(hdc, L"?", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(hdc, oldTextColor);
        SelectObject(hdc, oldFont);
    }
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

int PdfReaderWindow::hitTestTopButton(POINT point) const
{
    for (int i = 0; i < kTitleButtonCount; ++i)
    {
        if (m_topButtons[i].enabled && PdfReaderHelper::PtInRectLocal(m_topButtons[i].rect, point))
        {
            return m_topButtons[i].id;
        }
    }
    return TOP_BUTTON_NONE;
}

int PdfReaderWindow::hitTestToolbarButton(POINT point) const
{
    for (int i = 0; i < kToolbarButtonCount; ++i)
    {
        if (m_toolbarButtons[i].enabled && PdfReaderHelper::PtInRectLocal(m_toolbarButtons[i].rect, point))
        {
            return m_toolbarButtons[i].id;
        }
    }
    return TOP_BUTTON_NONE;
}

void PdfReaderWindow::paintThumbnailView(HDC hdc)
{
    RECT client;
    GetClientRect(m_thumbView, &client);
    PdfReaderHelper::FillSolidRect(hdc, client, PdfReaderHelper::Color(238, 241, 245));
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, PdfReaderHelper::GetUiFont());

    if (m_pages.empty())
    {
        SetTextColor(hdc, PdfReaderHelper::Color(122, 130, 143));
        DrawTextW(hdc, L"Open PDF", -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    HRGN clip = CreateRectRgn(client.left, client.top, client.right, client.bottom);
    SelectClipRgn(hdc, clip);
    DeleteObject(clip);

    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        int itemTop = getThumbItemTop(i) - m_thumbScrollY;
        int itemHeight = getThumbItemHeight(i);
        if (itemTop > client.bottom || itemTop + itemHeight < client.top)
        {
            continue;
        }

        RECT pageRect = getThumbPageRect(i);
        OffsetRect(&pageRect, 0, -m_thumbScrollY);
        RECT itemRect = PdfReaderHelper::MakeRect(kThumbPadding / 2, itemTop + 2, client.right - kThumbPadding / 2, itemTop + itemHeight - 2);
        COLORREF frameColor = i == m_selectedPage ? PdfReaderHelper::Color(48, 111, 210) : PdfReaderHelper::Color(192, 198, 207);
        PdfReaderHelper::FillSolidRect(hdc, itemRect, i == m_selectedPage ? PdfReaderHelper::Color(226, 235, 250) : PdfReaderHelper::Color(238, 241, 245));

        const PdfEngineBitmap* bitmap = getCachedBitmap(i, PdfReaderHelper::RectWidth(pageRect), PdfReaderHelper::RectHeight(pageRect), &m_thumbCache);
        if (bitmap != nullptr)
        {
            drawPageBitmap(hdc, *bitmap, pageRect, 255);
        }
        else
        {
            PdfReaderHelper::FillSolidRect(hdc, pageRect, PdfReaderHelper::Color(255, 255, 255));
        }
        PdfReaderHelper::DrawSolidFrame(hdc, pageRect, frameColor);

        if (m_draggingThumb && i == m_dragSourceIndex)
        {
            PdfReaderHelper::FillSolidRect(hdc, pageRect, PdfReaderHelper::Color(210, 215, 224));
            PdfReaderHelper::DrawSolidFrame(hdc, pageRect, PdfReaderHelper::Color(48, 111, 210));
        }

        std::wstring label = PdfReaderHelper::IntToWide(i + 1);
        RECT labelRect = PdfReaderHelper::MakeRect(itemRect.left, pageRect.bottom + 3, itemRect.right, pageRect.bottom + 3 + kThumbLabelHeight);
        SetTextColor(hdc, PdfReaderHelper::Color(72, 81, 95));
        DrawTextW(hdc, label.c_str(), -1, &labelRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (m_draggingThumb)
    {
        int lineY = getInsertLineY(m_dragInsertIndex) - m_thumbScrollY;
        HPEN linePen = CreatePen(PS_SOLID, 3, PdfReaderHelper::Color(49, 105, 200));
        HGDIOBJ oldPen = SelectObject(hdc, linePen);
        MoveToEx(hdc, kThumbPadding / 2, lineY, nullptr);
        LineTo(hdc, client.right - kThumbPadding / 2, lineY);
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);

        if (m_dragSourceIndex >= 0 && m_dragSourceIndex < static_cast<int>(m_pages.size()))
        {
            SIZE size = getThumbRenderSize(m_dragSourceIndex);
            int ghostWidth = std::max(36, static_cast<int>(size.cx) * 2 / 3);
            int ghostHeight = std::max(48, static_cast<int>(size.cy) * 2 / 3);
            RECT ghostRect = PdfReaderHelper::MakeRect(m_dragPoint.x + 14,
                                      m_dragPoint.y + 10,
                                      m_dragPoint.x + 14 + ghostWidth,
                                      m_dragPoint.y + 10 + ghostHeight);
            const PdfEngineBitmap* bitmap = getCachedBitmap(m_dragSourceIndex, size.cx, size.cy, &m_thumbCache);
            if (bitmap != nullptr)
            {
                drawPageBitmap(hdc, *bitmap, ghostRect, 190);
            }
            else
            {
                PdfReaderHelper::FillSolidRect(hdc, ghostRect, PdfReaderHelper::Color(255, 255, 255));
            }
            PdfReaderHelper::DrawSolidFrame(hdc, ghostRect, PdfReaderHelper::Color(48, 111, 210));
        }
    }

    SelectClipRgn(hdc, nullptr);
}

void PdfReaderWindow::paintDocumentView(HDC hdc)
{
    RECT client;
    GetClientRect(m_documentView, &client);
    PdfReaderHelper::FillSolidRect(hdc, client, PdfReaderHelper::Color(203, 207, 215));
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, PdfReaderHelper::GetUiFont());

    if (m_pages.empty())
    {
        SetTextColor(hdc, PdfReaderHelper::Color(99, 107, 120));
        DrawTextW(hdc, L"Open PDF", -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }

    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        RECT pageRect = getDocumentPageRect(i);
        if (pageRect.top > client.bottom || pageRect.bottom < client.top)
        {
            continue;
        }

        RECT shadowRect = pageRect;
        OffsetRect(&shadowRect, 4, 4);
        PdfReaderHelper::FillSolidRect(hdc, shadowRect, PdfReaderHelper::Color(178, 183, 193));

        const PdfEngineBitmap* bitmap = getCachedBitmap(i, PdfReaderHelper::RectWidth(pageRect), PdfReaderHelper::RectHeight(pageRect), &m_documentCache);
        if (bitmap != nullptr)
        {
            drawPageBitmap(hdc, *bitmap, pageRect, 255);
        }
        else
        {
            PdfReaderHelper::FillSolidRect(hdc, pageRect, PdfReaderHelper::Color(255, 255, 255));
        }
        PdfReaderHelper::DrawSolidFrame(hdc, pageRect, i == m_selectedPage ? PdfReaderHelper::Color(48, 111, 210) : PdfReaderHelper::Color(128, 134, 145));
    }
}

void PdfReaderWindow::drawPageBitmap(HDC hdc, const PdfEngineBitmap& bitmap, const RECT& targetRect, BYTE alpha)
{
    if (bitmap.width <= 0 || bitmap.height <= 0 || bitmap.pixels.empty())
    {
        PdfReaderHelper::FillSolidRect(hdc, targetRect, PdfReaderHelper::Color(255, 255, 255));
        return;
    }

    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = bitmap.width;
    info.bmiHeader.biHeight = -bitmap.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    if (alpha == 255)
    {
        SetStretchBltMode(hdc, HALFTONE);
        StretchDIBits(hdc,
                      targetRect.left,
                      targetRect.top,
                      PdfReaderHelper::RectWidth(targetRect),
                      PdfReaderHelper::RectHeight(targetRect),
                      0,
                      0,
                      bitmap.width,
                      bitmap.height,
                      &bitmap.pixels[0],
                      &info,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        return;
    }

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib == nullptr || bits == nullptr)
    {
        return;
    }

    int targetStride = bitmap.width * 4;
    unsigned char* targetBits = reinterpret_cast<unsigned char*>(bits);
    for (int y = 0; y < bitmap.height; ++y)
    {
        const unsigned char* source = &bitmap.pixels[static_cast<size_t>(bitmap.stride) * static_cast<size_t>(y)];
        memcpy(targetBits + static_cast<size_t>(targetStride) * static_cast<size_t>(y),
               source,
               static_cast<size_t>(std::min(targetStride, bitmap.stride)));
    }

    HDC memDc = CreateCompatibleDC(hdc);
    HGDIOBJ oldBitmap = SelectObject(memDc, dib);
    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = alpha;
    blend.AlphaFormat = 0;
    AlphaBlend(hdc,
               targetRect.left,
               targetRect.top,
               PdfReaderHelper::RectWidth(targetRect),
               PdfReaderHelper::RectHeight(targetRect),
               memDc,
               0,
               0,
               bitmap.width,
               bitmap.height,
               blend);
    SelectObject(memDc, oldBitmap);
    DeleteDC(memDc);
    DeleteObject(dib);
}

PdfEnginePageSize PdfReaderWindow::getPageSize(int pageOrder) const
{
    PdfEnginePageSize pageSize;
    pageSize.width = 612.0;
    pageSize.height = 792.0;
    if (pageOrder < 0 || pageOrder >= static_cast<int>(m_pages.size()))
    {
        return pageSize;
    }

    PdfEnginePageSize actualSize;
    if (m_pages[pageOrder].document != nullptr &&
        m_pages[pageOrder].document->getPageSize(m_pages[pageOrder].pageIndex, &actualSize) &&
        actualSize.width > 0.0 &&
        actualSize.height > 0.0)
    {
        pageSize = actualSize;
    }
    return pageSize;
}

SIZE PdfReaderWindow::getThumbRenderSize(int pageOrder) const
{
    RECT client;
    GetClientRect(m_thumbView, &client);
    int availableWidth = std::max(64, PdfReaderHelper::RectWidth(client) - kThumbPadding * 2 - GetSystemMetrics(SM_CXVSCROLL));
    int targetWidth = std::max(42, 112 * m_thumbScalePercent / 100);
    targetWidth = std::min(targetWidth, availableWidth);

    PdfEnginePageSize pageSize = getPageSize(pageOrder);
    int targetHeight = static_cast<int>(targetWidth * pageSize.height / pageSize.width + 0.5);
    SIZE size = {targetWidth, std::max(54, targetHeight)};
    return size;
}

SIZE PdfReaderWindow::getDocumentRenderSize(int pageOrder) const
{
    PdfEnginePageSize pageSize = getPageSize(pageOrder);
    int width = std::max(80, static_cast<int>(pageSize.width * m_documentZoomPercent / 100.0 + 0.5));
    int height = std::max(80, static_cast<int>(pageSize.height * m_documentZoomPercent / 100.0 + 0.5));
    SIZE size = {width, height};
    return size;
}

int PdfReaderWindow::getThumbItemHeight(int pageOrder) const
{
    SIZE size = getThumbRenderSize(pageOrder);
    return size.cy + kThumbLabelHeight + kThumbGap;
}

int PdfReaderWindow::getThumbItemTop(int pageOrder) const
{
    int top = kThumbPadding;
    for (int i = 0; i < pageOrder; ++i)
    {
        top += getThumbItemHeight(i);
    }
    return top;
}

int PdfReaderWindow::getThumbTotalHeight() const
{
    int total = kThumbPadding;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        total += getThumbItemHeight(i);
    }
    total += kThumbPadding;
    return total;
}

RECT PdfReaderWindow::getThumbPageRect(int pageOrder) const
{
    RECT client;
    GetClientRect(m_thumbView, &client);
    SIZE size = getThumbRenderSize(pageOrder);
    int x = std::max(kThumbPadding / 2, (PdfReaderHelper::RectWidth(client) - GetSystemMetrics(SM_CXVSCROLL) - static_cast<int>(size.cx)) / 2);
    int y = getThumbItemTop(pageOrder) + kThumbGap / 2;
    return PdfReaderHelper::MakeRect(x, y, x + size.cx, y + size.cy);
}

int PdfReaderWindow::getInsertLineY(int insertIndex) const
{
    if (insertIndex <= 0)
    {
        return getThumbItemTop(0);
    }
    if (insertIndex >= static_cast<int>(m_pages.size()))
    {
        int last = static_cast<int>(m_pages.size()) - 1;
        return getThumbItemTop(last) + getThumbItemHeight(last);
    }
    return getThumbItemTop(insertIndex);
}

int PdfReaderWindow::getDocumentTotalHeight() const
{
    if (m_pages.empty())
    {
        return 0;
    }

    int total = kDocPadding;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        total += getDocumentRenderSize(i).cy + kDocGap;
    }
    total += kDocPadding;
    return total;
}

int PdfReaderWindow::getDocumentTotalWidth() const
{
    int width = 0;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        width = std::max(width, static_cast<int>(getDocumentRenderSize(i).cx));
    }
    return width + kDocPadding * 2;
}

RECT PdfReaderWindow::getDocumentPageRect(int pageOrder) const
{
    RECT client;
    GetClientRect(m_documentView, &client);
    SIZE size = getDocumentRenderSize(pageOrder);
    int contentWidth = getDocumentTotalWidth();
    int viewWidth = PdfReaderHelper::RectWidth(client);
    int pageLeftInContent = std::max(kDocPadding, (contentWidth - static_cast<int>(size.cx)) / 2);
    int x = pageLeftInContent - m_documentScrollX;
    int y = kDocPadding - m_documentScrollY;
    for (int i = 0; i < pageOrder; ++i)
    {
        y += getDocumentRenderSize(i).cy + kDocGap;
    }

    if (contentWidth < viewWidth)
    {
        x = std::max(kDocPadding, (viewWidth - static_cast<int>(size.cx)) / 2);
    }
    return PdfReaderHelper::MakeRect(x, y, x + size.cx, y + size.cy);
}

const PdfEngineBitmap* PdfReaderWindow::getCachedBitmap(int pageOrder, int width, int height, std::vector<BitmapCacheEntry>* cache)
{
    if (cache == nullptr || pageOrder < 0 || pageOrder >= static_cast<int>(m_pages.size()) || width <= 0 || height <= 0)
    {
        return nullptr;
    }

    const PageItem& item = m_pages[pageOrder];
    for (size_t i = 0; i < cache->size(); ++i)
    {
        BitmapCacheEntry& entry = (*cache)[i];
        if (entry.document == item.document && entry.pageIndex == item.pageIndex && entry.width == width && entry.height == height)
        {
            return &entry.bitmap;
        }
    }

    BitmapCacheEntry entry;
    entry.document = item.document;
    entry.pageIndex = item.pageIndex;
    entry.width = width;
    entry.height = height;
    std::string error;
    if (item.document == nullptr || !item.document->renderPageToBgra(item.pageIndex, width, height, &entry.bitmap, &error))
    {
        PdfReaderInstance::instance().logError("Render page failed, order=%d, sourcePage=%d, size=%dx%d, error=%s",
                            pageOrder,
                            item.pageIndex + 1,
                            width,
                            height,
                            error.c_str());
        return nullptr;
    }

    cache->push_back(entry);
    if (cache->size() > 96)
    {
        PdfReaderInstance::instance().logInfo("Render cache limit reached, erase oldest cache item, size=%u",
                           static_cast<unsigned int>(cache->size()));
        cache->erase(cache->begin());
    }
    PdfReaderInstance::instance().logInfo("Render page cache added, order=%d, sourcePage=%d, size=%dx%d, cacheSize=%u",
                       pageOrder,
                       item.pageIndex + 1,
                       width,
                       height,
                       static_cast<unsigned int>(cache->size()));
    return &cache->back().bitmap;
}

int PdfReaderWindow::hitTestThumbnail(POINT point) const
{
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        int top = getThumbItemTop(i) - m_thumbScrollY;
        int bottom = top + getThumbItemHeight(i);
        if (point.y >= top && point.y < bottom)
        {
            return i;
        }
    }
    return -1;
}

int PdfReaderWindow::hitTestDocumentPage(POINT point) const
{
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        RECT pageRect = getDocumentPageRect(i);
        if (PdfReaderHelper::PtInRectLocal(pageRect, point))
        {
            return i;
        }
    }
    return -1;
}

int PdfReaderWindow::getInsertIndexForPoint(POINT point) const
{
    if (m_pages.empty())
    {
        return 0;
    }

    int contentY = point.y + m_thumbScrollY;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        int top = getThumbItemTop(i);
        int height = getThumbItemHeight(i);
        if (contentY < top + height / 2)
        {
            return i;
        }
        if (contentY < top + height)
        {
            return i + 1;
        }
    }
    return static_cast<int>(m_pages.size());
}

void PdfReaderWindow::beginThumbnailDrag(int pageOrder, POINT point)
{
    PdfReaderInstance::instance().logInfo("Thumbnail drag begin, sourceOrder=%d, mouse=(%ld,%ld)",
                       pageOrder,
                       point.x,
                       point.y);
    m_draggingThumb = true;
    m_dragSourceIndex = pageOrder;
    m_dragInsertIndex = pageOrder;
    m_dragPoint = point;
    SetCapture(m_thumbView);
    SetTimer(m_thumbView, kTimerDragScroll, 16, nullptr);
    InvalidateRect(m_thumbView, nullptr, FALSE);
}

void PdfReaderWindow::updateThumbnailDrag(POINT point)
{
    m_dragPoint = point;
    m_dragInsertIndex = PdfReaderHelper::ClampInt(getInsertIndexForPoint(point), 0, static_cast<int>(m_pages.size()));
    InvalidateRect(m_thumbView, nullptr, FALSE);
}

void PdfReaderWindow::finishThumbnailDrag(bool applyMove)
{
    KillTimer(m_thumbView, kTimerDragScroll);
    int oldSourceIndex = m_dragSourceIndex;
    int oldInsertIndex = m_dragInsertIndex;
    int finalIndex = oldSourceIndex;
    bool moved = false;
    if (applyMove &&
        m_dragSourceIndex >= 0 &&
        m_dragSourceIndex < static_cast<int>(m_pages.size()) &&
        m_dragInsertIndex >= 0 &&
        m_dragInsertIndex <= static_cast<int>(m_pages.size()) &&
        m_dragInsertIndex != m_dragSourceIndex &&
        m_dragInsertIndex != m_dragSourceIndex + 1)
    {
        PageItem page = m_pages[m_dragSourceIndex];
        m_pages.erase(m_pages.begin() + m_dragSourceIndex);
        int targetIndex = m_dragInsertIndex;
        if (targetIndex > m_dragSourceIndex)
        {
            --targetIndex;
        }
        targetIndex = PdfReaderHelper::ClampInt(targetIndex, 0, static_cast<int>(m_pages.size()));
        m_pages.insert(m_pages.begin() + targetIndex, page);
        m_selectedPage = targetIndex;
        m_documentCache.clear();
        finalIndex = targetIndex;
        moved = true;
    }

    PdfReaderInstance::instance().logInfo("Thumbnail drag end, applyMove=%d, moved=%d, sourceOrder=%d, insertIndex=%d, finalOrder=%d, pageCount=%u",
                       applyMove ? 1 : 0,
                       moved ? 1 : 0,
                       oldSourceIndex,
                       oldInsertIndex,
                       finalIndex,
                       static_cast<unsigned int>(m_pages.size()));

    m_draggingThumb = false;
    m_dragSourceIndex = -1;
    m_dragInsertIndex = -1;
    clampScrollPositions();
    invalidateAll();
}

void PdfReaderWindow::autoScrollDrag()
{
    RECT client;
    GetClientRect(m_thumbView, &client);
    int zone = 70;
    int speed = 0;
    if (m_dragPoint.y < zone)
    {
        int distance = zone - m_dragPoint.y;
        speed = -PdfReaderHelper::ClampInt(4 + distance / 3, 4, 46);
    }
    else if (m_dragPoint.y > client.bottom - zone)
    {
        int distance = m_dragPoint.y - (client.bottom - zone);
        speed = PdfReaderHelper::ClampInt(4 + distance / 3, 4, 46);
    }

    if (speed != 0)
    {
        scrollThumbBy(speed);
        m_dragInsertIndex = PdfReaderHelper::ClampInt(getInsertIndexForPoint(m_dragPoint), 0, static_cast<int>(m_pages.size()));
        InvalidateRect(m_thumbView, nullptr, FALSE);
    }
}

void PdfReaderWindow::scrollThumbBy(int delta)
{
    m_thumbScrollY += delta;
    updateThumbScrollbar();
    InvalidateRect(m_thumbView, nullptr, FALSE);
}

void PdfReaderWindow::scrollDocumentBy(int dx, int dy)
{
    m_documentScrollX += dx;
    m_documentScrollY += dy;
    updateDocumentScrollbars();
    InvalidateRect(m_documentView, nullptr, FALSE);
}

void PdfReaderWindow::setThumbnailScale(int scalePercent)
{
    int clamped = PdfReaderHelper::ClampInt(scalePercent, kMinThumbScale, kMaxThumbScale);
    if (clamped == m_thumbScalePercent)
    {
        return;
    }
    int oldScalePercent = m_thumbScalePercent;
    m_thumbScalePercent = clamped;
    m_thumbCache.clear();
    PdfReaderInstance::instance().logInfo("Thumbnail scale changed, old=%d, new=%d", oldScalePercent, m_thumbScalePercent);
    updateThumbScrollbar();
    InvalidateRect(m_thumbView, nullptr, FALSE);
}

void PdfReaderWindow::setDocumentZoom(int zoomPercent)
{
    int clamped = PdfReaderHelper::ClampInt(zoomPercent, kMinDocZoom, kMaxDocZoom);
    if (clamped == m_documentZoomPercent)
    {
        return;
    }
    int oldZoomPercent = m_documentZoomPercent;
    m_documentZoomPercent = clamped;
    m_documentCache.clear();
    PdfReaderInstance::instance().logInfo("Document zoom changed, old=%d, new=%d", oldZoomPercent, m_documentZoomPercent);
    updateDocumentScrollbars();
    InvalidateRect(m_documentView, nullptr, FALSE);
}

void PdfReaderWindow::showThumbnailContextMenu(POINT clientPoint)
{
    if (m_draggingThumb)
    {
        return;
    }

    m_contextPage = hitTestThumbnail(clientPoint);
    PdfReaderInstance::instance().logInfo("Thumbnail context menu open, mouse=(%ld,%ld), contextPage=%d",
                       clientPoint.x,
                       clientPoint.y,
                       m_contextPage);
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr)
    {
        PdfReaderInstance::instance().logError("Create thumbnail context menu failed, lastError=%lu", GetLastError());
        return;
    }

    if (m_contextPage >= 0)
    {
        AppendMenuW(menu, MF_STRING, IDM_THUMB_INSERT_BEFORE, L"Insert PDF Before");
        AppendMenuW(menu, MF_STRING, IDM_THUMB_INSERT_AFTER, L"Insert PDF After");
    }
    else
    {
        AppendMenuW(menu, MF_STRING | (m_pages.empty() ? MF_GRAYED : 0), IDM_LEFT_SAVE_EACH_PAGE, L"Save Each Page");
        AppendMenuW(menu, MF_STRING | (m_pages.empty() ? MF_GRAYED : 0), IDM_LEFT_SAVE_PAGE_RANGE, L"Save Page Range");
    }

    POINT screenPoint = clientPoint;
    ClientToScreen(m_thumbView, &screenPoint);
    int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, screenPoint.x, screenPoint.y, 0, m_hwnd, nullptr);
    DestroyMenu(menu);
    PdfReaderInstance::instance().logInfo("Thumbnail context menu closed, command=%d", command);

    if (command == IDM_THUMB_INSERT_BEFORE && m_contextPage >= 0)
    {
        insertPdfCommand(m_contextPage, false);
    }
    else if (command == IDM_THUMB_INSERT_AFTER && m_contextPage >= 0)
    {
        insertPdfCommand(m_contextPage, true);
    }
    else if (command == IDM_LEFT_SAVE_EACH_PAGE)
    {
        saveEachPageCommand();
    }
    else if (command == IDM_LEFT_SAVE_PAGE_RANGE)
    {
        savePageRangeCommand();
    }
}

bool PdfReaderWindow::showOpenPdfDialog(std::wstring* filePath)
{
    if (filePath == nullptr)
    {
        PdfReaderInstance::instance().logError("Show open PDF dialog failed, output path is null");
        return false;
    }

    wchar_t buffer[MAX_PATH] = {0};
    OPENFILENAMEW openFile;
    ZeroMemory(&openFile, sizeof(openFile));
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = m_hwnd;
    openFile.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    openFile.lpstrFile = buffer;
    openFile.nMaxFile = MAX_PATH;
    openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    openFile.lpstrDefExt = L"pdf";
    if (!GetOpenFileNameW(&openFile))
    {
        PdfReaderInstance::instance().logInfo("Open PDF dialog canceled or failed, CommDlgExtendedError=%lu", CommDlgExtendedError());
        return false;
    }

    *filePath = buffer;
    PdfReaderInstance::instance().logInfo("Open PDF dialog selected, path=%s", PdfReaderHelper::WideToUtf8(*filePath).c_str());
    return true;
}

bool PdfReaderWindow::showSavePdfDialog(const std::wstring& suggestedName, std::wstring* filePath)
{
    if (filePath == nullptr)
    {
        PdfReaderInstance::instance().logError("Show save PDF dialog failed, output path is null");
        return false;
    }

    wchar_t buffer[MAX_PATH] = {0};
    if (!suggestedName.empty())
    {
        StringCchCopyW(buffer, MAX_PATH, suggestedName.c_str());
    }

    OPENFILENAMEW saveFile;
    ZeroMemory(&saveFile, sizeof(saveFile));
    saveFile.lStructSize = sizeof(saveFile);
    saveFile.hwndOwner = m_hwnd;
    saveFile.lpstrFilter = L"PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    saveFile.lpstrFile = buffer;
    saveFile.nMaxFile = MAX_PATH;
    saveFile.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    saveFile.lpstrDefExt = L"pdf";
    if (!GetSaveFileNameW(&saveFile))
    {
        PdfReaderInstance::instance().logInfo("Save PDF dialog canceled or failed, suggestedName=%s, CommDlgExtendedError=%lu",
                           PdfReaderHelper::WideToUtf8(suggestedName).c_str(),
                           CommDlgExtendedError());
        return false;
    }

    *filePath = buffer;
    PdfReaderInstance::instance().logInfo("Save PDF dialog selected, suggestedName=%s, path=%s",
                       PdfReaderHelper::WideToUtf8(suggestedName).c_str(),
                       PdfReaderHelper::WideToUtf8(*filePath).c_str());
    return true;
}

bool PdfReaderWindow::showFolderDialog(std::wstring* folderPath)
{
    if (folderPath == nullptr)
    {
        PdfReaderInstance::instance().logError("Show folder dialog failed, output path is null");
        return false;
    }

    BROWSEINFOW browse;
    ZeroMemory(&browse, sizeof(browse));
    browse.hwndOwner = m_hwnd;
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    browse.lpszTitle = L"Select output folder";
    LPITEMIDLIST item = SHBrowseForFolderW(&browse);
    if (item == nullptr)
    {
        PdfReaderInstance::instance().logInfo("Folder dialog canceled");
        return false;
    }

    wchar_t buffer[MAX_PATH] = {0};
    bool ok = SHGetPathFromIDListW(item, buffer) ? true : false;
    CoTaskMemFree(item);
    if (!ok)
    {
        PdfReaderInstance::instance().logError("SHGetPathFromIDListW failed after folder selected, lastError=%lu", GetLastError());
        return false;
    }

    *folderPath = buffer;
    PdfReaderInstance::instance().logInfo("Folder dialog selected, path=%s", PdfReaderHelper::WideToUtf8(*folderPath).c_str());
    return true;
}

bool PdfReaderWindow::showTextPrompt(const std::wstring& title,
                                     const std::wstring& label,
                                     const std::wstring& initialValue,
                                     bool password,
                                     std::wstring* value)
{
    if (value == nullptr)
    {
        PdfReaderInstance::instance().logError("Show text prompt failed, output value is null, title=%s",
                            PdfReaderHelper::WideToUtf8(title).c_str());
        return false;
    }

    PdfReaderInstance::instance().logInfo("Show text prompt begin, title=%s, password=%d, initialLength=%u",
                       PdfReaderHelper::WideToUtf8(title).c_str(),
                       password ? 1 : 0,
                       static_cast<unsigned int>(initialValue.size()));

    PromptState state;
    state.instance = m_instance;
    state.owner = m_hwnd;
    state.hwnd = nullptr;
    state.edit = nullptr;
    state.title = title;
    state.label = label;
    state.initialValue = initialValue;
    state.password = password;
    state.accepted = false;
    state.done = false;

    HWND prompt = CreateWindowExW(WS_EX_DLGMODALFRAME,
                                  kPromptClass,
                                  title.c_str(),
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  360,
                                  150,
                                  m_hwnd,
                                  nullptr,
                                  m_instance,
                                  &state);
    if (prompt == nullptr)
    {
        PdfReaderInstance::instance().logError("Create text prompt failed, title=%s, lastError=%lu",
                            PdfReaderHelper::WideToUtf8(title).c_str(),
                            GetLastError());
        return false;
    }

    RECT ownerRect;
    RECT promptRect;
    GetWindowRect(m_hwnd, &ownerRect);
    GetWindowRect(prompt, &promptRect);
    int x = ownerRect.left + (PdfReaderHelper::RectWidth(ownerRect) - PdfReaderHelper::RectWidth(promptRect)) / 2;
    int y = ownerRect.top + (PdfReaderHelper::RectHeight(ownerRect) - PdfReaderHelper::RectHeight(promptRect)) / 2;
    SetWindowPos(prompt, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    EnableWindow(m_hwnd, FALSE);
    ShowWindow(prompt, SW_SHOW);
    UpdateWindow(prompt);

    MSG message;
    while (!state.done)
    {
        BOOL messageResult = GetMessageW(&message, nullptr, 0, 0);
        if (messageResult <= 0)
        {
            if (messageResult == 0)
            {
                PostQuitMessage(static_cast<int>(message.wParam));
            }
            break;
        }

        if (state.hwnd == nullptr || !IsDialogMessageW(state.hwnd, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(m_hwnd, TRUE);
    SetForegroundWindow(m_hwnd);
    if (state.accepted)
    {
        *value = state.value;
    }
    PdfReaderInstance::instance().logInfo("Show text prompt end, title=%s, accepted=%d, valueLength=%u",
                       PdfReaderHelper::WideToUtf8(title).c_str(),
                       state.accepted ? 1 : 0,
                       state.accepted ? static_cast<unsigned int>(state.value.size()) : 0);
    return state.accepted;
}

void PdfReaderWindow::showAboutDialog()
{
    PdfReaderInstance::instance().logInfo("Show about dialog begin");

    AboutDialogState state;
    state.instance = m_instance;
    state.hwnd = nullptr;
    state.done = false;

    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME,
                                  kAboutClass,
                                  L"关于PDF阅读器",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  430,
                                  292,
                                  m_hwnd,
                                  nullptr,
                                  m_instance,
                                  &state);
    if (dialog == nullptr)
    {
        PdfReaderInstance::instance().logError("Create about dialog failed, lastError=%lu", GetLastError());
        return;
    }

    RECT ownerRect;
    RECT dialogRect;
    GetWindowRect(m_hwnd, &ownerRect);
    GetWindowRect(dialog, &dialogRect);
    int x = ownerRect.left + (PdfReaderHelper::RectWidth(ownerRect) - PdfReaderHelper::RectWidth(dialogRect)) / 2;
    int y = ownerRect.top + (PdfReaderHelper::RectHeight(ownerRect) - PdfReaderHelper::RectHeight(dialogRect)) / 2;
    SetWindowPos(dialog, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

    EnableWindow(m_hwnd, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG message;
    while (!state.done)
    {
        BOOL messageResult = GetMessageW(&message, nullptr, 0, 0);
        if (messageResult <= 0)
        {
            if (messageResult == 0)
            {
                PostQuitMessage(static_cast<int>(message.wParam));
            }
            break;
        }

        if (state.hwnd == nullptr || !IsDialogMessageW(state.hwnd, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    EnableWindow(m_hwnd, TRUE);
    SetForegroundWindow(m_hwnd);
    PdfReaderInstance::instance().logInfo("Show about dialog end");
}

PdfDocument* PdfReaderWindow::openDocumentWithPassword(const std::wstring& filePath)
{
    PdfReaderInstance::instance().logInfo("Open document begin, path=%s", PdfReaderHelper::WideToUtf8(filePath).c_str());

    if (!m_engineReady)
    {
        PdfReaderInstance::instance().logError("Open document failed, PdfEngine is not initialized, path=%s",
                            PdfReaderHelper::WideToUtf8(filePath).c_str());
        MessageBoxW(m_hwnd, L"PdfEngine is not initialized.", kAppTitle, MB_OK | MB_ICONERROR);
        return nullptr;
    }

    PdfDocument* document = new PdfDocument();
    std::string error;
    if (document->open(filePath, &error))
    {
        PdfReaderInstance::instance().logInfo("Open document succeeded without password, path=%s, pageCount=%d",
                           PdfReaderHelper::WideToUtf8(filePath).c_str(),
                           document->pageCount());
        return document;
    }

    if (error.find("password") == std::string::npos)
    {
        PdfReaderInstance::instance().logError("Open document failed, path=%s, error=%s",
                            PdfReaderHelper::WideToUtf8(filePath).c_str(),
                            error.c_str());
        MessageBoxW(m_hwnd, PdfReaderHelper::Utf8ToWide(error).c_str(), kAppTitle, MB_OK | MB_ICONERROR);
        delete document;
        return nullptr;
    }

    PdfReaderInstance::instance().logWarning("Open document needs password, path=%s, error=%s",
                          PdfReaderHelper::WideToUtf8(filePath).c_str(),
                          error.c_str());
    for (;;)
    {
        std::wstring passwordText;
        if (!showTextPrompt(L"PDF Password", L"Password", L"", true, &passwordText))
        {
            PdfReaderInstance::instance().logInfo("Open document canceled at password prompt, path=%s",
                               PdfReaderHelper::WideToUtf8(filePath).c_str());
            delete document;
            return nullptr;
        }

        if (document->open(filePath, PdfReaderHelper::WideToUtf8(passwordText), &error))
        {
            PdfReaderInstance::instance().logInfo("Open document succeeded with password, path=%s, pageCount=%d",
                               PdfReaderHelper::WideToUtf8(filePath).c_str(),
                               document->pageCount());
            return document;
        }

        if (error.find("password") == std::string::npos)
        {
            PdfReaderInstance::instance().logError("Open password protected document failed after password input, path=%s, error=%s",
                                PdfReaderHelper::WideToUtf8(filePath).c_str(),
                                error.c_str());
            MessageBoxW(m_hwnd, PdfReaderHelper::Utf8ToWide(error).c_str(), kAppTitle, MB_OK | MB_ICONERROR);
            delete document;
            return nullptr;
        }

        PdfReaderInstance::instance().logWarning("Open document password incorrect, path=%s, error=%s",
                              PdfReaderHelper::WideToUtf8(filePath).c_str(),
                              error.c_str());
        MessageBoxW(m_hwnd, L"Password is required or incorrect.", kAppTitle, MB_OK | MB_ICONWARNING);
    }
}

bool PdfReaderWindow::openPdfFile(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        PdfReaderInstance::instance().logWarning("Open PDF file ignored, empty path");
        return false;
    }

    PdfReaderInstance::instance().logInfo("Open PDF file begin, path=%s", PdfReaderHelper::WideToUtf8(filePath).c_str());
    PdfDocument* document = openDocumentWithPassword(filePath);
    if (document == nullptr)
    {
        PdfReaderInstance::instance().logError("Open PDF file failed, path=%s", PdfReaderHelper::WideToUtf8(filePath).c_str());
        return false;
    }

    clearDocuments();
    m_documents.push_back(document);
    m_mainFilePath = filePath;

    int pageCount = document->pageCount();
    for (int i = 0; i < pageCount; ++i)
    {
        PageItem page;
        page.document = document;
        page.pageIndex = i;
        page.sourcePath = filePath;
        m_pages.push_back(page);
    }

    m_selectedPage = m_pages.empty() ? -1 : 0;
    m_thumbScrollY = 0;
    m_documentScrollX = 0;
    m_documentScrollY = 0;
    clearRenderCaches();
    updateScrollbars();
    invalidateAll();
    PdfReaderInstance::instance().logInfo("Open PDF file succeeded, path=%s, pageCount=%d",
                       PdfReaderHelper::WideToUtf8(filePath).c_str(),
                       pageCount);
    return true;
}

void PdfReaderWindow::openPdfCommand()
{
    PdfReaderInstance::instance().logInfo("Open PDF command begin");
    std::wstring filePath;
    if (!showOpenPdfDialog(&filePath))
    {
        PdfReaderInstance::instance().logInfo("Open PDF command canceled");
        return;
    }

    openPdfFile(filePath);
}

void PdfReaderWindow::insertPdfCommand(int pageOrder, bool insertAfter)
{
    PdfReaderInstance::instance().logInfo("Insert PDF command begin, pageOrder=%d, insertAfter=%d, currentPageCount=%u",
                       pageOrder,
                       insertAfter ? 1 : 0,
                       static_cast<unsigned int>(m_pages.size()));

    if (pageOrder < 0 || pageOrder >= static_cast<int>(m_pages.size()))
    {
        PdfReaderInstance::instance().logWarning("Insert PDF command ignored, invalid pageOrder=%d, pageCount=%u",
                              pageOrder,
                              static_cast<unsigned int>(m_pages.size()));
        return;
    }

    std::wstring filePath;
    if (!showOpenPdfDialog(&filePath))
    {
        PdfReaderInstance::instance().logInfo("Insert PDF command canceled at open dialog");
        return;
    }

    PdfDocument* document = openDocumentWithPassword(filePath);
    if (document == nullptr)
    {
        PdfReaderInstance::instance().logError("Insert PDF command failed to open insert document, path=%s",
                            PdfReaderHelper::WideToUtf8(filePath).c_str());
        return;
    }

    m_documents.push_back(document);
    int insertIndex = insertAfter ? pageOrder + 1 : pageOrder;
    int insertPageCount = document->pageCount();
    for (int i = 0; i < document->pageCount(); ++i)
    {
        PageItem page;
        page.document = document;
        page.pageIndex = i;
        page.sourcePath = filePath;
        m_pages.insert(m_pages.begin() + insertIndex + i, page);
    }

    m_selectedPage = insertIndex;
    clearRenderCaches();
    updateScrollbars();
    invalidateAll();
    PdfReaderInstance::instance().logInfo("Insert PDF command succeeded, path=%s, insertIndex=%d, insertedPages=%d, totalPages=%u",
                       PdfReaderHelper::WideToUtf8(filePath).c_str(),
                       insertIndex,
                       insertPageCount,
                       static_cast<unsigned int>(m_pages.size()));
}

bool PdfReaderWindow::buildPageRefs(const std::vector<int>& pageOrders, std::vector<PdfEnginePageRef>* refs) const
{
    if (refs == nullptr)
    {
        PdfReaderInstance::instance().logError("Build page refs failed, refs is null");
        return false;
    }

    refs->clear();
    for (size_t i = 0; i < pageOrders.size(); ++i)
    {
        int pageOrder = pageOrders[i];
        if (pageOrder < 0 || pageOrder >= static_cast<int>(m_pages.size()))
        {
            PdfReaderInstance::instance().logError("Build page refs failed, invalid pageOrder=%d, pageCount=%u",
                                pageOrder,
                                static_cast<unsigned int>(m_pages.size()));
            return false;
        }
        refs->push_back(PdfEnginePageRef(m_pages[pageOrder].document, m_pages[pageOrder].pageIndex));
    }
    PdfReaderInstance::instance().logInfo("Build page refs finished, inputCount=%u, outputCount=%u",
                       static_cast<unsigned int>(pageOrders.size()),
                       static_cast<unsigned int>(refs->size()));
    return !refs->empty();
}

void PdfReaderWindow::savePdfCommand()
{
    PdfReaderInstance::instance().logInfo("Save PDF command begin, pageCount=%u", static_cast<unsigned int>(m_pages.size()));
    if (m_pages.empty())
    {
        PdfReaderInstance::instance().logWarning("Save PDF command ignored, no pages");
        return;
    }

    std::vector<int> pageOrders;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        pageOrders.push_back(i);
    }

    std::vector<PdfEnginePageRef> refs;
    if (!buildPageRefs(pageOrders, &refs))
    {
        PdfReaderInstance::instance().logError("Save PDF command failed, buildPageRefs returned false");
        return;
    }

    std::wstring suggestedName = m_mainFilePath.empty() ? L"PdfReader_edited.pdf" : PdfReaderHelper::GetFileStem(m_mainFilePath) + L"_edited.pdf";
    std::wstring outputPath;
    if (!showSavePdfDialog(suggestedName, &outputPath))
    {
        PdfReaderInstance::instance().logInfo("Save PDF command canceled at save dialog");
        return;
    }

    std::string error;
    if (!m_engine.savePages(outputPath, refs, &error))
    {
        PdfReaderInstance::instance().logError("Save PDF command failed, outputPath=%s, refs=%u, error=%s",
                            PdfReaderHelper::WideToUtf8(outputPath).c_str(),
                            static_cast<unsigned int>(refs.size()),
                            error.c_str());
        MessageBoxW(m_hwnd, PdfReaderHelper::Utf8ToWide(error).c_str(), kAppTitle, MB_OK | MB_ICONERROR);
        return;
    }
    PdfReaderInstance::instance().logInfo("Save PDF command succeeded, outputPath=%s, refs=%u",
                       PdfReaderHelper::WideToUtf8(outputPath).c_str(),
                       static_cast<unsigned int>(refs.size()));
    MessageBoxW(m_hwnd, L"Saved.", kAppTitle, MB_OK | MB_ICONINFORMATION);
}

void PdfReaderWindow::saveEachPageCommand()
{
    PdfReaderInstance::instance().logInfo("Save each page command begin, pageCount=%u", static_cast<unsigned int>(m_pages.size()));
    if (m_pages.empty())
    {
        PdfReaderInstance::instance().logWarning("Save each page command ignored, no pages");
        return;
    }

    std::wstring folder;
    if (!showFolderDialog(&folder))
    {
        PdfReaderInstance::instance().logInfo("Save each page command canceled at folder dialog");
        return;
    }

    std::wstring baseName = m_mainFilePath.empty() ? L"PdfReader" : PdfReaderHelper::GetFileStem(m_mainFilePath);
    std::vector<std::wstring> conflicts;
    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        std::wstring outputPath = PdfReaderHelper::JoinPath(folder, PdfReaderHelper::MakePageFileName(baseName, i + 1));
        if (CSystem::fileExists(outputPath))
        {
            PdfReaderInstance::instance().logWarning("Save each page conflict found, outputPath=%s",
                                  PdfReaderHelper::WideToUtf8(outputPath).c_str());
            conflicts.push_back(outputPath);
        }
    }

    if (!conflicts.empty())
    {
        std::wstring message = L"Target files already exist. Overwrite them?\r\n";
        int previewCount = std::min(5, static_cast<int>(conflicts.size()));
        for (int i = 0; i < previewCount; ++i)
        {
            message += L"\r\n";
            message += conflicts[i];
        }
        if (static_cast<int>(conflicts.size()) > previewCount)
        {
            message += L"\r\n...";
        }

        int result = MessageBoxW(m_hwnd, message.c_str(), kAppTitle, MB_YESNO | MB_ICONQUESTION);
        if (result != IDYES)
        {
            PdfReaderInstance::instance().logInfo("Save each page command canceled by overwrite confirmation, conflictCount=%u",
                               static_cast<unsigned int>(conflicts.size()));
            MessageBoxW(m_hwnd, L"Save canceled. Rename existing files and retry.", kAppTitle, MB_OK | MB_ICONINFORMATION);
            return;
        }
        PdfReaderInstance::instance().logInfo("Save each page overwrite confirmed, conflictCount=%u",
                           static_cast<unsigned int>(conflicts.size()));
    }

    for (int i = 0; i < static_cast<int>(m_pages.size()); ++i)
    {
        std::vector<int> pageOrders;
        pageOrders.push_back(i);
        std::vector<PdfEnginePageRef> refs;
        if (!buildPageRefs(pageOrders, &refs))
        {
            PdfReaderInstance::instance().logError("Save each page command failed, buildPageRefs returned false, pageOrder=%d", i);
            return;
        }

        std::wstring outputPath = PdfReaderHelper::JoinPath(folder, PdfReaderHelper::MakePageFileName(baseName, i + 1));
        std::string error;
        if (!m_engine.savePages(outputPath, refs, &error))
        {
            PdfReaderInstance::instance().logError("Save each page command failed, pageOrder=%d, outputPath=%s, error=%s",
                                i,
                                PdfReaderHelper::WideToUtf8(outputPath).c_str(),
                                error.c_str());
            std::wstring message = L"Save failed:\r\n" + outputPath + L"\r\n" + PdfReaderHelper::Utf8ToWide(error);
            MessageBoxW(m_hwnd, message.c_str(), kAppTitle, MB_OK | MB_ICONERROR);
            return;
        }
        PdfReaderInstance::instance().logInfo("Save each page file succeeded, pageOrder=%d, outputPath=%s",
                           i,
                           PdfReaderHelper::WideToUtf8(outputPath).c_str());
    }
    PdfReaderInstance::instance().logInfo("Save each page command succeeded, folder=%s, savedCount=%u",
                       PdfReaderHelper::WideToUtf8(folder).c_str(),
                       static_cast<unsigned int>(m_pages.size()));
    MessageBoxW(m_hwnd, L"Saved.", kAppTitle, MB_OK | MB_ICONINFORMATION);
}

void PdfReaderWindow::savePageRangeCommand()
{
    PdfReaderInstance::instance().logInfo("Save page range command begin, pageCount=%u", static_cast<unsigned int>(m_pages.size()));
    if (m_pages.empty())
    {
        PdfReaderInstance::instance().logWarning("Save page range command ignored, no pages");
        return;
    }

    std::wstring initial = L"1-" + PdfReaderHelper::IntToWide(static_cast<int>(m_pages.size()));
    std::wstring rangeText;
    if (!showTextPrompt(L"Save Page Range", L"Pages, example: 3-5,7-8", initial, false, &rangeText))
    {
        PdfReaderInstance::instance().logInfo("Save page range command canceled at range prompt");
        return;
    }

    PdfReaderInstance::instance().logInfo("Save page range input, text=%s", PdfReaderHelper::WideToUtf8(rangeText).c_str());
    std::vector<int> pageOrders;
    std::string error;
    if (!PdfEngine::parsePageRanges(PdfReaderHelper::WideToUtf8(rangeText), static_cast<int32_t>(m_pages.size()), &pageOrders, &error))
    {
        PdfReaderInstance::instance().logWarning("Save page range parse failed, text=%s, pageCount=%u, error=%s",
                              PdfReaderHelper::WideToUtf8(rangeText).c_str(),
                              static_cast<unsigned int>(m_pages.size()),
                              error.c_str());
        MessageBoxW(m_hwnd, PdfReaderHelper::Utf8ToWide(error).c_str(), kAppTitle, MB_OK | MB_ICONWARNING);
        return;
    }
    PdfReaderInstance::instance().logInfo("Save page range parse succeeded, selectedPages=%u",
                       static_cast<unsigned int>(pageOrders.size()));

    std::vector<PdfEnginePageRef> refs;
    if (!buildPageRefs(pageOrders, &refs))
    {
        PdfReaderInstance::instance().logError("Save page range command failed, buildPageRefs returned false");
        return;
    }

    std::wstring suggestedName = m_mainFilePath.empty() ? L"PdfReader_pages.pdf" : PdfReaderHelper::GetFileStem(m_mainFilePath) + L"_pages.pdf";
    std::wstring outputPath;
    if (!showSavePdfDialog(suggestedName, &outputPath))
    {
        PdfReaderInstance::instance().logInfo("Save page range command canceled at save dialog");
        return;
    }

    std::string saveError;
    if (!m_engine.savePages(outputPath, refs, &saveError))
    {
        PdfReaderInstance::instance().logError("Save page range command failed, outputPath=%s, refs=%u, error=%s",
                            PdfReaderHelper::WideToUtf8(outputPath).c_str(),
                            static_cast<unsigned int>(refs.size()),
                            saveError.c_str());
        MessageBoxW(m_hwnd, PdfReaderHelper::Utf8ToWide(saveError).c_str(), kAppTitle, MB_OK | MB_ICONERROR);
        return;
    }
    PdfReaderInstance::instance().logInfo("Save page range command succeeded, outputPath=%s, refs=%u",
                       PdfReaderHelper::WideToUtf8(outputPath).c_str(),
                       static_cast<unsigned int>(refs.size()));
    MessageBoxW(m_hwnd, L"Saved.", kAppTitle, MB_OK | MB_ICONINFORMATION);
}
