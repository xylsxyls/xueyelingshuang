#include "SplitViewerWindow.h"

#include "Logger.h"
#include "TempFileGuard.h"

#include "CStringManager/CStringManagerAPI.h"

SplitViewerWindow::SplitViewerWindow()
    : hinst_(NULL),
    hwnd_(NULL),
    saveButton_(NULL),
    saveConfigButton_(NULL),
    newLayerButton_(NULL),
    fullScreenButton_(NULL),
    aboutButton_(NULL),
    tooltipWindow_(NULL),
    mainFont_(NULL),
    dragOutSourceFrame_(NULL),
    dragOutPreviewFrame_(NULL),
    mouseHook_(NULL),
    backBufferDc_(NULL),
    backBufferBitmap_(NULL),
    backBufferOldBitmap_(NULL),
    backBufferWidth_(0),
    backBufferHeight_(0),
    backBufferValid_(false),
    baseRoot_(new SplitViewerNode()),
    selectedLayer_(-1),
    borderVisible_(true),
    isFullScreen_(false),
    stageAspect_(4.0 / 3.0),
    defaultStageWidth_(640),
    defaultStageHeight_(360),
    savedStyle_(0),
    savedExStyle_(0),
    dragMode_(SPLITVIEWER_DRAG_NONE),
    dragNode_(NULL),
    dragRoot_(NULL),
    dragLayerIndex_(-1),
    dragResizeHit_(SPLITVIEWER_RESIZE_NONE),
    dragImageOffsetX_(0.0),
    dragImageOffsetY_(0.0),
    hoveredSplitter_(NULL),
    hoveredSplitterLayer_(-2),
    hoverResizeLayer_(-1),
    hoverResizeHit_(SPLITVIEWER_RESIZE_NONE),
    trackingMouse_(false),
    externalDragWindow_(NULL),
    externalDropLeaf_(NULL),
    pendingEmbedWindow_(NULL),
    pendingEmbedLeaf_(NULL),
    pendingEmbedLayer_(-1),
    embeddedDragWindow_(NULL),
    embeddedDragLeaf_(NULL),
    embeddedDragLayer_(-1),
    embeddedClickTick_(0),
    tipLayerIndex_(-2),
    tipTick_(0),
    hasPaintClip_(false)
{
    ZeroMemory(&savedPlacement_, sizeof(savedPlacement_));
    savedPlacement_.length = sizeof(savedPlacement_);
    ZeroMemory(&dragStartPoint_, sizeof(dragStartPoint_));
    ZeroMemory(&dragLayerStartRect_, sizeof(dragLayerStartRect_));
    ZeroMemory(&dragLastInvalidRect_, sizeof(dragLastInvalidRect_));
    ZeroMemory(&lastStageRect_, sizeof(lastStageRect_));
    ZeroMemory(&tipOwnerRect_, sizeof(tipOwnerRect_));
    ZeroMemory(&paintClipRect_, sizeof(paintClipRect_));
    ZeroMemory(&zoomFinishInvalidRect_, sizeof(zoomFinishInvalidRect_));
    ZeroMemory(&externalDragStartPoint_, sizeof(externalDragStartPoint_));
    ZeroMemory(&externalDragStartRect_, sizeof(externalDragStartRect_));
    ZeroMemory(&embeddedDragStartPoint_, sizeof(embeddedDragStartPoint_));
    ZeroMemory(&embeddedDragStartRect_, sizeof(embeddedDragStartRect_));
    ZeroMemory(&embeddedClickPoint_, sizeof(embeddedClickPoint_));
    SplitViewerDebugLog(L"SplitViewerWindow constructed.");
}

SplitViewerWindow::~SplitViewerWindow()
{
    SplitViewerDebugLog(L"SplitViewerWindow destroying.");
    UninstallMouseHook();
    ReleaseBackBuffer();
    DestroyEmbeddedResizePreviewFrames();
    DestroyInterfaceFont();
    delete baseRoot_;
    ClearLayers();
}

const wchar_t* SplitViewerWindow::DragModeText(SplitViewerDragMode mode) const
{
    switch (mode)
    {
    case SPLITVIEWER_DRAG_IMAGE:
        {
            return L"image";
        }
    case SPLITVIEWER_DRAG_SPLITTER:
        {
            return L"splitter";
        }
    case SPLITVIEWER_DRAG_LAYER_MOVE:
        {
            return L"layer_move";
        }
    case SPLITVIEWER_DRAG_LAYER_RESIZE:
        {
            return L"layer_resize";
        }
    case SPLITVIEWER_DRAG_NONE:
    default:
        {
            return L"none";
        }
    }
}

const wchar_t* SplitViewerWindow::SplitDirectionText(SplitViewerSplitDirection direction) const
{
    return direction == SPLITVIEWER_SPLIT_HORIZONTAL ? L"horizontal" : L"vertical";
}

bool SplitViewerWindow::Create(HINSTANCE hinst, int cmdShow)
{
    hinst_ = hinst;

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    defaultStageWidth_ = std::max(320, screenWidth / 2);
    defaultStageHeight_ = std::max(240, screenHeight / 2);
    stageAspect_ = static_cast<double>(defaultStageWidth_) / static_cast<double>(defaultStageHeight_);
    SplitViewerDebugLogFormat(L"Create window begin screen=%dx%d defaultStage=%dx%d aspect=%.6f cmdShow=%d.",
        screenWidth,
        screenHeight,
        defaultStageWidth_,
        defaultStageHeight_,
        stageAspect_,
        cmdShow);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = SplitViewerWindow::StaticWndProc;
    wc.hInstance = hinst_;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kSplitViewerWindowClass;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        SplitViewerDebugLogFormat(L"RegisterClassEx failed error=%u.", static_cast<unsigned int>(GetLastError()));
        return false;
    }

    RegisterDragFrameClass();
    RegisterAboutWindowClass();

    RECT clientRect = SplitViewerMakeRect(0,
        0,
        defaultStageWidth_ + kSplitViewerMargin * 2,
        kSplitViewerToolbarHeight + defaultStageHeight_ + kSplitViewerMargin * 2);
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    AdjustWindowRectEx(&clientRect, style, FALSE, 0);

    const int windowWidth = SplitViewerRectWidth(clientRect);
    const int windowHeight = SplitViewerRectHeight(clientRect);
    const int x = std::max(0, (screenWidth - windowWidth) / 2);
    const int y = std::max(0, (screenHeight - windowHeight) / 2);

    hwnd_ = CreateWindowExW(0,
        kSplitViewerWindowClass,
        kSplitViewerAppTitle,
        style,
        x,
        y,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        hinst_,
        this);
    if (!hwnd_)
    {
        SplitViewerDebugLogFormat(L"CreateWindowEx failed error=%u window=%dx%d pos=%d,%d.",
            static_cast<unsigned int>(GetLastError()),
            windowWidth,
            windowHeight,
            x,
            y);
        return false;
    }

    ShowWindow(hwnd_, cmdShow);
    UpdateWindow(hwnd_);
    SplitViewerDebugLogFormat(L"Create window success hwnd=0x%p window=%dx%d pos=%d,%d.",
        hwnd_,
        windowWidth,
        windowHeight,
        x,
        y);
    return true;
}

bool SplitViewerWindow::LoadStartupPath(const wchar_t* path)
{
    if (!path || !path[0])
    {
        SplitViewerDebugLog(L"LoadStartupPath skipped: empty path.");
        return false;
    }

    SplitViewerDebugLogFormat(L"LoadStartupPath begin path=%s.", path);
    if (SplitViewerIsConfigPath(path))
    {
        const bool loadedConfig = LoadConfigFromFile(path);
        SplitViewerDebugLogFormat(L"LoadStartupPath config result=%d path=%s.", loadedConfig ? 1 : 0, path);
        return loadedConfig;
    }

    currentConfigPath_.clear();
    SplitViewerNode* leaf = FirstLeaf(baseRoot_);
    if (!leaf || !leaf->view.Load(path))
    {
        SplitViewerDebugLogFormat(L"LoadStartupPath image failed path=%s.", path);
        return false;
    }

    UpdateForCurrentStage(false);
    UpdatePrimaryButtonText();
    UpdateMouseHookState();
    InvalidateRect(hwnd_, NULL, FALSE);
    SplitViewerDebugLogFormat(L"LoadStartupPath image success path=%s.", path);
    return true;
}

LRESULT CALLBACK SplitViewerWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    SplitViewerWindow* self = NULL;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = reinterpret_cast<SplitViewerWindow*>(cs->lpCreateParams);
        self->hwnd_ = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<SplitViewerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (self)
    {
        return self->WndProc(msg, wparam, lparam);
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK SplitViewerWindow::StaticDragFrameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_NCCREATE:
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return TRUE;
        }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);

            HBRUSH transparentBrush = CreateSolidBrush(kSplitViewerFrameTransparentColor);
            FillRect(hdc, &rc, transparentBrush);
            DeleteObject(transparentBrush);

            COLORREF color = static_cast<COLORREF>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (color == 0)
            {
                color = kSplitViewerExternalDragOutFrameColor;
            }

            HPEN pen = CreatePen(PS_SOLID, 3, color);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);

            EndPaint(hwnd, &ps);
            return 0;
        }
    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK SplitViewerWindow::StaticAboutWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    SplitViewerWindow* self = reinterpret_cast<SplitViewerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
        self = reinterpret_cast<SplitViewerWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }

    switch (msg)
    {
    case WM_CREATE:
        {
            SplitViewerDebugLog(L"About window WM_CREATE.");
            HINSTANCE instance = self ? self->hinst_ : GetModuleHandleW(NULL);
            HWND title = CreateWindowExW(0,
                L"STATIC",
                kSplitViewerAppTitle,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
                20,
                18,
                390,
                26,
                hwnd,
                NULL,
                instance,
                NULL);
            std::wstring versionText = L"\u7248\u672C\uFF1A";
            versionText += kSplitViewerAppVersion;
            HWND version = CreateWindowExW(0,
                L"STATIC",
                versionText.c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX,
                20,
                48,
                390,
                22,
                hwnd,
                NULL,
                instance,
                NULL);
            HWND content = CreateWindowExW(0,
                L"STATIC",
                L"\u652F\u6301\u591A\u56FE\u5206\u5C4F\u3001\u60AC\u6D6E\u56FE\u5C42\u3001\u56FE\u7247\u62D6\u52A8\u7F29\u653E\u3001\u914D\u7F6E\u4FDD\u5B58\u548C\u5916\u90E8\u7A97\u53E3\u5D4C\u5165\u3002\r\n\r\n\u70B9\u51FB+\u52A0\u8F7D\u56FE\u7247\u6216\u62D6\u5165\u7A97\u53E3\uFF1B\u53F3\u952E\u53EF\u5206\u5C4F\u6216\u5220\u9664\u5206\u5C4F\uFF1B\u6EDA\u8F6E\u7F29\u653E\uFF0CCtrl+\u6EDA\u8F6E\u7CBE\u7EC6\u7F29\u653E\uFF1BF1\u663E\u793A/\u9690\u85CF\u8FB9\u6846\uFF1B\u53CC\u51FB\u6216\u5168\u5C4F\u6309\u94AE\u8FDB\u5165\u5168\u5C4F\uFF0CEsc\u9000\u51FA\u3002",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                24,
                82,
                382,
                105,
                hwnd,
                NULL,
                instance,
                NULL);
            HWND okButton = CreateWindowExW(0,
                L"BUTTON",
                L"\u786E\u5B9A",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                170,
                198,
                88,
                28,
                hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDOK)),
                instance,
                NULL);
            if (self)
            {
                self->ApplyFont(title);
                self->ApplyFont(version);
                self->ApplyFont(content);
                self->ApplyFont(okButton);
            }
            return 0;
        }
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = reinterpret_cast<HDC>(wparam);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(32, 32, 32));
            return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
        }
    case WM_ERASEBKGND:
        {
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect(reinterpret_cast<HDC>(wparam), &rc, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
            return 1;
        }
    case WM_COMMAND:
        {
            WORD command = LOWORD(wparam);
            if (command == IDOK || command == IDCANCEL)
            {
                SplitViewerDebugLogFormat(L"About window close command=%u.", static_cast<unsigned int>(command));
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
    case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE)
            {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
    case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            return 0;
        }
    default:
        {
            break;
        }
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void SplitViewerWindow::RegisterDragFrameClass()
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SplitViewerWindow::StaticDragFrameWndProc;
    wc.hInstance = hinst_;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wc.lpszClassName = kSplitViewerDragFrameClass;
    RegisterClassExW(&wc);
}

void SplitViewerWindow::RegisterAboutWindowClass()
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SplitViewerWindow::StaticAboutWndProc;
    wc.hInstance = hinst_;
    wc.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszClassName = kSplitViewerAboutWindowClass;
    wc.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        SplitViewerDebugLogFormat(L"RegisterAboutWindowClass failed error=%u.", static_cast<unsigned int>(GetLastError()));
    }
}

LRESULT SplitViewerWindow::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CREATE:
        SplitViewerDebugLog(L"WM_CREATE.");
        CreateInterfaceFont();
        CreateToolbar();
        UpdateForCurrentStage(false);
        UpdateMouseHookState();
        return 0;
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED)
        {
            SplitViewerDebugLogFormat(L"WM_SIZE type=%u size=%dx%d.",
                static_cast<unsigned int>(wparam),
                LOWORD(lparam),
                HIWORD(lparam));
        }
        LayoutToolbar();
        UpdateForCurrentStage(true);
        InvalidateRect(hwnd_, NULL, FALSE);
        return 0;
    case WM_COMMAND:
        OnCommand(LOWORD(wparam));
        return 0;
    case WM_DRAWITEM:
        if (OnDrawItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lparam)))
        {
            return TRUE;
        }
        break;
    case WM_MEASUREITEM:
        if (OnMeasureItem(reinterpret_cast<MEASUREITEMSTRUCT*>(lparam)))
        {
            return TRUE;
        }
        break;
    case WM_PAINT:
        OnPaint();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
        OnLeftButtonDown(MakePoint(lparam), wparam);
        return 0;
    case WM_LBUTTONUP:
        OnLeftButtonUp(MakePoint(lparam));
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(MakePoint(lparam), wparam);
        return 0;
    case WM_MOUSELEAVE:
        trackingMouse_ = false;
        ClearHover();
        return 0;
    case WM_RBUTTONUP:
        OnRightButtonUp(MakePoint(lparam));
        return 0;
    case WM_LBUTTONDBLCLK:
        OnDoubleClick(MakePoint(lparam));
        return 0;
    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wparam), wparam, MakeScreenPoint(lparam));
        return 0;
    case WM_KEYDOWN:
        OnKeyDown(wparam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lparam) == HTCLIENT && ApplyHoverCursor())
        {
            return TRUE;
        }
        break;
    case WM_TIMER:
        if (wparam == SPLITVIEWER_TIMER_TIP)
        {
            OnTipTimer();
            return 0;
        }
        break;
    case SPLITVIEWER_WM_APP_EMBED_EXTERNAL_WINDOW:
        OnEmbedExternalWindow(reinterpret_cast<HWND>(wparam));
        return 0;
    case SPLITVIEWER_WM_APP_TOGGLE_FULLSCREEN:
        ToggleFullScreen();
        return 0;
    case WM_GETMINMAXINFO:
        OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lparam));
        return 0;
    case WM_DESTROY:
        SplitViewerDebugLog(L"WM_DESTROY.");
        UninstallMouseHook();
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd_, msg, wparam, lparam);
}

LRESULT CALLBACK SplitViewerWindow::LowLevelMouseProc(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HC_ACTION && hookOwner_)
    {
        if (wparam == WM_MOUSEMOVE &&
            !hookOwner_->embeddedDragWindow_ &&
            !hookOwner_->externalDragWindow_)
        {
            return CallNextHookEx(hookOwner_->mouseHook_, code, wparam, lparam);
        }

        const MSLLHOOKSTRUCT* info = reinterpret_cast<const MSLLHOOKSTRUCT*>(lparam);
        if (info)
        {
            hookOwner_->OnGlobalMouseEvent(wparam, info->pt);
        }
    }

    return CallNextHookEx(hookOwner_ ? hookOwner_->mouseHook_ : NULL, code, wparam, lparam);
}

void SplitViewerWindow::InstallMouseHook()
{
    if (mouseHook_)
    {
        return;
    }

    hookOwner_ = this;
    mouseHook_ = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hinst_, 0);
    if (!mouseHook_ && hookOwner_ == this)
    {
        SplitViewerDebugLogFormat(L"InstallMouseHook failed error=%u.", static_cast<unsigned int>(GetLastError()));
        hookOwner_ = NULL;
        return;
    }
    SplitViewerDebugLogFormat(L"InstallMouseHook success hook=0x%p.", mouseHook_);
}

void SplitViewerWindow::UninstallMouseHook()
{
    if (mouseHook_)
    {
        SplitViewerDebugLogFormat(L"UninstallMouseHook hook=0x%p.", mouseHook_);
        UnhookWindowsHookEx(mouseHook_);
        mouseHook_ = NULL;
    }
    if (hookOwner_ == this)
    {
        hookOwner_ = NULL;
    }
    ClearEmbeddedDragOutState();
    ClearExternalDragState(false);
    DestroyDragOutFrames();
}

void SplitViewerWindow::UpdateMouseHookState()
{
    if (NeedsMouseHook())
    {
        if (!mouseHook_)
        {
            SplitViewerDebugLog(L"UpdateMouseHookState needs hook.");
        }
        InstallMouseHook();
        return;
    }

    if (!embeddedDragWindow_ && !externalDragWindow_)
    {
        if (mouseHook_)
        {
            SplitViewerDebugLog(L"UpdateMouseHookState no hook needed.");
        }
        UninstallMouseHook();
    }
}

bool SplitViewerWindow::NeedsMouseHook() const
{
    if (NodeNeedsMouseHook(baseRoot_))
    {
        return true;
    }

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        if (NodeNeedsMouseHook(layers_[i]->root))
        {
            return true;
        }
    }

    return false;
}

bool SplitViewerWindow::NodeNeedsMouseHook(const SplitViewerNode* node) const
{
    if (!node)
    {
        return false;
    }
    if (node->IsLeaf())
    {
        return !node->view.HasContent() || node->view.HasEmbeddedWindow();
    }

    return NodeNeedsMouseHook(node->first) || NodeNeedsMouseHook(node->second);
}

HWND SplitViewerWindow::EnsureDragFrameWindow(HWND& frameWindow, COLORREF color)
{
    if (frameWindow && IsWindow(frameWindow))
    {
        SetWindowLongPtrW(frameWindow, GWLP_USERDATA, static_cast<LONG_PTR>(color));
        return frameWindow;
    }

    frameWindow = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kSplitViewerDragFrameClass,
        L"",
        WS_POPUP,
        0,
        0,
        1,
        1,
        NULL,
        NULL,
        hinst_,
        reinterpret_cast<LPVOID>(static_cast<LONG_PTR>(color)));
    if (frameWindow)
    {
        SetLayeredWindowAttributes(frameWindow, kSplitViewerFrameTransparentColor, 0, LWA_COLORKEY);
        SplitViewerDebugLogFormat(L"Drag frame window created hwnd=0x%p color=%u.", frameWindow, static_cast<unsigned int>(color));
    }
    else
    {
        SplitViewerDebugLogFormat(L"Drag frame window create failed color=%u error=%u.",
            static_cast<unsigned int>(color),
            static_cast<unsigned int>(GetLastError()));
    }
    return frameWindow;
}

void SplitViewerWindow::ShowDragFrameWindow(HWND& frameWindow, const RECT& screenRect, COLORREF color)
{
    if (!SplitViewerIsRectUsable(screenRect))
    {
        HideDragFrameWindow(frameWindow);
        return;
    }

    HWND frame = EnsureDragFrameWindow(frameWindow, color);
    if (!frame)
    {
        return;
    }

    SetWindowPos(frame,
        HWND_TOPMOST,
        screenRect.left,
        screenRect.top,
        std::max(1, SplitViewerRectWidth(screenRect)),
        std::max(1, SplitViewerRectHeight(screenRect)),
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(frame, NULL, TRUE);
}

void SplitViewerWindow::HideDragFrameWindow(HWND frameWindow)
{
    if (frameWindow && IsWindow(frameWindow))
    {
        ShowWindow(frameWindow, SW_HIDE);
    }
}

void SplitViewerWindow::HideDragOutFrames()
{
    HideDragFrameWindow(dragOutSourceFrame_);
    HideDragFrameWindow(dragOutPreviewFrame_);
}

void SplitViewerWindow::DestroyDragFrameWindow(HWND& frameWindow)
{
    if (frameWindow && IsWindow(frameWindow))
    {
        DestroyWindow(frameWindow);
    }
    frameWindow = NULL;
}

void SplitViewerWindow::DestroyDragOutFrames()
{
    DestroyDragFrameWindow(dragOutSourceFrame_);
    DestroyDragFrameWindow(dragOutPreviewFrame_);
}

void SplitViewerWindow::HideEmbeddedResizePreviewFrames()
{
    for (size_t i = 0; i < embeddedResizePreviewFrames_.size(); ++i)
    {
        HideDragFrameWindow(embeddedResizePreviewFrames_[i]);
    }
}

void SplitViewerWindow::DestroyEmbeddedResizePreviewFrames()
{
    for (size_t i = 0; i < embeddedResizePreviewFrames_.size(); ++i)
    {
        DestroyDragFrameWindow(embeddedResizePreviewFrames_[i]);
    }
    embeddedResizePreviewFrames_.clear();
}

void SplitViewerWindow::ShowEmbeddedResizePreviews(SplitViewerNode* root, const RECT& rootRect)
{
    if (!hwnd_ || !root || !SplitViewerIsRectUsable(rootRect))
    {
        HideEmbeddedResizePreviewFrames();
        return;
    }

    std::vector<RECT> clientRects;
    CollectEmbeddedResizePreviewRects(root, rootRect, clientRects);
    SplitViewerDebugLogFormat(L"ShowEmbeddedResizePreviews count=%u rootRect=%d,%d,%d,%d.",
        static_cast<unsigned int>(clientRects.size()),
        rootRect.left,
        rootRect.top,
        rootRect.right,
        rootRect.bottom);
    while (embeddedResizePreviewFrames_.size() < clientRects.size())
    {
        embeddedResizePreviewFrames_.push_back(NULL);
    }

    for (size_t i = 0; i < clientRects.size(); ++i)
    {
        POINT points[2] =
        {
            { clientRects[i].left, clientRects[i].top },
            { clientRects[i].right, clientRects[i].bottom }
        };
        MapWindowPoints(hwnd_, NULL, points, 2);

        RECT screenRect = SplitViewerMakeRect(points[0].x, points[0].y, points[1].x, points[1].y);
        ShowDragFrameWindow(embeddedResizePreviewFrames_[i], screenRect, kSplitViewerExternalDropFrameColor);
    }

    for (size_t i = clientRects.size(); i < embeddedResizePreviewFrames_.size(); ++i)
    {
        HideDragFrameWindow(embeddedResizePreviewFrames_[i]);
    }
}

void SplitViewerWindow::CollectEmbeddedResizePreviewRects(SplitViewerNode* node, const RECT& rc, std::vector<RECT>& rects) const
{
    if (!node || !SplitViewerIsRectUsable(rc))
    {
        return;
    }

    if (node->IsLeaf())
    {
        if (node->view.HasEmbeddedWindow())
        {
            rects.push_back(rc);
        }
        return;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);
    CollectEmbeddedResizePreviewRects(node->first, firstRect, rects);
    CollectEmbeddedResizePreviewRects(node->second, secondRect, rects);
}

void SplitViewerWindow::OnGlobalMouseEvent(WPARAM mouseMessage, POINT screenPt)
{
    if (!hwnd_ || !IsWindow(hwnd_))
    {
        return;
    }

    switch (mouseMessage)
    {
    case WM_LBUTTONDOWN:
        if (BeginEmbeddedChildInteraction(screenPt))
        {
            ClearExternalDragState(false);
            return;
        }
        BeginExternalDragProbe(screenPt);
        break;
    case WM_MOUSEMOVE:
        if (embeddedDragWindow_)
        {
            if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
            {
                ClearEmbeddedDragOutState();
            }
            else
            {
                UpdateEmbeddedDragOutPreview(screenPt);
            }
        }
        if (externalDragWindow_)
        {
            if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0)
            {
                ClearExternalDragState(true);
            }
            else
            {
                UpdateExternalDropTarget(screenPt);
            }
        }
        break;
    case WM_LBUTTONUP:
        if (embeddedDragWindow_)
        {
            CompleteEmbeddedDragOut(screenPt);
            return;
        }
        CompleteExternalDragProbe(screenPt);
        break;
    default:
        break;
    }
}

bool SplitViewerWindow::BeginEmbeddedChildInteraction(POINT screenPt)
{
    SplitViewerLeafHit hit;
    if (!FindEmbeddedLeafAtScreen(screenPt, hit))
    {
        return false;
    }

    embeddedDragWindow_ = hit.leaf->view.embeddedWindow;
    embeddedDragLeaf_ = hit.leaf;
    embeddedDragLayer_ = hit.layerIndex;
    embeddedDragStartPoint_ = screenPt;
    GetWindowRect(embeddedDragWindow_, &embeddedDragStartRect_);
    SplitViewerDebugLogFormat(L"Embedded child mouse down window=0x%p layer=%d start=%d,%d rect=%d,%d,%d,%d.",
        embeddedDragWindow_,
        embeddedDragLayer_,
        screenPt.x,
        screenPt.y,
        embeddedDragStartRect_.left,
        embeddedDragStartRect_.top,
        embeddedDragStartRect_.right,
        embeddedDragStartRect_.bottom);
    TrackEmbeddedDoubleClick(screenPt);
    return true;
}

void SplitViewerWindow::TrackEmbeddedDoubleClick(POINT screenPt)
{
    const DWORD now = GetTickCount();
    const int doubleClickWidth = std::max(1, GetSystemMetrics(SM_CXDOUBLECLK));
    const int doubleClickHeight = std::max(1, GetSystemMetrics(SM_CYDOUBLECLK));
    const bool isDoubleClick = embeddedClickTick_ != 0 &&
        now - embeddedClickTick_ <= GetDoubleClickTime() &&
        std::abs(screenPt.x - embeddedClickPoint_.x) <= doubleClickWidth &&
        std::abs(screenPt.y - embeddedClickPoint_.y) <= doubleClickHeight;

    if (isDoubleClick)
    {
        SplitViewerDebugLogFormat(L"Embedded child double click toggles fullscreen at=%d,%d.", screenPt.x, screenPt.y);
        embeddedClickTick_ = 0;
        ZeroMemory(&embeddedClickPoint_, sizeof(embeddedClickPoint_));
        PostMessageW(hwnd_, SPLITVIEWER_WM_APP_TOGGLE_FULLSCREEN, 0, 0);
    }
    else
    {
        embeddedClickTick_ = now;
        embeddedClickPoint_ = screenPt;
    }
}

void SplitViewerWindow::UpdateEmbeddedDragOutPreview(POINT screenPt)
{
    const bool showPreview = embeddedDragWindow_ &&
        IsWindow(embeddedDragWindow_) &&
        embeddedDragLeaf_ &&
        embeddedDragLeaf_->IsLeaf() &&
        embeddedDragLeaf_->view.embeddedWindow == embeddedDragWindow_ &&
        EmbeddedDragMovedEnough(screenPt) &&
        IsOutsideStageAtScreen(screenPt);

    if (!showPreview)
    {
        HideDragOutFrames();
        return;
    }

    RECT sourceRect = { 0 };
    if (GetLeafScreenRect(embeddedDragLeaf_, embeddedDragLayer_, sourceRect))
    {
        ShowDragFrameWindow(dragOutSourceFrame_, sourceRect, kSplitViewerExternalDragOutFrameColor);
    }
    else
    {
        HideDragFrameWindow(dragOutSourceFrame_);
    }

    RECT previewRect = MakeDetachedWindowRect(embeddedDragWindow_, screenPt);
    ShowDragFrameWindow(dragOutPreviewFrame_, previewRect, kSplitViewerExternalDragOutFrameColor);
}

void SplitViewerWindow::CompleteEmbeddedDragOut(POINT screenPt)
{
    HWND window = embeddedDragWindow_;
    SplitViewerNode* leaf = embeddedDragLeaf_;
    const int layerIndex = embeddedDragLayer_;
    const bool shouldDetach = window &&
        IsWindow(window) &&
        leaf &&
        leaf->IsLeaf() &&
        leaf->view.embeddedWindow == window &&
        EmbeddedDragMovedEnough(screenPt) &&
        IsOutsideStageAtScreen(screenPt);

    RECT detachedRect = MakeDetachedWindowRect(window, screenPt);
    ClearEmbeddedDragOutState();

    if (!shouldDetach)
    {
        SplitViewerDebugLogFormat(L"Embedded drag out cancelled window=0x%p layer=%d end=%d,%d.",
            window,
            layerIndex,
            screenPt.x,
            screenPt.y);
        return;
    }

    SplitViewerDebugLogFormat(L"Embedded drag out detach window=0x%p layer=%d rect=%d,%d,%d,%d.",
        window,
        layerIndex,
        detachedRect.left,
        detachedRect.top,
        detachedRect.right,
        detachedRect.bottom);
    leaf->view.DetachEmbeddedWindowToRect(detachedRect);
    selectedLayer_ = layerIndex;
    SyncEmbeddedWindows();
    UpdatePrimaryButtonText();
    UpdateMouseHookState();
    InvalidateStage();
}

void SplitViewerWindow::ClearEmbeddedDragOutState()
{
    HideDragOutFrames();
    embeddedDragWindow_ = NULL;
    embeddedDragLeaf_ = NULL;
    embeddedDragLayer_ = -1;
    ZeroMemory(&embeddedDragStartPoint_, sizeof(embeddedDragStartPoint_));
    ZeroMemory(&embeddedDragStartRect_, sizeof(embeddedDragStartRect_));
}

void SplitViewerWindow::BeginExternalDragProbe(POINT screenPt)
{
    ClearExternalDragState(false);

    HWND candidate = ExternalTopLevelWindowFromPoint(screenPt);
    if (!IsUsableExternalWindow(candidate))
    {
        return;
    }

    externalDragWindow_ = candidate;
    externalDragStartPoint_ = screenPt;
    GetWindowRect(candidate, &externalDragStartRect_);
    SplitViewerDebugLogFormat(L"External drag probe begin window=0x%p start=%d,%d rect=%d,%d,%d,%d.",
        externalDragWindow_,
        screenPt.x,
        screenPt.y,
        externalDragStartRect_.left,
        externalDragStartRect_.top,
        externalDragStartRect_.right,
        externalDragStartRect_.bottom);
}

void SplitViewerWindow::UpdateExternalDropTarget(POINT screenPt)
{
    SplitViewerNode* nextLeaf = NULL;
    SplitViewerLeafHit hit;
    if (externalDragWindow_ &&
        IsWindow(externalDragWindow_) &&
        ExternalDragMovedEnough(screenPt) &&
        FindExternalDropLeafAtScreen(screenPt, hit))
    {
        nextLeaf = hit.leaf;
    }

    if (nextLeaf != externalDropLeaf_)
    {
        externalDropLeaf_ = nextLeaf;
        SplitViewerDebugLogFormat(L"External drop target changed window=0x%p targetLeaf=0x%p.",
            externalDragWindow_,
            externalDropLeaf_);
        InvalidateStage();
    }
}

void SplitViewerWindow::CompleteExternalDragProbe(POINT screenPt)
{
    if (!externalDragWindow_)
    {
        return;
    }

    HWND droppedWindow = externalDragWindow_;
    SplitViewerLeafHit hit;
    const bool shouldEmbed = IsWindow(droppedWindow) &&
        ExternalDragMovedEnough(screenPt) &&
        IsUsableExternalWindow(droppedWindow) &&
        FindExternalDropLeafAtScreen(screenPt, hit);

    ClearExternalDragState(true);

    if (shouldEmbed)
    {
        SplitViewerDebugLogFormat(L"External drop accepted window=0x%p layer=%d leaf=0x%p end=%d,%d.",
            droppedWindow,
            hit.layerIndex,
            hit.leaf,
            screenPt.x,
            screenPt.y);
        pendingEmbedWindow_ = droppedWindow;
        pendingEmbedLeaf_ = hit.leaf;
        pendingEmbedLayer_ = hit.layerIndex;
        PostMessageW(hwnd_, SPLITVIEWER_WM_APP_EMBED_EXTERNAL_WINDOW, reinterpret_cast<WPARAM>(droppedWindow), 0);
    }
    else
    {
        SplitViewerDebugLogFormat(L"External drop ignored window=0x%p end=%d,%d.",
            droppedWindow,
            screenPt.x,
            screenPt.y);
    }
}

void SplitViewerWindow::ClearExternalDragState(bool invalidate)
{
    externalDragWindow_ = NULL;
    ZeroMemory(&externalDragStartPoint_, sizeof(externalDragStartPoint_));
    ZeroMemory(&externalDragStartRect_, sizeof(externalDragStartRect_));
    if (externalDropLeaf_)
    {
        externalDropLeaf_ = NULL;
        if (invalidate)
        {
            InvalidateStage();
        }
    }
}

void SplitViewerWindow::OnEmbedExternalWindow(HWND window)
{
    SplitViewerNode* targetLeaf = pendingEmbedLeaf_;
    const int targetLayer = pendingEmbedLayer_;
    pendingEmbedWindow_ = NULL;
    pendingEmbedLeaf_ = NULL;
    pendingEmbedLayer_ = -1;

    if (!window || !targetLeaf || !targetLeaf->IsLeaf() || targetLeaf->view.HasContent())
    {
        SplitViewerDebugLogFormat(L"OnEmbedExternalWindow rejected window=0x%p layer=%d leaf=0x%p.",
            window,
            targetLayer,
            targetLeaf);
        return;
    }
    if (!IsUsableExternalWindow(window))
    {
        SplitViewerDebugLogFormat(L"OnEmbedExternalWindow unusable window=0x%p.", window);
        return;
    }

    if (!targetLeaf->view.AttachEmbeddedWindow(window, hwnd_))
    {
        SplitViewerDebugLogFormat(L"OnEmbedExternalWindow attach failed window=0x%p layer=%d.", window, targetLayer);
        MessageBoxW(hwnd_, L"\u5D4C\u5165\u7A97\u53E3\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return;
    }

    selectedLayer_ = targetLayer;
    SyncEmbeddedWindows();
    UpdatePrimaryButtonText();
    UpdateMouseHookState();
    InvalidateRect(hwnd_, NULL, FALSE);
    SplitViewerDebugLogFormat(L"OnEmbedExternalWindow success window=0x%p layer=%d.", window, targetLayer);
}

void SplitViewerWindow::CreateInterfaceFont()
{
    if (mainFont_)
    {
        return;
    }

    HDC screenDc = GetDC(NULL);
    int fontHeight = -16;
    if (screenDc)
    {
        fontHeight = -MulDiv(10, GetDeviceCaps(screenDc, LOGPIXELSY), 72);
        ReleaseDC(NULL, screenDc);
    }

    mainFont_ = CreateFontW(fontHeight,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        GB2312_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        kSplitViewerFontName);
    if (!mainFont_)
    {
        SplitViewerDebugLogFormat(L"CreateInterfaceFont failed font=%s error=%u.",
            kSplitViewerFontName,
            static_cast<unsigned int>(GetLastError()));
    }
    else
    {
        SplitViewerDebugLogFormat(L"CreateInterfaceFont success font=%s height=%d.",
            kSplitViewerFontName,
            fontHeight);
    }
}

void SplitViewerWindow::DestroyInterfaceFont()
{
    if (mainFont_)
    {
        DeleteObject(mainFont_);
        mainFont_ = NULL;
    }
}

void SplitViewerWindow::ApplyFont(HWND window)
{
    if (!window || !mainFont_)
    {
        return;
    }

    SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(mainFont_), TRUE);
}

void SplitViewerWindow::CreateToolbar()
{
    saveConfigButton_ = CreateButton(SPLITVIEWER_ID_SAVE_CONFIG);
    saveButton_ = CreateButton(SPLITVIEWER_ID_SAVE_IMAGE);
    newLayerButton_ = CreateButton(SPLITVIEWER_ID_NEW_LAYER);
    fullScreenButton_ = CreateButton(SPLITVIEWER_ID_FULLSCREEN);
    aboutButton_ = CreateButton(SPLITVIEWER_ID_ABOUT);
    CreateTooltipWindow();
    UpdateButtonTooltips();
    LayoutToolbar();
    SplitViewerDebugLogFormat(L"CreateToolbar buttons saveConfig=0x%p saveImage=0x%p newLayer=0x%p fullscreen=0x%p about=0x%p tooltip=0x%p.",
        saveConfigButton_,
        saveButton_,
        newLayerButton_,
        fullScreenButton_,
        aboutButton_,
        tooltipWindow_);
}

HWND SplitViewerWindow::CreateButton(int id)
{
    HWND button = CreateWindowExW(0,
        L"BUTTON",
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        0,
        0,
        kSplitViewerButtonWidth,
        kSplitViewerButtonHeight,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hinst_,
        NULL);
    if (!button)
    {
        SplitViewerDebugLogFormat(L"CreateButton failed id=%d error=%u.",
            id,
            static_cast<unsigned int>(GetLastError()));
        return NULL;
    }

    ApplyFont(button);
    return button;
}

void SplitViewerWindow::LayoutToolbar()
{
    if (!hwnd_)
    {
        return;
    }

    const BOOL visible = isFullScreen_ ? FALSE : TRUE;
    UpdatePrimaryButtonText();

    HWND buttons[] = { saveConfigButton_, saveButton_, newLayerButton_, fullScreenButton_ };
    int x = kSplitViewerMargin;
    const int y = (kSplitViewerToolbarHeight - kSplitViewerButtonHeight) / 2;
    for (int i = 0; i < ARRAYSIZE(buttons); ++i)
    {
        if (!buttons[i])
        {
            continue;
        }

        ShowWindow(buttons[i], visible ? SW_SHOW : SW_HIDE);
        SetWindowPos(buttons[i],
            NULL,
            x,
            y,
            kSplitViewerButtonWidth,
            kSplitViewerButtonHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
        x += kSplitViewerButtonWidth + kSplitViewerButtonGap;
    }

    if (aboutButton_)
    {
        RECT client = { 0 };
        GetClientRect(hwnd_, &client);
        const int aboutX = std::max(kSplitViewerMargin,
            static_cast<int>(client.right) - kSplitViewerMargin - kSplitViewerButtonWidth);
        ShowWindow(aboutButton_, visible ? SW_SHOW : SW_HIDE);
        SetWindowPos(aboutButton_,
            NULL,
            aboutX,
            y,
            kSplitViewerButtonWidth,
            kSplitViewerButtonHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void SplitViewerWindow::CreateTooltipWindow()
{
    if (tooltipWindow_)
    {
        return;
    }

    tooltipWindow_ = CreateWindowExW(WS_EX_TOPMOST,
        TOOLTIPS_CLASSW,
        NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwnd_,
        NULL,
        hinst_,
        NULL);
    if (!tooltipWindow_)
    {
        SplitViewerDebugLogFormat(L"CreateTooltipWindow failed error=%u.", static_cast<unsigned int>(GetLastError()));
        return;
    }

    ApplyFont(tooltipWindow_);
    SendMessageW(tooltipWindow_, TTM_SETMAXTIPWIDTH, 0, 360);
    SetWindowPos(tooltipWindow_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SplitViewerWindow::AddTooltip(HWND control, const wchar_t* text)
{
    if (!tooltipWindow_ || !control || !text)
    {
        return;
    }

    TOOLINFOW toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.hwnd = hwnd_;
    toolInfo.uId = reinterpret_cast<UINT_PTR>(control);
    toolInfo.lpszText = const_cast<LPWSTR>(text);
    SendMessageW(tooltipWindow_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&toolInfo));
}

void SplitViewerWindow::UpdateButtonTooltips()
{
    primaryConfigTipText_ = HasAnyContent() ? L"\u4FDD\u5B58" : L"\u52A0\u8F7D\u914D\u7F6E";
    AddTooltip(saveConfigButton_, primaryConfigTipText_.c_str());
    AddTooltip(saveButton_, L"\u53E6\u5B58\u56FE\u7247");
    AddTooltip(newLayerButton_, L"\u65B0\u5EFA\u56FE\u5C42");
    AddTooltip(fullScreenButton_, L"\u5168\u5C4F");
    AddTooltip(aboutButton_, L"\u5173\u4E8E");
    UpdatePrimaryButtonText();
}

void SplitViewerWindow::OnCommand(WORD id)
{
    SplitViewerDebugLogFormat(L"OnCommand id=%u.", static_cast<unsigned int>(id));
    switch (id)
    {
    case SPLITVIEWER_ID_SAVE_IMAGE:
        {
            SaveStageImage();
            break;
        }
    case SPLITVIEWER_ID_SAVE_CONFIG:
        {
            PrimaryConfigAction();
            break;
        }
    case SPLITVIEWER_ID_NEW_LAYER:
        {
            AddLayer();
            break;
        }
    case SPLITVIEWER_ID_FULLSCREEN:
        {
            ToggleFullScreen();
            break;
        }
    case SPLITVIEWER_ID_ABOUT:
        {
            ShowAboutDialog();
            break;
        }
    default:
        {
            break;
        }
    }
}

void SplitViewerWindow::PrimaryConfigAction()
{
    if (HasAnyContent())
    {
        SplitViewerDebugLog(L"PrimaryConfigAction executes save config.");
        SaveConfigDialog();
    }
    else
    {
        SplitViewerDebugLog(L"PrimaryConfigAction executes load config.");
        LoadConfigDialog();
    }
}

void SplitViewerWindow::UpdatePrimaryButtonText()
{
    if (!saveConfigButton_)
    {
        return;
    }

    const std::wstring newText = HasAnyContent() ? L"\u4FDD\u5B58" : L"\u52A0\u8F7D\u914D\u7F6E";
    if (primaryConfigTipText_ != newText)
    {
        SplitViewerDebugLogFormat(L"Primary config button tip changed old=%s new=%s.",
            primaryConfigTipText_.c_str(),
            newText.c_str());
        primaryConfigTipText_ = newText;
    }

    if (tooltipWindow_)
    {
        TOOLINFOW toolInfo = { 0 };
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.hwnd = hwnd_;
        toolInfo.uId = reinterpret_cast<UINT_PTR>(saveConfigButton_);
        toolInfo.lpszText = const_cast<LPWSTR>(primaryConfigTipText_.c_str());
        SendMessageW(tooltipWindow_, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&toolInfo));
    }

    InvalidateRect(saveConfigButton_, NULL, TRUE);
}

bool SplitViewerWindow::OnDrawItem(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
    {
        return false;
    }

    if (drawItem->CtlType == ODT_BUTTON)
    {
        DrawToolbarButton(drawItem);
        return true;
    }
    if (drawItem->CtlType == ODT_MENU)
    {
        DrawOwnerMenuItem(drawItem);
        return true;
    }

    return false;
}

bool SplitViewerWindow::OnMeasureItem(MEASUREITEMSTRUCT* measureItem)
{
    if (!measureItem || measureItem->CtlType != ODT_MENU)
    {
        return false;
    }

    const wchar_t* text = reinterpret_cast<const wchar_t*>(measureItem->itemData);
    if (!text)
    {
        text = L"";
    }

    SIZE textSize = { 72, 18 };
    HDC hdc = hwnd_ ? GetDC(hwnd_) : NULL;
    if (hdc)
    {
        HFONT font = mainFont_ ? mainFont_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ oldFont = SelectObject(hdc, font);
        GetTextExtentPoint32W(hdc, text, static_cast<int>(wcslen(text)), &textSize);
        SelectObject(hdc, oldFont);
        ReleaseDC(hwnd_, hdc);
    }

    measureItem->itemWidth = static_cast<UINT>(std::max(96, static_cast<int>(textSize.cx) + 42));
    measureItem->itemHeight = static_cast<UINT>(std::max(26, static_cast<int>(textSize.cy) + 10));
    return true;
}

void SplitViewerWindow::DrawToolbarButton(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
    {
        return;
    }

    RECT rc = drawItem->rcItem;
    const bool enabled = (drawItem->itemState & ODS_DISABLED) == 0;
    const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
    const bool hot = (drawItem->itemState & ODS_HOTLIGHT) != 0;
    COLORREF backColor = kSplitViewerToolbarColor;
    if (pressed)
    {
        backColor = RGB(210, 226, 246);
    }
    else if (hot)
    {
        backColor = RGB(230, 240, 252);
    }

    FillSolidRect(drawItem->hDC, rc, backColor);

    HPEN borderPen = CreatePen(PS_SOLID, 1, hot || pressed ? RGB(78, 132, 190) : RGB(190, 190, 190));
    HGDIOBJ oldPen = SelectObject(drawItem->hDC, borderPen);
    HGDIOBJ oldBrush = SelectObject(drawItem->hDC, GetStockObject(NULL_BRUSH));
    Rectangle(drawItem->hDC, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(drawItem->hDC, oldBrush);
    SelectObject(drawItem->hDC, oldPen);
    DeleteObject(borderPen);

    RECT iconRect = rc;
    InflateRect(&iconRect, -7, -5);
    if (pressed)
    {
        OffsetRect(&iconRect, 1, 1);
    }
    DrawToolbarIcon(drawItem->hDC, iconRect, drawItem->CtlID, enabled, hot, pressed);

    if ((drawItem->itemState & ODS_FOCUS) != 0)
    {
        RECT focusRect = rc;
        InflateRect(&focusRect, -3, -3);
        DrawFocusRect(drawItem->hDC, &focusRect);
    }
}

void SplitViewerWindow::DrawToolbarIcon(HDC hdc, const RECT& rc, UINT controlId, bool enabled, bool hot, bool pressed)
{
    UNREFERENCED_PARAMETER(hot);
    UNREFERENCED_PARAMETER(pressed);

    const COLORREF lineColor = enabled ? RGB(42, 78, 116) : RGB(150, 150, 150);
    const COLORREF blueColor = enabled ? RGB(34, 126, 220) : RGB(170, 170, 170);
    const COLORREF greenColor = enabled ? RGB(70, 155, 92) : RGB(170, 170, 170);
    const COLORREF yellowColor = enabled ? RGB(252, 196, 72) : RGB(190, 190, 190);

    HPEN linePen = CreatePen(PS_SOLID, 2, lineColor);
    HGDIOBJ oldPen = SelectObject(hdc, linePen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

    switch (controlId)
    {
    case SPLITVIEWER_ID_SAVE_CONFIG:
        {
            if (HasAnyContent())
            {
                HBRUSH bodyBrush = CreateSolidBrush(blueColor);
                HGDIOBJ oldCaseBrush = SelectObject(hdc, bodyBrush);
                Rectangle(hdc, rc.left + 2, rc.top + 1, rc.right - 2, rc.bottom - 1);
                SelectObject(hdc, oldCaseBrush);
                DeleteObject(bodyBrush);

                RECT labelRect = SplitViewerMakeRect(rc.left + 5, rc.top + 4, rc.right - 5, rc.top + 9);
                FillSolidRect(hdc, labelRect, RGB(245, 248, 252));
                RECT slotRect = SplitViewerMakeRect(rc.left + 7, rc.bottom - 9, rc.right - 7, rc.bottom - 3);
                FillSolidRect(hdc, slotRect, RGB(232, 238, 246));
            }
            else
            {
                HBRUSH folderBrush = CreateSolidBrush(yellowColor);
                HGDIOBJ oldCaseBrush = SelectObject(hdc, folderBrush);
                Rectangle(hdc, rc.left + 2, rc.top + 6, rc.right - 2, rc.bottom - 2);
                Rectangle(hdc, rc.left + 4, rc.top + 3, rc.left + 13, rc.top + 8);
                SelectObject(hdc, oldCaseBrush);
                DeleteObject(folderBrush);

                MoveToEx(hdc, rc.left + 9, rc.top + 13, NULL);
                LineTo(hdc, rc.left + 15, rc.bottom - 5);
                LineTo(hdc, rc.right - 7, rc.top + 13);
            }
            break;
        }
    case SPLITVIEWER_ID_SAVE_IMAGE:
        {
            Rectangle(hdc, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 4);
            HBRUSH sunBrush = CreateSolidBrush(yellowColor);
            HGDIOBJ oldCaseBrush = SelectObject(hdc, sunBrush);
            Ellipse(hdc, rc.right - 10, rc.top + 5, rc.right - 5, rc.top + 10);
            SelectObject(hdc, oldCaseBrush);
            DeleteObject(sunBrush);

            MoveToEx(hdc, rc.left + 5, rc.bottom - 7, NULL);
            LineTo(hdc, rc.left + 10, rc.top + 11);
            LineTo(hdc, rc.left + 15, rc.bottom - 7);
            LineTo(hdc, rc.left + 19, rc.top + 13);
            LineTo(hdc, rc.right - 5, rc.bottom - 7);

            HPEN arrowPen = CreatePen(PS_SOLID, 2, blueColor);
            SelectObject(hdc, arrowPen);
            const int centerX = (rc.left + rc.right) / 2;
            MoveToEx(hdc, centerX, rc.top + 4, NULL);
            LineTo(hdc, centerX, rc.top + 13);
            LineTo(hdc, centerX - 4, rc.top + 9);
            MoveToEx(hdc, centerX, rc.top + 13, NULL);
            LineTo(hdc, centerX + 4, rc.top + 9);
            SelectObject(hdc, linePen);
            DeleteObject(arrowPen);
            break;
        }
    case SPLITVIEWER_ID_NEW_LAYER:
        {
            HPEN backPen = CreatePen(PS_SOLID, 2, RGB(120, 120, 120));
            HGDIOBJ oldBackPen = SelectObject(hdc, backPen);
            Rectangle(hdc, rc.left + 6, rc.top + 2, rc.right - 3, rc.bottom - 7);
            SelectObject(hdc, oldBackPen);
            DeleteObject(backPen);
            SelectObject(hdc, linePen);
            Rectangle(hdc, rc.left + 2, rc.top + 6, rc.right - 7, rc.bottom - 3);

            HPEN plusPen = CreatePen(PS_SOLID, 2, greenColor);
            SelectObject(hdc, plusPen);
            const int centerX = (rc.left + rc.right) / 2 - 1;
            const int centerY = (rc.top + rc.bottom) / 2 + 2;
            MoveToEx(hdc, centerX - 5, centerY, NULL);
            LineTo(hdc, centerX + 6, centerY);
            MoveToEx(hdc, centerX, centerY - 5, NULL);
            LineTo(hdc, centerX, centerY + 6);
            SelectObject(hdc, linePen);
            DeleteObject(plusPen);
            break;
        }
    case SPLITVIEWER_ID_FULLSCREEN:
        {
            const int left = rc.left + 2;
            const int top = rc.top + 2;
            const int right = rc.right - 2;
            const int bottom = rc.bottom - 2;
            MoveToEx(hdc, left, top + 6, NULL);
            LineTo(hdc, left, top);
            LineTo(hdc, left + 6, top);
            MoveToEx(hdc, right - 6, top, NULL);
            LineTo(hdc, right, top);
            LineTo(hdc, right, top + 6);
            MoveToEx(hdc, left, bottom - 6, NULL);
            LineTo(hdc, left, bottom);
            LineTo(hdc, left + 6, bottom);
            MoveToEx(hdc, right - 6, bottom, NULL);
            LineTo(hdc, right, bottom);
            LineTo(hdc, right, bottom - 6);
            break;
        }
    case SPLITVIEWER_ID_ABOUT:
        {
            HBRUSH questionBrush = CreateSolidBrush(blueColor);
            HGDIOBJ oldCaseBrush = SelectObject(hdc, questionBrush);
            Ellipse(hdc, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
            SelectObject(hdc, oldCaseBrush);
            DeleteObject(questionBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            HFONT font = mainFont_ ? mainFont_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HGDIOBJ oldTextFont = SelectObject(hdc, font);
            RECT textRect = rc;
            DrawTextW(hdc, L"?", -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            SelectObject(hdc, oldTextFont);
            break;
        }
    default:
        {
            break;
        }
    }

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

void SplitViewerWindow::DrawOwnerMenuItem(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
    {
        return;
    }

    const wchar_t* text = reinterpret_cast<const wchar_t*>(drawItem->itemData);
    if (!text)
    {
        text = L"";
    }

    RECT rc = drawItem->rcItem;
    const bool selected = (drawItem->itemState & ODS_SELECTED) != 0;
    const bool disabled = (drawItem->itemState & ODS_GRAYED) != 0;
    FillSolidRect(drawItem->hDC, rc, selected ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_MENU));

    HFONT font = mainFont_ ? mainFont_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(drawItem->hDC, font);
    SetBkMode(drawItem->hDC, TRANSPARENT);
    if (disabled)
    {
        SetTextColor(drawItem->hDC, GetSysColor(COLOR_GRAYTEXT));
    }
    else
    {
        SetTextColor(drawItem->hDC, selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_MENUTEXT));
    }

    RECT textRect = rc;
    textRect.left += 18;
    textRect.right -= 8;
    DrawTextW(drawItem->hDC, text, -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
    SelectObject(drawItem->hDC, oldFont);
}

void SplitViewerWindow::AppendOwnerDrawMenuItem(HMENU menu, UINT command, const wchar_t* text, bool enabled)
{
    if (!menu || !text)
    {
        return;
    }

    UINT flags = MF_OWNERDRAW;
    if (!enabled)
    {
        flags |= MF_GRAYED;
    }

    AppendMenuW(menu, flags, command, reinterpret_cast<LPCWSTR>(text));
}

void SplitViewerWindow::ShowAboutDialog()
{
    RegisterAboutWindowClass();

    const int dialogWidth = 430;
    const int dialogHeight = 270;
    RECT ownerRect = { 0 };
    if (!GetWindowRect(hwnd_, &ownerRect))
    {
        ownerRect = SplitViewerMakeRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }

    const int x = ownerRect.left + (SplitViewerRectWidth(ownerRect) - dialogWidth) / 2;
    const int y = ownerRect.top + (SplitViewerRectHeight(ownerRect) - dialogHeight) / 2;
    SplitViewerDebugLogFormat(L"ShowAboutDialog create position=%d,%d size=%dx%d.", x, y, dialogWidth, dialogHeight);
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
        kSplitViewerAboutWindowClass,
        L"\u5173\u4E8E\u5206\u5C4F\u770B\u56FE",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x,
        y,
        dialogWidth,
        dialogHeight,
        hwnd_,
        NULL,
        hinst_,
        this);
    if (!dialog)
    {
        SplitViewerDebugLogFormat(L"ShowAboutDialog CreateWindowEx failed error=%u.", static_cast<unsigned int>(GetLastError()));
        return;
    }

    // 用本地消息循环实现轻量模态窗口，确保父窗口禁用期间仍能处理绘制和关闭消息。
    EnableWindow(hwnd_, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    MSG msg = { 0 };
    while (IsWindow(dialog))
    {
        BOOL result = GetMessageW(&msg, NULL, 0, 0);
        if (result == -1)
        {
            SplitViewerDebugLogFormat(L"ShowAboutDialog GetMessage failed error=%u.", static_cast<unsigned int>(GetLastError()));
            break;
        }
        if (result == 0)
        {
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }

        if (!IsDialogMessageW(dialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (IsWindow(hwnd_))
    {
        EnableWindow(hwnd_, TRUE);
        SetForegroundWindow(hwnd_);
        SetActiveWindow(hwnd_);
    }
    SplitViewerDebugLog(L"ShowAboutDialog finished.");
}

void SplitViewerWindow::OnPaint()
{
    PAINTSTRUCT ps = { 0 };
    const DWORD paintStartTick = GetTickCount();
    HDC hdc = BeginPaint(hwnd_, &ps);
    RECT client = { 0 };
    GetClientRect(hwnd_, &client);

    const int clientWidth = SplitViewerRectWidth(client);
    const int clientHeight = SplitViewerRectHeight(client);
    if (clientWidth <= 0 || clientHeight <= 0)
    {
        EndPaint(hwnd_, &ps);
        return;
    }

    if (!EnsureBackBuffer(hdc, clientWidth, clientHeight))
    {
        DrawWindow(hdc, client, true);
        const DWORD elapsed = GetTickCount() - paintStartTick;
        if (elapsed > 30)
        {
            SplitViewerDebugLogFormat(L"Paint slow direct elapsedMs=%u client=%d,%d,%d,%d paint=%d,%d,%d,%d.",
                static_cast<unsigned int>(elapsed),
                client.left,
                client.top,
                client.right,
                client.bottom,
                ps.rcPaint.left,
                ps.rcPaint.top,
                ps.rcPaint.right,
                ps.rcPaint.bottom);
        }
        EndPaint(hwnd_, &ps);
        return;
    }

    RECT paintRect = ps.rcPaint;
    if (!backBufferValid_ || !SplitViewerIsRectUsable(paintRect))
    {
        paintRect = client;
    }

    hasPaintClip_ = true;
    paintClipRect_ = paintRect;

    HRGN clipRegion = CreateRectRgn(paintRect.left, paintRect.top, paintRect.right, paintRect.bottom);
    if (clipRegion)
    {
        SelectClipRgn(backBufferDc_, clipRegion);
    }

    DrawWindow(backBufferDc_, client, true);

    if (clipRegion)
    {
        SelectClipRgn(backBufferDc_, NULL);
        DeleteObject(clipRegion);
    }
    hasPaintClip_ = false;
    ZeroMemory(&paintClipRect_, sizeof(paintClipRect_));

    BitBlt(hdc,
        paintRect.left,
        paintRect.top,
        SplitViewerRectWidth(paintRect),
        SplitViewerRectHeight(paintRect),
        backBufferDc_,
        paintRect.left,
        paintRect.top,
        SRCCOPY);
    backBufferValid_ = true;
    const DWORD elapsed = GetTickCount() - paintStartTick;
    if (elapsed > 30)
    {
        SplitViewerDebugLogFormat(L"Paint slow buffered elapsedMs=%u client=%d,%d,%d,%d paint=%d,%d,%d,%d fast=%d drag=%s layers=%d.",
            static_cast<unsigned int>(elapsed),
            client.left,
            client.top,
            client.right,
            client.bottom,
            paintRect.left,
            paintRect.top,
            paintRect.right,
            paintRect.bottom,
            ShouldUseFastImageRendering() ? 1 : 0,
            DragModeText(dragMode_),
            static_cast<int>(layers_.size()));
    }
    EndPaint(hwnd_, &ps);
}

bool SplitViewerWindow::EnsureBackBuffer(HDC referenceDc, int width, int height)
{
    width = std::max(1, width);
    height = std::max(1, height);
    if (backBufferDc_ &&
        backBufferBitmap_ &&
        backBufferWidth_ == width &&
        backBufferHeight_ == height)
    {
        return true;
    }

    ReleaseBackBuffer();
    SplitViewerDebugLogFormat(L"Recreate back buffer size=%dx%d.", width, height);

    backBufferDc_ = CreateCompatibleDC(referenceDc);
    if (!backBufferDc_)
    {
        SplitViewerDebugLogFormat(L"CreateCompatibleDC for back buffer failed error=%u.", static_cast<unsigned int>(GetLastError()));
        return false;
    }

    backBufferBitmap_ = CreateCompatibleBitmap(referenceDc, width, height);
    if (!backBufferBitmap_)
    {
        SplitViewerDebugLogFormat(L"CreateCompatibleBitmap for back buffer failed size=%dx%d error=%u.",
            width,
            height,
            static_cast<unsigned int>(GetLastError()));
        ReleaseBackBuffer();
        return false;
    }

    backBufferOldBitmap_ = SelectObject(backBufferDc_, backBufferBitmap_);
    if (!backBufferOldBitmap_ || backBufferOldBitmap_ == HGDI_ERROR)
    {
        backBufferOldBitmap_ = NULL;
        ReleaseBackBuffer();
        return false;
    }

    backBufferWidth_ = width;
    backBufferHeight_ = height;
    backBufferValid_ = false;
    SplitViewerDebugLogFormat(L"Back buffer ready size=%dx%d.", backBufferWidth_, backBufferHeight_);
    return true;
}

void SplitViewerWindow::ReleaseBackBuffer()
{
    if (backBufferDc_ && backBufferOldBitmap_)
    {
        SelectObject(backBufferDc_, backBufferOldBitmap_);
    }
    if (backBufferBitmap_)
    {
        DeleteObject(backBufferBitmap_);
    }
    if (backBufferDc_)
    {
        DeleteDC(backBufferDc_);
    }

    backBufferDc_ = NULL;
    backBufferBitmap_ = NULL;
    backBufferOldBitmap_ = NULL;
    backBufferWidth_ = 0;
    backBufferHeight_ = 0;
    backBufferValid_ = false;
}

bool SplitViewerWindow::PaintIntersects(const RECT& rc) const
{
    if (!SplitViewerIsRectUsable(rc))
    {
        return false;
    }
    if (!hasPaintClip_)
    {
        return true;
    }

    RECT intersectRect = { 0 };
    return IntersectRect(&intersectRect, &paintClipRect_, &rc) && SplitViewerIsRectUsable(intersectRect);
}

bool SplitViewerWindow::ShouldUseFastImageRendering() const
{
    if (dragMode_ != SPLITVIEWER_DRAG_NONE)
    {
        return true;
    }
    if (!tipText_.empty() && GetTickCount() - tipTick_ < 1000)
    {
        return true;
    }

    return false;
}

void SplitViewerWindow::DrawWindow(HDC hdc, const RECT& client, bool includeTip)
{
    FillSolidRect(hdc, client, isFullScreen_ ? RGB(18, 18, 18) : kSplitViewerWindowBackColor);

    if (!isFullScreen_)
    {
        RECT toolbar = SplitViewerMakeRect(0, 0, client.right, kSplitViewerToolbarHeight);
        FillSolidRect(hdc, toolbar, kSplitViewerToolbarColor);
    }

    RECT stage = GetStageRect();
    DrawStage(hdc, stage);

    if (includeTip)
    {
        DrawTip(hdc, client);
    }
}

void SplitViewerWindow::DrawStage(HDC hdc, const RECT& stage)
{
    if (!SplitViewerIsRectUsable(stage) || !PaintIntersects(stage))
    {
        return;
    }

    FillSolidRect(hdc, stage, kSplitViewerGrayColor);
    RECT baseContent = ContentRect(stage);
    DrawNode(hdc, baseRoot_, baseContent, -1);
    DrawFrame(hdc, stage, kSplitViewerFrameColor);

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        RECT layerRect = GetLayerRect(static_cast<int>(i), stage);
        if (!SplitViewerIsRectUsable(layerRect) || !PaintIntersects(layerRect))
        {
            continue;
        }

        FillSolidRect(hdc, layerRect, kSplitViewerGrayColor);
        RECT layerContent = ContentRect(layerRect);
        DrawNode(hdc, layers_[i]->root, layerContent, static_cast<int>(i));
        DrawFrame(hdc, layerRect, selectedLayer_ == static_cast<int>(i) ? kSplitViewerSelectedFrameColor : kSplitViewerFrameColor);
    }
}

void SplitViewerWindow::DrawProjectedStage(HDC hdc, const RECT& sourceStage, const RECT& targetStage, bool includeEmbeddedWindows)
{
    if (!SplitViewerIsRectUsable(sourceStage) || !SplitViewerIsRectUsable(targetStage))
    {
        return;
    }

    FillSolidRect(hdc, targetStage, kSplitViewerGrayColor);

    RECT sourceBase = ContentRect(sourceStage);
    RECT targetBase = ContentRect(targetStage);
    DrawProjectedNode(hdc, baseRoot_, sourceBase, targetBase, -1, includeEmbeddedWindows);
    DrawFrame(hdc, targetStage, kSplitViewerFrameColor);

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        RECT sourceLayer = GetLayerRect(static_cast<int>(i), sourceStage);
        RECT targetLayer = GetLayerRect(static_cast<int>(i), targetStage);
        if (!SplitViewerIsRectUsable(sourceLayer) || !SplitViewerIsRectUsable(targetLayer))
        {
            continue;
        }

        FillSolidRect(hdc, targetLayer, kSplitViewerGrayColor);
        DrawProjectedNode(hdc,
            layers_[i]->root,
            ContentRect(sourceLayer),
            ContentRect(targetLayer),
            static_cast<int>(i),
            includeEmbeddedWindows);
        DrawFrame(hdc, targetLayer, selectedLayer_ == static_cast<int>(i) ? kSplitViewerSelectedFrameColor : kSplitViewerFrameColor);
    }
}

void SplitViewerWindow::DrawNode(HDC hdc, SplitViewerNode* node, const RECT& rc, int layerIndex)
{
    if (!node || !SplitViewerIsRectUsable(rc) || !PaintIntersects(rc))
    {
        return;
    }

    if (node->IsLeaf())
    {
        DrawLeaf(hdc, node, rc);
        if (node == externalDropLeaf_)
        {
            DrawDropFrame(hdc, rc);
        }
        return;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);

    DrawNode(hdc, node->first, firstRect, layerIndex);
    DrawNode(hdc, node->second, secondRect, layerIndex);

    if (borderVisible_)
    {
        const bool hover = hoveredSplitter_ == node && hoveredSplitterLayer_ == layerIndex;
        RECT drawRect = SplitterDrawRect(splitterRect, node->direction, hover ? kSplitViewerSplitterHoverSize : SplitterSize());
        if (PaintIntersects(drawRect))
        {
            FillSolidRect(hdc, drawRect, hover ? kSplitViewerSplitterHoverColor : kSplitViewerSplitterColor);
        }
    }
}

void SplitViewerWindow::DrawProjectedNode(HDC hdc, SplitViewerNode* node, const RECT& sourceRect, const RECT& targetRect, int layerIndex, bool includeEmbeddedWindows)
{
    if (!node || !SplitViewerIsRectUsable(sourceRect) || !SplitViewerIsRectUsable(targetRect))
    {
        return;
    }

    if (node->IsLeaf())
    {
        DrawProjectedLeaf(hdc, node, sourceRect, targetRect, includeEmbeddedWindows);
        return;
    }

    RECT sourceFirst = { 0 };
    RECT sourceSplitter = { 0 };
    RECT sourceSecond = { 0 };
    RECT targetFirst = { 0 };
    RECT targetSplitter = { 0 };
    RECT targetSecond = { 0 };
    SplitNodeRects(sourceRect, node, sourceFirst, sourceSplitter, sourceSecond);
    SplitNodeRects(targetRect, node, targetFirst, targetSplitter, targetSecond);

    DrawProjectedNode(hdc, node->first, sourceFirst, targetFirst, layerIndex, includeEmbeddedWindows);
    DrawProjectedNode(hdc, node->second, sourceSecond, targetSecond, layerIndex, includeEmbeddedWindows);

    if (borderVisible_)
    {
        RECT drawRect = SplitterDrawRect(targetSplitter, node->direction, SplitterSize());
        FillSolidRect(hdc, drawRect, kSplitViewerSplitterColor);
    }
}

void SplitViewerWindow::DrawLeaf(HDC hdc, SplitViewerNode* node, const RECT& rc)
{
    if (!PaintIntersects(rc))
    {
        return;
    }

    FillSolidRect(hdc, rc, kSplitViewerGrayColor);
    if (node->view.HasEmbeddedWindow())
    {
        return;
    }
    if (!node->view.HasImage())
    {
        DrawPlusButton(hdc, PlusButtonRect(rc));
        return;
    }

    const double imageWidth = static_cast<double>(node->view.image->GetWidth()) * node->view.scale;
    const double imageHeight = static_cast<double>(node->view.image->GetHeight()) * node->view.scale;
    const double centerX = (static_cast<double>(rc.left + rc.right) * 0.5) + node->view.offsetX;
    const double centerY = (static_cast<double>(rc.top + rc.bottom) * 0.5) + node->view.offsetY;

    if (ShouldUseFastImageRendering() && node->view.HasFastBitmap())
    {
        const int savedDc = SaveDC(hdc);
        bool drawnFast = false;
        if (savedDc != 0)
        {
            const int destLeft = static_cast<int>(std::floor(centerX - imageWidth * 0.5 + 0.5));
            const int destTop = static_cast<int>(std::floor(centerY - imageHeight * 0.5 + 0.5));
            const int destWidth = std::max(1, static_cast<int>(std::floor(imageWidth + 0.5)));
            const int destHeight = std::max(1, static_cast<int>(std::floor(imageHeight + 0.5)));

            IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SetStretchBltMode(hdc, COLORONCOLOR);
            drawnFast = StretchBlt(hdc,
                destLeft,
                destTop,
                destWidth,
                destHeight,
                node->view.fastDc,
                0,
                0,
                node->view.fastBitmapWidth,
                node->view.fastBitmapHeight,
                SRCCOPY) != FALSE;
            RestoreDC(hdc, savedDc);
        }

        if (drawnFast)
        {
            return;
        }
    }

    Gdiplus::Graphics graphics(hdc);
    graphics.SetClip(Gdiplus::Rect(rc.left, rc.top, SplitViewerRectWidth(rc), SplitViewerRectHeight(rc)));
    graphics.SetInterpolationMode(CurrentImageInterpolationMode());
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    Gdiplus::RectF dest(static_cast<Gdiplus::REAL>(centerX - imageWidth * 0.5),
        static_cast<Gdiplus::REAL>(centerY - imageHeight * 0.5),
        static_cast<Gdiplus::REAL>(imageWidth),
        static_cast<Gdiplus::REAL>(imageHeight));
    graphics.DrawImage(node->view.image, dest);
    graphics.ResetClip();
}

void SplitViewerWindow::DrawProjectedLeaf(HDC hdc, SplitViewerNode* node, const RECT& sourceRect, const RECT& targetRect, bool includeEmbeddedWindows)
{
    FillSolidRect(hdc, targetRect, kSplitViewerGrayColor);
    if (node->view.HasEmbeddedWindow())
    {
        if (includeEmbeddedWindows)
        {
            DrawEmbeddedWindowSnapshot(hdc, node->view.embeddedWindow, targetRect);
        }
        else
        {
            DrawPlusButton(hdc, PlusButtonRect(targetRect));
        }
        return;
    }
    if (!node->view.HasImage())
    {
        DrawPlusButton(hdc, PlusButtonRect(targetRect));
        return;
    }

    const double sourceWidth = static_cast<double>(SplitViewerRectWidth(sourceRect));
    const double sourceHeight = static_cast<double>(SplitViewerRectHeight(sourceRect));
    const double targetWidth = static_cast<double>(SplitViewerRectWidth(targetRect));
    const double targetHeight = static_cast<double>(SplitViewerRectHeight(targetRect));
    if (sourceWidth <= 0.0 || sourceHeight <= 0.0 || targetWidth <= 0.0 || targetHeight <= 0.0)
    {
        return;
    }

    const SplitViewerImageView& view = node->view;
    const double sourceCenterX = (static_cast<double>(sourceRect.left + sourceRect.right) * 0.5) + view.offsetX;
    const double sourceCenterY = (static_cast<double>(sourceRect.top + sourceRect.bottom) * 0.5) + view.offsetY;
    const double normalizedX = (sourceCenterX - static_cast<double>(sourceRect.left)) / sourceWidth;
    const double normalizedY = (sourceCenterY - static_cast<double>(sourceRect.top)) / sourceHeight;
    const double targetCenterX = static_cast<double>(targetRect.left) + normalizedX * targetWidth;
    const double targetCenterY = static_cast<double>(targetRect.top) + normalizedY * targetHeight;

    double renderScale = view.autoFit ? FitImageScale(view, targetRect) : view.scale;
    if (!view.autoFit)
    {
        renderScale *= (targetWidth / sourceWidth + targetHeight / sourceHeight) * 0.5;
    }

    Gdiplus::Graphics graphics(hdc);
    graphics.SetClip(Gdiplus::Rect(targetRect.left, targetRect.top, SplitViewerRectWidth(targetRect), SplitViewerRectHeight(targetRect)));
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    const double imageWidth = static_cast<double>(view.image->GetWidth()) * renderScale;
    const double imageHeight = static_cast<double>(view.image->GetHeight()) * renderScale;
    Gdiplus::RectF dest(static_cast<Gdiplus::REAL>(targetCenterX - imageWidth * 0.5),
        static_cast<Gdiplus::REAL>(targetCenterY - imageHeight * 0.5),
        static_cast<Gdiplus::REAL>(imageWidth),
        static_cast<Gdiplus::REAL>(imageHeight));
    graphics.DrawImage(view.image, dest);
    graphics.ResetClip();
}

void SplitViewerWindow::DrawPlusButton(HDC hdc, const RECT& rc)
{
    if (!SplitViewerIsRectUsable(rc) || !PaintIntersects(rc))
    {
        return;
    }

    FillSolidRect(hdc, rc, RGB(238, 238, 238));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    HPEN plusPen = CreatePen(PS_SOLID, 3, RGB(70, 70, 70));
    SelectObject(hdc, plusPen);
    const int cx = (rc.left + rc.right) / 2;
    const int cy = (rc.top + rc.bottom) / 2;
    MoveToEx(hdc, cx - 10, cy, NULL);
    LineTo(hdc, cx + 11, cy);
    MoveToEx(hdc, cx, cy - 10, NULL);
    LineTo(hdc, cx, cy + 11);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(plusPen);
    DeleteObject(borderPen);
}

bool SplitViewerWindow::DrawEmbeddedWindowSnapshot(HDC hdc, HWND window, const RECT& targetRect)
{
    if (!hdc || !window || !IsWindow(window) || !SplitViewerIsRectUsable(targetRect))
    {
        return false;
    }

    const int targetWidth = std::max(1, SplitViewerRectWidth(targetRect));
    const int targetHeight = std::max(1, SplitViewerRectHeight(targetRect));
    RECT originalScreenRect = { 0 };
    GetWindowRect(window, &originalScreenRect);

    POINT originalClientPoint = { originalScreenRect.left, originalScreenRect.top };
    ScreenToClient(hwnd_, &originalClientPoint);
    const int originalWidth = std::max(1, SplitViewerRectWidth(originalScreenRect));
    const int originalHeight = std::max(1, SplitViewerRectHeight(originalScreenRect));
    const bool resizeForCapture = originalWidth != targetWidth || originalHeight != targetHeight;

    if (resizeForCapture)
    {
        SetWindowPos(window,
            NULL,
            originalClientPoint.x,
            originalClientPoint.y,
            targetWidth,
            targetHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
        UpdateWindow(window);
    }

    HDC screenDc = GetDC(hwnd_);
    HDC memDc = screenDc ? CreateCompatibleDC(screenDc) : NULL;
    HBITMAP bitmap = memDc ? CreateCompatibleBitmap(screenDc, targetWidth, targetHeight) : NULL;
    bool drawn = false;

    if (screenDc && memDc && bitmap)
    {
        HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);
        FillSolidRect(memDc, SplitViewerMakeRect(0, 0, targetWidth, targetHeight), kSplitViewerGrayColor);

        BOOL printed = PrintWindow(window, memDc, 0);
        if (!printed)
        {
            HDC windowDc = GetDC(window);
            if (windowDc)
            {
                printed = BitBlt(memDc, 0, 0, targetWidth, targetHeight, windowDc, 0, 0, SRCCOPY);
                ReleaseDC(window, windowDc);
            }
        }

        if (printed)
        {
            SetStretchBltMode(hdc, HALFTONE);
            SetBrushOrgEx(hdc, 0, 0, NULL);
            drawn = StretchBlt(hdc,
                targetRect.left,
                targetRect.top,
                targetWidth,
                targetHeight,
                memDc,
                0,
                0,
                targetWidth,
                targetHeight,
                SRCCOPY) != FALSE;
        }

        SelectObject(memDc, oldBitmap);
    }

    if (bitmap)
    {
        DeleteObject(bitmap);
    }
    if (memDc)
    {
        DeleteDC(memDc);
    }
    if (screenDc)
    {
        ReleaseDC(hwnd_, screenDc);
    }

    if (resizeForCapture && IsWindow(window))
    {
        SetWindowPos(window,
            NULL,
            originalClientPoint.x,
            originalClientPoint.y,
            originalWidth,
            originalHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
        UpdateWindow(window);
    }

    return drawn;
}

void SplitViewerWindow::DrawDropFrame(HDC hdc, const RECT& rc)
{
    if (!SplitViewerIsRectUsable(rc) || !PaintIntersects(rc))
    {
        return;
    }

    HPEN pen = CreatePen(PS_SOLID, 3, kSplitViewerExternalDropFrameColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void SplitViewerWindow::DrawFrame(HDC hdc, const RECT& rc, COLORREF color)
{
    if (!borderVisible_ || !SplitViewerIsRectUsable(rc) || !PaintIntersects(rc))
    {
        return;
    }

    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void SplitViewerWindow::DrawTip(HDC hdc, const RECT& client)
{
    if (tipText_.empty())
    {
        return;
    }

    RECT tipRect = GetTipRect(client);
    if (!PaintIntersects(tipRect))
    {
        return;
    }

    HFONT font = mainFont_ ? mainFont_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    FillSolidRect(hdc, tipRect, RGB(255, 252, 214));
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(92, 92, 70));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, tipRect.left, tipRect.top, tipRect.right, tipRect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(34, 34, 34));
    DrawTextW(hdc, tipText_.c_str(), -1, &tipRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SelectObject(hdc, oldFont);
}

RECT SplitViewerWindow::GetTipRect(const RECT& client) const
{
    if (tipText_.empty())
    {
        return SplitViewerMakeRect(0, 0, 0, 0);
    }

    RECT owner = tipOwnerRect_;
    if (!SplitViewerIsRectUsable(owner))
    {
        owner = client;
    }

    HDC hdc = hwnd_ ? GetDC(hwnd_) : NULL;
    SIZE textSize = { 42, 16 };
    if (hdc)
    {
        HFONT font = mainFont_ ? mainFont_ : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ oldFont = SelectObject(hdc, font);
        GetTextExtentPoint32W(hdc, tipText_.c_str(), static_cast<int>(tipText_.size()), &textSize);
        SelectObject(hdc, oldFont);
        ReleaseDC(hwnd_, hdc);
    }

    const int padX = 12;
    const int padY = 7;
    const int width = textSize.cx + padX * 2;
    const int height = textSize.cy + padY * 2;
    int left = (owner.left + owner.right - width) / 2;
    int top = (owner.top + owner.bottom - height) / 2;
    left = SplitViewerClampInt(left, client.left + 4, client.right - width - 4);
    top = SplitViewerClampInt(top, client.top + 4, client.bottom - height - 4);
    return SplitViewerMakeRect(left, top, left + width, top + height);
}

void SplitViewerWindow::FillSolidRect(HDC hdc, const RECT& rc, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);
}

void SplitViewerWindow::InvalidateStage()
{
    RECT stage = GetStageRect();
    if (SplitViewerIsRectUsable(stage))
    {
        InvalidateRect(hwnd_, &stage, FALSE);
        return;
    }

    InvalidateRect(hwnd_, NULL, FALSE);
}

void SplitViewerWindow::InvalidateStageRect(const RECT& rc)
{
    if (!SplitViewerIsRectUsable(rc))
    {
        return;
    }

    RECT stage = GetStageRect();
    RECT invalidRect = { 0 };
    if (SplitViewerIsRectUsable(stage) && IntersectRect(&invalidRect, &stage, &rc) && SplitViewerIsRectUsable(invalidRect))
    {
        InvalidateRect(hwnd_, &invalidRect, FALSE);
    }
}

void SplitViewerWindow::InvalidateStageRects(const RECT& first, const RECT& second)
{
    if (!SplitViewerIsRectUsable(first))
    {
        InvalidateStageRect(second);
        return;
    }
    if (!SplitViewerIsRectUsable(second))
    {
        InvalidateStageRect(first);
        return;
    }

    RECT invalidRect = { 0 };
    UnionRect(&invalidRect, &first, &second);
    InvalidateStageRect(invalidRect);
}

Gdiplus::InterpolationMode SplitViewerWindow::CurrentImageInterpolationMode() const
{
    if (dragMode_ != SPLITVIEWER_DRAG_NONE)
    {
        return Gdiplus::InterpolationModeNearestNeighbor;
    }

    return Gdiplus::InterpolationModeHighQualityBicubic;
}

bool SplitViewerWindow::SaveBitmapToFile(HBITMAP bitmapHandle, const std::wstring& path)
{
    if (!bitmapHandle)
    {
        return false;
    }

    Gdiplus::Bitmap bitmap(bitmapHandle, NULL);
    CLSID clsid = { 0 };
    return SplitViewerGetEncoderClsid(SplitViewerImageMimeForPath(path), &clsid) >= 0 &&
        bitmap.Save(path.c_str(), &clsid, NULL) == Gdiplus::Ok;
}

bool SplitViewerWindow::SaveBitmapToPngBytes(HBITMAP bitmapHandle, std::vector<BYTE>& pngBytes)
{
    pngBytes.clear();
    if (!bitmapHandle)
    {
        return false;
    }

    CLSID clsid = { 0 };
    if (SplitViewerGetEncoderClsid(L"image/png", &clsid) < 0)
    {
        return false;
    }

    IStream* stream = NULL;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &stream)) || !stream)
    {
        return false;
    }

    Gdiplus::Bitmap bitmap(bitmapHandle, NULL);
    if (bitmap.Save(stream, &clsid, NULL) != Gdiplus::Ok)
    {
        stream->Release();
        return false;
    }

    HGLOBAL global = NULL;
    if (FAILED(GetHGlobalFromStream(stream, &global)) || !global)
    {
        stream->Release();
        return false;
    }

    const SIZE_T size = GlobalSize(global);
    void* data = GlobalLock(global);
    if (!data || size == 0)
    {
        if (data)
        {
            GlobalUnlock(global);
        }
        stream->Release();
        return false;
    }

    pngBytes.resize(static_cast<size_t>(size));
    std::memcpy(&pngBytes[0], data, static_cast<size_t>(size));
    GlobalUnlock(global);
    stream->Release();
    return true;
}

void SplitViewerWindow::SetLayerRectFromStage(int index, const RECT& rc, const RECT& stage)
{
    if (index < 0 || index >= static_cast<int>(layers_.size()) || !SplitViewerIsRectUsable(stage))
    {
        return;
    }

    const double width = static_cast<double>(SplitViewerRectWidth(stage));
    const double height = static_cast<double>(SplitViewerRectHeight(stage));
    layers_[index]->rect.left = SplitViewerClampDouble(static_cast<double>(rc.left - stage.left) / width, 0.0, 1.0);
    layers_[index]->rect.top = SplitViewerClampDouble(static_cast<double>(rc.top - stage.top) / height, 0.0, 1.0);
    layers_[index]->rect.right = SplitViewerClampDouble(static_cast<double>(rc.right - stage.left) / width, 0.0, 1.0);
    layers_[index]->rect.bottom = SplitViewerClampDouble(static_cast<double>(rc.bottom - stage.top) / height, 0.0, 1.0);
}

void SplitViewerWindow::UpdateForCurrentStage(bool scaleManualViews)
{
    RECT newStage = GetStageRect();
    const bool stageChanged = SplitViewerIsRectUsable(lastStageRect_) &&
        SplitViewerIsRectUsable(newStage) &&
        !SplitViewerAreRectsEqual(lastStageRect_, newStage);
    if (scaleManualViews &&
        stageChanged)
    {
        SplitViewerDebugLogFormat(L"UpdateForCurrentStage scaling manual views old=%d,%d,%d,%d new=%d,%d,%d,%d.",
            lastStageRect_.left,
            lastStageRect_.top,
            lastStageRect_.right,
            lastStageRect_.bottom,
            newStage.left,
            newStage.top,
            newStage.right,
            newStage.bottom);
        ScaleViewsForStageChange(lastStageRect_, newStage);
    }
    else if (stageChanged)
    {
        SplitViewerDebugLogFormat(L"UpdateForCurrentStage changed without manual scaling old=%d,%d,%d,%d new=%d,%d,%d,%d scaleManualViews=%d.",
            lastStageRect_.left,
            lastStageRect_.top,
            lastStageRect_.right,
            lastStageRect_.bottom,
            newStage.left,
            newStage.top,
            newStage.right,
            newStage.bottom,
            scaleManualViews ? 1 : 0);
    }

    SyncAutoFitAll();
    lastStageRect_ = newStage;
}

void SplitViewerWindow::ScaleViewsForStageChange(const RECT& oldStage, const RECT& newStage)
{
    ScaleViewsForRectChange(baseRoot_, ContentRect(oldStage), ContentRect(newStage));

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        RECT oldLayer = GetLayerRect(static_cast<int>(i), oldStage);
        RECT newLayer = GetLayerRect(static_cast<int>(i), newStage);
        ScaleViewsForRectChange(layers_[i]->root, ContentRect(oldLayer), ContentRect(newLayer));
    }
}

void SplitViewerWindow::ScaleViewsForRectChange(SplitViewerNode* node, const RECT& oldRect, const RECT& newRect)
{
    if (!node || !SplitViewerIsRectUsable(oldRect) || !SplitViewerIsRectUsable(newRect))
    {
        return;
    }

    if (node->IsLeaf())
    {
        ScaleLeafViewForRectChange(node->view, oldRect, newRect);
        return;
    }

    RECT oldFirst = { 0 };
    RECT oldSplitter = { 0 };
    RECT oldSecond = { 0 };
    RECT newFirst = { 0 };
    RECT newSplitter = { 0 };
    RECT newSecond = { 0 };
    SplitNodeRects(oldRect, node, oldFirst, oldSplitter, oldSecond);
    SplitNodeRects(newRect, node, newFirst, newSplitter, newSecond);

    ScaleViewsForRectChange(node->first, oldFirst, newFirst);
    ScaleViewsForRectChange(node->second, oldSecond, newSecond);
}

void SplitViewerWindow::ScaleLeafViewForRectChange(SplitViewerImageView& view, const RECT& oldRect, const RECT& newRect)
{
    if (!view.HasImage() || !SplitViewerIsRectUsable(oldRect) || !SplitViewerIsRectUsable(newRect))
    {
        return;
    }

    const double oldWidth = static_cast<double>(SplitViewerRectWidth(oldRect));
    const double oldHeight = static_cast<double>(SplitViewerRectHeight(oldRect));
    const double newWidth = static_cast<double>(SplitViewerRectWidth(newRect));
    const double newHeight = static_cast<double>(SplitViewerRectHeight(newRect));
    if (oldWidth <= 0.0 || oldHeight <= 0.0 || newWidth <= 0.0 || newHeight <= 0.0)
    {
        return;
    }

    const double oldCenterX = (static_cast<double>(oldRect.left + oldRect.right) * 0.5) + view.offsetX;
    const double oldCenterY = (static_cast<double>(oldRect.top + oldRect.bottom) * 0.5) + view.offsetY;
    const double normalizedX = (oldCenterX - static_cast<double>(oldRect.left)) / oldWidth;
    const double normalizedY = (oldCenterY - static_cast<double>(oldRect.top)) / oldHeight;

    const double newCenterX = static_cast<double>(newRect.left) + normalizedX * newWidth;
    const double newCenterY = static_cast<double>(newRect.top) + normalizedY * newHeight;
    view.offsetX = newCenterX - (static_cast<double>(newRect.left + newRect.right) * 0.5);
    view.offsetY = newCenterY - (static_cast<double>(newRect.top + newRect.bottom) * 0.5);

    if (!view.autoFit)
    {
        const double scaleX = newWidth / oldWidth;
        const double scaleY = newHeight / oldHeight;
        const double scaleFactor = (scaleX + scaleY) * 0.5;
        if (scaleFactor > 0.00001)
        {
            view.scale *= scaleFactor;
        }
    }
}

void SplitViewerWindow::SyncAutoFitAll()
{
    if (!hwnd_)
    {
        return;
    }

    RECT stage = GetStageRect();
    SyncAutoFitNode(baseRoot_, ContentRect(stage));

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        RECT layerRect = GetLayerRect(static_cast<int>(i), stage);
        SyncAutoFitNode(layers_[i]->root, ContentRect(layerRect));
    }

    SyncEmbeddedWindows();
}

void SplitViewerWindow::SyncAutoFitOwner(SplitViewerNode* root, const RECT& ownerRect)
{
    if (!root || !SplitViewerIsRectUsable(ownerRect))
    {
        return;
    }

    SyncAutoFitNode(root, ownerRect);
    SyncEmbeddedNode(root, ownerRect);
}

void SplitViewerWindow::SyncAutoFitNode(SplitViewerNode* node, const RECT& rc)
{
    if (!node || !SplitViewerIsRectUsable(rc))
    {
        return;
    }

    if (node->IsLeaf())
    {
        if (node->view.HasImage() && node->view.autoFit)
        {
            node->view.scale = FitImageScale(node->view, rc);
        }
        return;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);
    SyncAutoFitNode(node->first, firstRect);
    SyncAutoFitNode(node->second, secondRect);
}

void SplitViewerWindow::SyncEmbeddedWindows()
{
    if (!hwnd_)
    {
        return;
    }

    RECT stage = GetStageRect();
    SyncEmbeddedNode(baseRoot_, ContentRect(stage));

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        RECT layerRect = GetLayerRect(static_cast<int>(i), stage);
        SyncEmbeddedNode(layers_[i]->root, ContentRect(layerRect));
    }
}

void SplitViewerWindow::SyncEmbeddedOwner(SplitViewerNode* root, const RECT& ownerRect)
{
    if (!root || !SplitViewerIsRectUsable(ownerRect))
    {
        return;
    }

    SyncEmbeddedNode(root, ownerRect);
}

void SplitViewerWindow::SyncEmbeddedNode(SplitViewerNode* node, const RECT& rc)
{
    if (!node)
    {
        return;
    }

    if (node->IsLeaf())
    {
        HWND embedded = node->view.embeddedWindow;
        if (!embedded)
        {
            return;
        }
        if (!IsWindow(embedded))
        {
            node->view.ForgetEmbeddedWindow();
            return;
        }

        if (!SplitViewerIsRectUsable(rc))
        {
            ShowWindow(embedded, SW_HIDE);
            return;
        }

        const DWORD startTick = GetTickCount();
        SetWindowPos(embedded,
            HWND_TOP,
            rc.left,
            rc.top,
            std::max(1, SplitViewerRectWidth(rc)),
            std::max(1, SplitViewerRectHeight(rc)),
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
        const DWORD elapsed = GetTickCount() - startTick;
        if (elapsed > 30)
        {
            SplitViewerDebugLogFormat(L"SyncEmbeddedNode slow SetWindowPos window=0x%p rect=%d,%d,%d,%d elapsedMs=%u.",
                embedded,
                rc.left,
                rc.top,
                rc.right,
                rc.bottom,
                static_cast<unsigned int>(elapsed));
        }
        return;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);
    SyncEmbeddedNode(node->first, firstRect);
    SyncEmbeddedNode(node->second, secondRect);
}

bool SplitViewerWindow::DeleteLeafFromSplit(SplitViewerNode* root, SplitViewerNode* target)
{
    if (!root || !target || root->IsLeaf())
    {
        return false;
    }

    if (root->first == target || root->second == target)
    {
        PromoteSiblingIntoNode(root, root->first == target ? root->second : root->first, target);
        return true;
    }

    return DeleteLeafFromSplit(root->first, target) ||
        DeleteLeafFromSplit(root->second, target);
}

void SplitViewerWindow::PromoteSiblingIntoNode(SplitViewerNode* parent, SplitViewerNode* sibling, SplitViewerNode* removed)
{
    if (!parent || !sibling || !removed)
    {
        return;
    }

    SplitViewerNode* oldFirst = parent->first;
    SplitViewerNode* oldSecond = parent->second;

    if (sibling->IsLeaf())
    {
        parent->view.TakeFrom(sibling->view);
        parent->kind = SPLITVIEWER_NODE_LEAF;
        parent->direction = SPLITVIEWER_SPLIT_HORIZONTAL;
        parent->ratio = 0.5;
        parent->first = NULL;
        parent->second = NULL;
    }
    else
    {
        parent->view.Clear();
        parent->kind = SPLITVIEWER_NODE_SPLIT;
        parent->direction = sibling->direction;
        parent->ratio = sibling->ratio;
        parent->first = sibling->first;
        parent->second = sibling->second;
        sibling->first = NULL;
        sibling->second = NULL;
    }

    delete oldFirst;
    delete oldSecond;
}

void SplitViewerWindow::ClearTransientNodeReferences()
{
    if (dragMode_ != SPLITVIEWER_DRAG_NONE)
    {
        ReleaseCapture();
    }

    dragMode_ = SPLITVIEWER_DRAG_NONE;
    dragNode_ = NULL;
    dragRoot_ = NULL;
    dragLayerIndex_ = -1;
    dragResizeHit_ = SPLITVIEWER_RESIZE_NONE;
    hoveredSplitter_ = NULL;
    hoveredSplitterLayer_ = -2;
    externalDropLeaf_ = NULL;
    HideEmbeddedResizePreviewFrames();
    ClearEmbeddedDragOutState();
    pendingEmbedLeaf_ = NULL;
    pendingEmbedWindow_ = NULL;
    pendingEmbedLayer_ = -1;
}

void SplitViewerWindow::OnLeftButtonDown(POINT pt, WPARAM keyState)
{
    SetFocus(hwnd_);

    if (!isFullScreen_ && pt.y >= 0 && pt.y < kSplitViewerToolbarHeight)
    {
        ReleaseCapture();
        SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return;
    }

    RECT stage = GetStageRect();
    if (!SplitViewerPointInRect(stage, pt))
    {
        return;
    }

    int layerIndex = -1;
    RECT layerRect = { 0 };
    if (FindTopLayer(pt, layerIndex, layerRect))
    {
        selectedLayer_ = layerIndex;
        int resizeHit = HitLayerResize(layerRect, pt);
        if (resizeHit != SPLITVIEWER_RESIZE_NONE)
        {
            BeginLayerResize(layerIndex, layerRect, resizeHit, pt);
            InvalidateStageRect(SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize));
            return;
        }

        if (keyState & MK_CONTROL)
        {
            BeginLayerMove(layerIndex, layerRect, pt);
            InvalidateStageRect(SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize));
            return;
        }
    }
    else
    {
        selectedLayer_ = -1;
    }

    SplitViewerSplitterHit splitterHit;
    if (FindTopSplitter(pt, splitterHit))
    {
        BeginSplitterDrag(splitterHit, pt);
        return;
    }

    SplitViewerLeafHit leafHit;
    if (FindTopLeaf(pt, leafHit))
    {
        selectedLayer_ = leafHit.layerIndex;
        if (!leafHit.leaf->view.HasContent() && SplitViewerPointInRect(PlusButtonRect(leafHit.leafRect), pt))
        {
            LoadImageForLeaf(leafHit);
            return;
        }

        if (leafHit.leaf->view.HasImage())
        {
            BeginImageDrag(leafHit, pt);
            return;
        }
    }

    InvalidateStage();
}

void SplitViewerWindow::OnLeftButtonUp(POINT)
{
    if (dragMode_ != SPLITVIEWER_DRAG_NONE)
    {
        const SplitViewerDragMode completedDragMode = dragMode_;
        const int completedLayerIndex = dragLayerIndex_;
        SplitViewerNode* completedNode = dragNode_;
        const double completedRatio = completedNode ? completedNode->ratio : 0.0;
        RECT invalidRect = dragLastInvalidRect_;
        if (completedDragMode == SPLITVIEWER_DRAG_LAYER_MOVE || completedDragMode == SPLITVIEWER_DRAG_LAYER_RESIZE)
        {
            RECT startInvalidRect = SplitViewerInflatedRect(dragLayerStartRect_, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize);
            if (SplitViewerIsRectUsable(startInvalidRect))
            {
                if (SplitViewerIsRectUsable(invalidRect))
                {
                    UnionRect(&invalidRect, &invalidRect, &startInvalidRect);
                }
                else
                {
                    invalidRect = startInvalidRect;
                }
            }
        }

        ReleaseCapture();
        dragMode_ = SPLITVIEWER_DRAG_NONE;
        dragNode_ = NULL;
        dragRoot_ = NULL;
        dragLayerIndex_ = -1;
        dragResizeHit_ = SPLITVIEWER_RESIZE_NONE;
        ZeroMemory(&dragLastInvalidRect_, sizeof(dragLastInvalidRect_));
        ZeroMemory(&dragLayerStartRect_, sizeof(dragLayerStartRect_));
        HideEmbeddedResizePreviewFrames();
        if (completedDragMode == SPLITVIEWER_DRAG_SPLITTER ||
            completedDragMode == SPLITVIEWER_DRAG_LAYER_MOVE ||
            completedDragMode == SPLITVIEWER_DRAG_LAYER_RESIZE)
        {
            SyncEmbeddedWindows();
        }
        InvalidateStageRect(invalidRect);
        SplitViewerDebugLogFormat(L"Drag end mode=%s layer=%d node=0x%p ratio=%.6f invalid=%d,%d,%d,%d.",
            DragModeText(completedDragMode),
            completedLayerIndex,
            completedNode,
            completedRatio,
            invalidRect.left,
            invalidRect.top,
            invalidRect.right,
            invalidRect.bottom);
    }
}

void SplitViewerWindow::OnMouseMove(POINT pt, WPARAM)
{
    TrackMouse();

    switch (dragMode_)
    {
    case SPLITVIEWER_DRAG_IMAGE:
        if (dragNode_ && dragNode_->IsLeaf())
        {
            dragNode_->view.offsetX = dragImageOffsetX_ + static_cast<double>(pt.x - dragStartPoint_.x);
            dragNode_->view.offsetY = dragImageOffsetY_ + static_cast<double>(pt.y - dragStartPoint_.y);
            InvalidateStageRect(dragLastInvalidRect_);
        }
        return;
    case SPLITVIEWER_DRAG_SPLITTER:
        UpdateSplitterDrag(pt);
        return;
    case SPLITVIEWER_DRAG_LAYER_MOVE:
        UpdateLayerMove(pt);
        return;
    case SPLITVIEWER_DRAG_LAYER_RESIZE:
        UpdateLayerResize(pt);
        return;
    case SPLITVIEWER_DRAG_NONE:
    default:
        UpdateHover(pt);
        return;
    }
}

void SplitViewerWindow::OnRightButtonUp(POINT pt)
{
    RECT stage = GetStageRect();
    if (!SplitViewerPointInRect(stage, pt))
    {
        return;
    }

    SplitViewerLeafHit leafHit;
    if (!FindTopLeaf(pt, leafHit))
    {
        return;
    }

    selectedLayer_ = leafHit.layerIndex;
    HMENU menu = CreatePopupMenu();
    AppendOwnerDrawMenuItem(menu, SPLITVIEWER_MENU_SPLIT_HORIZONTAL, L"\u6C34\u5E73\u5206\u5272", true);
    AppendOwnerDrawMenuItem(menu, SPLITVIEWER_MENU_SPLIT_VERTICAL, L"\u5782\u76F4\u5206\u5272", true);
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendOwnerDrawMenuItem(menu,
        SPLITVIEWER_MENU_DELETE_SPLIT,
        L"\u5220\u9664\u5206\u5C4F",
        CanDeleteLeafFromSplit(leafHit.root, leafHit.leaf));

    POINT screenPt = pt;
    ClientToScreen(hwnd_, &screenPt);
    const UINT command = TrackPopupMenu(menu,
        TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
        screenPt.x,
        screenPt.y,
        0,
        hwnd_,
        NULL);
    DestroyMenu(menu);

    if (command == SPLITVIEWER_MENU_SPLIT_HORIZONTAL || command == SPLITVIEWER_MENU_SPLIT_VERTICAL)
    {
        const SplitViewerSplitDirection direction = command == SPLITVIEWER_MENU_SPLIT_HORIZONTAL ? SPLITVIEWER_SPLIT_HORIZONTAL : SPLITVIEWER_SPLIT_VERTICAL;
        SplitViewerDebugLogFormat(L"Split command layer=%d direction=%s leaf=0x%p root=0x%p.",
            leafHit.layerIndex,
            SplitDirectionText(direction),
            leafHit.leaf,
            leafHit.root);
        leafHit.leaf->MakeSplit(direction);
        SyncAutoFitAll();
        UpdatePrimaryButtonText();
        UpdateMouseHookState();
        InvalidateStage();
    }
    else if (command == SPLITVIEWER_MENU_DELETE_SPLIT)
    {
        if (DeleteLeafFromSplit(leafHit.root, leafHit.leaf))
        {
            SplitViewerDebugLogFormat(L"Delete split success layer=%d leaf=0x%p root=0x%p.",
                leafHit.layerIndex,
                leafHit.leaf,
                leafHit.root);
            ClearTransientNodeReferences();
            SyncAutoFitAll();
            UpdatePrimaryButtonText();
            UpdateMouseHookState();
            InvalidateStage();
        }
        else
        {
            SplitViewerDebugLogFormat(L"Delete split failed layer=%d leaf=0x%p root=0x%p.",
                leafHit.layerIndex,
                leafHit.leaf,
                leafHit.root);
        }
    }
}

void SplitViewerWindow::OnDoubleClick(POINT pt)
{
    RECT stage = GetStageRect();
    if (SplitViewerPointInRect(stage, pt))
    {
        SplitViewerDebugLogFormat(L"Double click toggles fullscreen at=%d,%d fullScreen=%d.",
            pt.x,
            pt.y,
            isFullScreen_ ? 1 : 0);
        ToggleFullScreen();
    }
}

void SplitViewerWindow::OnMouseWheel(int delta, WPARAM keyState, POINT screenPt)
{
    const DWORD wheelStartTick = GetTickCount();
    POINT pt = screenPt;
    ScreenToClient(hwnd_, &pt);

    SplitViewerLeafHit leafHit;
    if (!FindTopLeaf(pt, leafHit) || !leafHit.leaf->view.HasImage())
    {
        return;
    }

    RECT client = { 0 };
    GetClientRect(hwnd_, &client);
    RECT oldTipRect = GetTipRect(client);

    selectedLayer_ = leafHit.layerIndex;
    SplitViewerImageView& view = leafHit.leaf->view;
    const double fitScale = FitImageScale(view, leafHit.leafRect);
    const bool wasAutoFit = view.autoFit;
    const double oldScale = view.scale;
    if (view.autoFit)
    {
        view.scale = fitScale;
        view.autoFit = false;
    }

    int steps = delta / WHEEL_DELTA;
    if (steps == 0)
    {
        steps = delta > 0 ? 1 : -1;
    }

    const double stepFactor = (keyState & MK_CONTROL) ? 1.01 : 1.05;
    const double factor = std::pow(stepFactor, static_cast<double>(steps));
    const double minScale = std::max(0.0001, fitScale * 0.05);
    const double maxScale = std::max(minScale * 10.0, fitScale * 50.0);
    view.scale = SplitViewerClampDouble(view.scale * factor, minScale, maxScale);

    ShowZoomTip(leafHit.layerIndex, leafHit.ownerRect, view.scale, fitScale);
    RECT newTipRect = GetTipRect(client);
    RECT invalidRect = leafHit.leafRect;
    if (SplitViewerIsRectUsable(oldTipRect))
    {
        UnionRect(&invalidRect, &invalidRect, &oldTipRect);
    }
    if (SplitViewerIsRectUsable(newTipRect))
    {
        UnionRect(&invalidRect, &invalidRect, &newTipRect);
    }

    if (SplitViewerIsRectUsable(zoomFinishInvalidRect_))
    {
        UnionRect(&zoomFinishInvalidRect_, &zoomFinishInvalidRect_, &invalidRect);
    }
    else
    {
        zoomFinishInvalidRect_ = invalidRect;
    }

    InvalidateStageRect(invalidRect);
    const double percent = fitScale > 0.00001 ? view.scale / fitScale * 100.0 : 100.0;
    SplitViewerDebugLogFormat(L"Mouse wheel zoom layer=%d delta=%d ctrl=%d steps=%d autoFitWas=%d scale=%.6f->%.6f fit=%.6f percent=%.2f leaf=%d,%d,%d,%d invalid=%d,%d,%d,%d elapsedMs=%u.",
        leafHit.layerIndex,
        delta,
        (keyState & MK_CONTROL) ? 1 : 0,
        steps,
        wasAutoFit ? 1 : 0,
        oldScale,
        view.scale,
        fitScale,
        percent,
        leafHit.leafRect.left,
        leafHit.leafRect.top,
        leafHit.leafRect.right,
        leafHit.leafRect.bottom,
        invalidRect.left,
        invalidRect.top,
        invalidRect.right,
        invalidRect.bottom,
        static_cast<unsigned int>(GetTickCount() - wheelStartTick));
}

void SplitViewerWindow::OnKeyDown(WPARAM key)
{
    switch (key)
    {
    case VK_ESCAPE:
        if (isFullScreen_)
        {
            ExitFullScreen();
        }
        break;
    case VK_F1:
        borderVisible_ = !borderVisible_;
        SplitViewerDebugLogFormat(L"F1 borderVisible=%d.", borderVisible_ ? 1 : 0);
        UpdateForCurrentStage(false);
        InvalidateStage();
        break;
    case VK_F11:
        SplitViewerDebugLog(L"F11 toggles fullscreen.");
        ToggleFullScreen();
        break;
    default:
        break;
    }
}

void SplitViewerWindow::OnGetMinMaxInfo(MINMAXINFO* info)
{
    if (!info)
    {
        return;
    }

    info->ptMinTrackSize.x = 420;
    info->ptMinTrackSize.y = 320;
}

void SplitViewerWindow::BeginImageDrag(const SplitViewerLeafHit& hit, POINT pt)
{
    dragMode_ = SPLITVIEWER_DRAG_IMAGE;
    dragNode_ = hit.leaf;
    dragRoot_ = hit.root;
    dragLayerIndex_ = hit.layerIndex;
    dragStartPoint_ = pt;
    dragImageOffsetX_ = hit.leaf->view.offsetX;
    dragImageOffsetY_ = hit.leaf->view.offsetY;
    dragLastInvalidRect_ = hit.leafRect;
    SetCapture(hwnd_);
    InvalidateStageRect(dragLastInvalidRect_);
    SplitViewerDebugLogFormat(L"Drag begin mode=image layer=%d node=0x%p start=%d,%d offset=%.2f,%.2f rect=%d,%d,%d,%d.",
        hit.layerIndex,
        hit.leaf,
        pt.x,
        pt.y,
        dragImageOffsetX_,
        dragImageOffsetY_,
        hit.leafRect.left,
        hit.leafRect.top,
        hit.leafRect.right,
        hit.leafRect.bottom);
}

void SplitViewerWindow::BeginSplitterDrag(const SplitViewerSplitterHit& hit, POINT pt)
{
    dragMode_ = SPLITVIEWER_DRAG_SPLITTER;
    dragRoot_ = hit.root;
    dragNode_ = hit.splitNode;
    dragLayerIndex_ = hit.layerIndex;
    dragLastInvalidRect_ = SplitViewerInflatedRect(hit.nodeRect, kSplitViewerSplitterHoverSize, kSplitViewerSplitterHoverSize);
    dragStartPoint_ = pt;
    ShowEmbeddedResizePreviews(dragNode_, hit.nodeRect);
    SetCapture(hwnd_);
    SplitViewerDebugLogFormat(L"Drag begin mode=splitter layer=%d node=0x%p direction=%s ratio=%.6f start=%d,%d nodeRect=%d,%d,%d,%d.",
        hit.layerIndex,
        hit.splitNode,
        hit.splitNode ? SplitDirectionText(hit.splitNode->direction) : L"unknown",
        hit.splitNode ? hit.splitNode->ratio : 0.0,
        pt.x,
        pt.y,
        hit.nodeRect.left,
        hit.nodeRect.top,
        hit.nodeRect.right,
        hit.nodeRect.bottom);
}

void SplitViewerWindow::BeginLayerMove(int layerIndex, const RECT& layerRect, POINT pt)
{
    dragMode_ = SPLITVIEWER_DRAG_LAYER_MOVE;
    dragLayerIndex_ = layerIndex;
    dragLayerStartRect_ = layerRect;
    dragLastInvalidRect_ = SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize);
    dragStartPoint_ = pt;
    ShowEmbeddedResizePreviews(layers_[layerIndex]->root, ContentRect(layerRect));
    SetCapture(hwnd_);
    SplitViewerDebugLogFormat(L"Drag begin mode=layer_move layer=%d start=%d,%d rect=%d,%d,%d,%d.",
        layerIndex,
        pt.x,
        pt.y,
        layerRect.left,
        layerRect.top,
        layerRect.right,
        layerRect.bottom);
}

void SplitViewerWindow::BeginLayerResize(int layerIndex, const RECT& layerRect, int resizeHit, POINT pt)
{
    dragMode_ = SPLITVIEWER_DRAG_LAYER_RESIZE;
    dragLayerIndex_ = layerIndex;
    dragResizeHit_ = resizeHit;
    dragLayerStartRect_ = layerRect;
    dragLastInvalidRect_ = SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize);
    dragStartPoint_ = pt;
    ShowEmbeddedResizePreviews(layers_[layerIndex]->root, ContentRect(layerRect));
    SetCapture(hwnd_);
    SplitViewerDebugLogFormat(L"Drag begin mode=layer_resize layer=%d hit=%d start=%d,%d rect=%d,%d,%d,%d.",
        layerIndex,
        resizeHit,
        pt.x,
        pt.y,
        layerRect.left,
        layerRect.top,
        layerRect.right,
        layerRect.bottom);
}

void SplitViewerWindow::UpdateSplitterDrag(POINT pt)
{
    if (!dragNode_ || !dragRoot_)
    {
        return;
    }

    RECT stage = GetStageRect();
    RECT ownerRect = stage;
    if (dragLayerIndex_ >= 0)
    {
        ownerRect = GetLayerRect(dragLayerIndex_, stage);
    }

    RECT rootRect = ContentRect(ownerRect);
    RECT nodeRect = { 0 };
    if (!FindNodeRect(dragRoot_, rootRect, dragNode_, nodeRect))
    {
        return;
    }

    double newRatio = dragNode_->ratio;
    if (dragNode_->direction == SPLITVIEWER_SPLIT_HORIZONTAL)
    {
        const int splitterSize = SplitterSize();
        const int available = std::max(2, SplitViewerRectHeight(nodeRect) - splitterSize);
        double ratio = (static_cast<double>(pt.y - nodeRect.top) - splitterSize * 0.5) / static_cast<double>(available);
        const double minRatio = available > kSplitViewerMinPaneSize * 2 ? static_cast<double>(kSplitViewerMinPaneSize) / static_cast<double>(available) : 0.1;
        newRatio = SplitViewerClampDouble(ratio, minRatio, 1.0 - minRatio);
    }
    else
    {
        const int splitterSize = SplitterSize();
        const int available = std::max(2, SplitViewerRectWidth(nodeRect) - splitterSize);
        double ratio = (static_cast<double>(pt.x - nodeRect.left) - splitterSize * 0.5) / static_cast<double>(available);
        const double minRatio = available > kSplitViewerMinPaneSize * 2 ? static_cast<double>(kSplitViewerMinPaneSize) / static_cast<double>(available) : 0.1;
        newRatio = SplitViewerClampDouble(ratio, minRatio, 1.0 - minRatio);
    }

    if (std::fabs(newRatio - dragNode_->ratio) < 0.000001)
    {
        return;
    }

    dragNode_->ratio = newRatio;
    SyncAutoFitNode(dragNode_, nodeRect);
    ShowEmbeddedResizePreviews(dragNode_, nodeRect);
    dragLastInvalidRect_ = SplitViewerInflatedRect(nodeRect, kSplitViewerSplitterHoverSize, kSplitViewerSplitterHoverSize);
    InvalidateStageRect(dragLastInvalidRect_);
}

void SplitViewerWindow::UpdateLayerMove(POINT pt)
{
    if (dragLayerIndex_ < 0 || dragLayerIndex_ >= static_cast<int>(layers_.size()))
    {
        return;
    }

    RECT stage = GetStageRect();
    RECT rc = dragLayerStartRect_;
    OffsetRect(&rc, pt.x - dragStartPoint_.x, pt.y - dragStartPoint_.y);
    ConstrainRectToBounds(rc, stage);
    SetLayerRectFromStage(dragLayerIndex_, rc, stage);
    RECT layerRect = GetLayerRect(dragLayerIndex_, stage);
    RECT newInvalidRect = SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize);
    if (SplitViewerAreRectsEqual(dragLastInvalidRect_, newInvalidRect))
    {
        return;
    }

    ShowEmbeddedResizePreviews(layers_[dragLayerIndex_]->root, ContentRect(layerRect));
    InvalidateStageRects(dragLastInvalidRect_, newInvalidRect);
    dragLastInvalidRect_ = newInvalidRect;
}

void SplitViewerWindow::UpdateLayerResize(POINT pt)
{
    if (dragLayerIndex_ < 0 || dragLayerIndex_ >= static_cast<int>(layers_.size()))
    {
        return;
    }

    RECT stage = GetStageRect();
    RECT rc = dragLayerStartRect_;
    const int dx = pt.x - dragStartPoint_.x;
    const int dy = pt.y - dragStartPoint_.y;

    if (dragResizeHit_ & SPLITVIEWER_RESIZE_LEFT)
    {
        rc.left += dx;
    }
    if (dragResizeHit_ & SPLITVIEWER_RESIZE_RIGHT)
    {
        rc.right += dx;
    }
    if (dragResizeHit_ & SPLITVIEWER_RESIZE_TOP)
    {
        rc.top += dy;
    }
    if (dragResizeHit_ & SPLITVIEWER_RESIZE_BOTTOM)
    {
        rc.bottom += dy;
    }

    NormalizeLayerResizeRect(rc, stage);
    SetLayerRectFromStage(dragLayerIndex_, rc, stage);
    RECT layerRect = GetLayerRect(dragLayerIndex_, stage);
    RECT newInvalidRect = SplitViewerInflatedRect(layerRect, kSplitViewerLayerHitSize, kSplitViewerLayerHitSize);
    if (SplitViewerAreRectsEqual(dragLastInvalidRect_, newInvalidRect))
    {
        return;
    }

    SyncAutoFitNode(layers_[dragLayerIndex_]->root, ContentRect(layerRect));
    ShowEmbeddedResizePreviews(layers_[dragLayerIndex_]->root, ContentRect(layerRect));
    InvalidateStageRects(dragLastInvalidRect_, newInvalidRect);
    dragLastInvalidRect_ = newInvalidRect;
}

void SplitViewerWindow::UpdateHover(POINT pt)
{
    int resizeLayer = -1;
    int resizeHit = SPLITVIEWER_RESIZE_NONE;
    RECT layerRect = { 0 };
    if (FindTopLayer(pt, resizeLayer, layerRect))
    {
        resizeHit = HitLayerResize(layerRect, pt);
    }

    SplitViewerNode* splitNode = NULL;
    int splitLayer = -2;
    if (resizeHit == SPLITVIEWER_RESIZE_NONE)
    {
        SplitViewerSplitterHit splitterHit;
        if (FindTopSplitter(pt, splitterHit))
        {
            splitNode = splitterHit.splitNode;
            splitLayer = splitterHit.layerIndex;
        }
    }

    const bool changed = resizeLayer != hoverResizeLayer_ ||
        resizeHit != hoverResizeHit_ ||
        splitNode != hoveredSplitter_ ||
        splitLayer != hoveredSplitterLayer_;

    RECT oldSplitterRect = { 0 };
    bool hasOldSplitterRect = false;
    if (hoveredSplitter_)
    {
        SplitViewerNode* oldRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
            layers_[hoveredSplitterLayer_]->root :
        baseRoot_;
        hasOldSplitterRect = GetSplitterVisualRect(oldRoot, hoveredSplitterLayer_, hoveredSplitter_, true, oldSplitterRect);
    }

    hoverResizeLayer_ = resizeLayer;
    hoverResizeHit_ = resizeHit;
    hoveredSplitter_ = splitNode;
    hoveredSplitterLayer_ = splitLayer;

    if (changed)
    {
        RECT newSplitterRect = { 0 };
        bool hasNewSplitterRect = false;
        if (hoveredSplitter_)
        {
            SplitViewerNode* newRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
                layers_[hoveredSplitterLayer_]->root :
            baseRoot_;
            hasNewSplitterRect = GetSplitterVisualRect(newRoot, hoveredSplitterLayer_, hoveredSplitter_, true, newSplitterRect);
        }

        if (hasOldSplitterRect || hasNewSplitterRect)
        {
            InvalidateStageRects(oldSplitterRect, newSplitterRect);
        }
    }

    ApplyHoverCursor();
}

void SplitViewerWindow::ClearHover()
{
    if (hoveredSplitter_ || hoverResizeHit_ != SPLITVIEWER_RESIZE_NONE)
    {
        RECT oldSplitterRect = { 0 };
        bool hasOldSplitterRect = false;
        if (hoveredSplitter_)
        {
            SplitViewerNode* oldRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
                layers_[hoveredSplitterLayer_]->root :
            baseRoot_;
            hasOldSplitterRect = GetSplitterVisualRect(oldRoot, hoveredSplitterLayer_, hoveredSplitter_, true, oldSplitterRect);
        }

        hoveredSplitter_ = NULL;
        hoveredSplitterLayer_ = -2;
        hoverResizeLayer_ = -1;
        hoverResizeHit_ = SPLITVIEWER_RESIZE_NONE;
        if (hasOldSplitterRect)
        {
            InvalidateStageRect(oldSplitterRect);
        }
    }
}

bool SplitViewerWindow::ApplyHoverCursor()
{
    if (dragMode_ == SPLITVIEWER_DRAG_SPLITTER && dragNode_)
    {
        SetCursor(LoadCursor(NULL, dragNode_->direction == SPLITVIEWER_SPLIT_HORIZONTAL ? IDC_SIZENS : IDC_SIZEWE));
        return true;
    }
    if (dragMode_ == SPLITVIEWER_DRAG_LAYER_MOVE)
    {
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return true;
    }
    if (dragMode_ == SPLITVIEWER_DRAG_LAYER_RESIZE)
    {
        SetCursorForResizeHit(dragResizeHit_);
        return true;
    }
    if (hoverResizeHit_ != SPLITVIEWER_RESIZE_NONE)
    {
        SetCursorForResizeHit(hoverResizeHit_);
        return true;
    }
    if (hoveredSplitter_)
    {
        SetCursor(LoadCursor(NULL, hoveredSplitter_->direction == SPLITVIEWER_SPLIT_HORIZONTAL ? IDC_SIZENS : IDC_SIZEWE));
        return true;
    }
    return false;
}

void SplitViewerWindow::SetCursorForResizeHit(int hit)
{
    if ((hit & SPLITVIEWER_RESIZE_LEFT) && (hit & SPLITVIEWER_RESIZE_TOP))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
    }
    else if ((hit & SPLITVIEWER_RESIZE_RIGHT) && (hit & SPLITVIEWER_RESIZE_BOTTOM))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
    }
    else if ((hit & SPLITVIEWER_RESIZE_RIGHT) && (hit & SPLITVIEWER_RESIZE_TOP))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENESW));
    }
    else if ((hit & SPLITVIEWER_RESIZE_LEFT) && (hit & SPLITVIEWER_RESIZE_BOTTOM))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENESW));
    }
    else if (hit & (SPLITVIEWER_RESIZE_LEFT | SPLITVIEWER_RESIZE_RIGHT))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
    }
    else if (hit & (SPLITVIEWER_RESIZE_TOP | SPLITVIEWER_RESIZE_BOTTOM))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
    }
}

void SplitViewerWindow::TrackMouse()
{
    if (trackingMouse_)
    {
        return;
    }

    TRACKMOUSEEVENT tme = { 0 };
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hwnd_;
    if (TrackMouseEvent(&tme))
    {
        trackingMouse_ = true;
    }
}

void SplitViewerWindow::LoadImageForLeaf(const SplitViewerLeafHit& hit)
{
    std::wstring path;
    if (!BrowseOpenImage(path))
    {
        SplitViewerDebugLog(L"LoadImageForLeaf cancelled.");
        return;
    }

    SplitViewerDebugLogFormat(L"LoadImageForLeaf selected layer=%d leaf=0x%p path=%s.",
        hit.layerIndex,
        hit.leaf,
        path.c_str());
    if (!hit.leaf->view.Load(path))
    {
        SplitViewerDebugLogFormat(L"LoadImageForLeaf failed layer=%d leaf=0x%p path=%s.",
            hit.layerIndex,
            hit.leaf,
            path.c_str());
        MessageBoxW(hwnd_, L"\u56FE\u7247\u52A0\u8F7D\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return;
    }

    SyncAutoFitAll();
    UpdatePrimaryButtonText();
    UpdateMouseHookState();
    InvalidateStage();
    SplitViewerDebugLogFormat(L"LoadImageForLeaf success layer=%d leaf=0x%p path=%s.",
        hit.layerIndex,
        hit.leaf,
        path.c_str());
}

bool SplitViewerWindow::BrowseOpenImage(std::wstring& path)
{
    wchar_t fileName[4096] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"\u56FE\u7247\u6587\u4EF6\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff\0\u6240\u6709\u6587\u4EF6\0*.*\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = ARRAYSIZE(fileName);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"\u9009\u62E9\u56FE\u7247";
    if (!GetOpenFileNameW(&ofn))
    {
        const DWORD error = CommDlgExtendedError();
        SplitViewerDebugLogFormat(L"BrowseOpenImage closed error=%u.", static_cast<unsigned int>(error));
        return false;
    }

    path = fileName;
    SplitViewerDebugLogFormat(L"BrowseOpenImage success path=%s.", path.c_str());
    return true;
}

bool SplitViewerWindow::BrowseSaveImage(std::wstring& path)
{
    wchar_t fileName[4096] = { 0 };
    std::wstring defaultPath = lastImageSavePath_;
    if (defaultPath.empty())
    {
        defaultPath = SplitViewerMakeTimestampName() + L".png";
    }
    StringCchCopyW(fileName, ARRAYSIZE(fileName), defaultPath.c_str());

    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"PNG \u56FE\u7247\0*.png\0JPEG \u56FE\u7247\0*.jpg\0BMP \u56FE\u7247\0*.bmp\0\u6240\u6709\u6587\u4EF6\0*.*\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = ARRAYSIZE(fileName);
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = L"\u4FDD\u5B58\u622A\u56FE";
    if (!GetSaveFileNameW(&ofn))
    {
        const DWORD error = CommDlgExtendedError();
        SplitViewerDebugLogFormat(L"BrowseSaveImage closed error=%u default=%s.",
            static_cast<unsigned int>(error),
            defaultPath.c_str());
        return false;
    }

    path = SplitViewerWithDefaultExtension(fileName, L".png");
    SplitViewerDebugLogFormat(L"BrowseSaveImage success path=%s.", path.c_str());
    return true;
}

bool SplitViewerWindow::BrowseSaveConfig(std::wstring& path)
{
    wchar_t fileName[4096] = { 0 };
    std::wstring defaultPath = currentConfigPath_;
    if (defaultPath.empty())
    {
        defaultPath = SplitViewerMakeTimestampName() + L".sv";
    }
    StringCchCopyW(fileName, ARRAYSIZE(fileName), defaultPath.c_str());

    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"SplitViewer \u914D\u7F6E\0*.sv\0\u6240\u6709\u6587\u4EF6\0*.*\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = ARRAYSIZE(fileName);
    ofn.lpstrDefExt = L"sv";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrTitle = L"\u4FDD\u5B58\u914D\u7F6E";
    if (!GetSaveFileNameW(&ofn))
    {
        const DWORD error = CommDlgExtendedError();
        SplitViewerDebugLogFormat(L"BrowseSaveConfig closed error=%u default=%s.",
            static_cast<unsigned int>(error),
            defaultPath.c_str());
        return false;
    }

    path = SplitViewerWithDefaultExtension(fileName, L".sv");
    SplitViewerDebugLogFormat(L"BrowseSaveConfig success path=%s.", path.c_str());
    return true;
}

bool SplitViewerWindow::BrowseOpenConfig(std::wstring& path)
{
    wchar_t fileName[4096] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFilter = L"SplitViewer \u914D\u7F6E\0*.sv\0\u6240\u6709\u6587\u4EF6\0*.*\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = ARRAYSIZE(fileName);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"\u52A0\u8F7D\u914D\u7F6E";
    if (!GetOpenFileNameW(&ofn))
    {
        const DWORD error = CommDlgExtendedError();
        SplitViewerDebugLogFormat(L"BrowseOpenConfig closed error=%u.", static_cast<unsigned int>(error));
        return false;
    }

    path = fileName;
    SplitViewerDebugLogFormat(L"BrowseOpenConfig success path=%s.", path.c_str());
    return true;
}

void SplitViewerWindow::SaveStageImage()
{
    std::wstring path;
    if (!BrowseSaveImage(path))
    {
        SplitViewerDebugLog(L"SaveStageImage cancelled.");
        return;
    }

    const DWORD startTick = GetTickCount();
    RECT stage = GetStageRect();
    SIZE fullSize = GetFullScreenStageSize();
    if (!SplitViewerIsRectUsable(stage) || fullSize.cx <= 0 || fullSize.cy <= 0)
    {
        SplitViewerDebugLogFormat(L"SaveStageImage rejected stage=%d,%d,%d,%d fullSize=%dx%d path=%s.",
            stage.left,
            stage.top,
            stage.right,
            stage.bottom,
            fullSize.cx,
            fullSize.cy,
            path.c_str());
        return;
    }

    SplitViewerDebugLogFormat(L"SaveStageImage begin path=%s stage=%d,%d,%d,%d fullSize=%dx%d.",
        path.c_str(),
        stage.left,
        stage.top,
        stage.right,
        stage.bottom,
        fullSize.cx,
        fullSize.cy);
    HBITMAP bitmapHandle = NULL;
    if (!RenderProjectedStageBitmap(stage, fullSize.cx, fullSize.cy, bitmapHandle, true) ||
        !SaveBitmapToFile(bitmapHandle, path))
    {
        SplitViewerDebugLogFormat(L"SaveStageImage failed path=%s elapsedMs=%u.",
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
        MessageBoxW(hwnd_, L"\u622A\u56FE\u4FDD\u5B58\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
    }
    else
    {
        lastImageSavePath_ = path;
        SplitViewerDebugLogFormat(L"SaveStageImage success path=%s elapsedMs=%u.",
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
    }

    if (bitmapHandle)
    {
        DeleteObject(bitmapHandle);
    }
}

void SplitViewerWindow::SaveConfigDialog()
{
    std::wstring path = currentConfigPath_;
    if (path.empty() && !BrowseSaveConfig(path))
    {
        SplitViewerDebugLog(L"SaveConfigDialog cancelled.");
        return;
    }

    SplitViewerDebugLogFormat(L"SaveConfigDialog begin path=%s.", path.c_str());
    if (SaveConfigToFile(path))
    {
        currentConfigPath_ = path;
        UpdatePrimaryButtonText();
        SplitViewerDebugLogFormat(L"SaveConfigDialog success path=%s.", path.c_str());
    }
    else
    {
        SplitViewerDebugLogFormat(L"SaveConfigDialog failed path=%s.", path.c_str());
    }
}

void SplitViewerWindow::LoadConfigDialog()
{
    std::wstring path;
    if (BrowseOpenConfig(path))
    {
        SplitViewerDebugLogFormat(L"LoadConfigDialog selected path=%s.", path.c_str());
        LoadConfigFromFile(path);
    }
    else
    {
        SplitViewerDebugLog(L"LoadConfigDialog cancelled.");
    }
}

bool SplitViewerWindow::SaveConfigToFile(const std::wstring& path)
{
    const DWORD startTick = GetTickCount();
    SplitViewerDebugLogFormat(L"SaveConfigToFile begin path=%s layers=%d selectedLayer=%d border=%d fullScreen=%d.",
        path.c_str(),
        static_cast<int>(layers_.size()),
        selectedLayer_,
        borderVisible_ ? 1 : 0,
        isFullScreen_ ? 1 : 0);
    SplitViewerTempFileGuard profileTemp;
    if (!SplitViewerCreateTempProfilePath(profileTemp.path) || !SplitViewerCreateUtf16ProfileFile(profileTemp.path))
    {
        SplitViewerDebugLogFormat(L"SaveConfigToFile temp profile failed path=%s elapsedMs=%u.",
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    const std::wstring& profilePath = profileTemp.path;
    RECT windowRect = { 0 };
    if (isFullScreen_)
    {
        windowRect = savedPlacement_.rcNormalPosition;
    }
    else
    {
        GetWindowRect(hwnd_, &windowRect);
    }

    RECT stage = GetStageRect();
    SplitViewerWriteProfileString(profilePath, L"SplitViewer", L"Version", L"2");
    SplitViewerWriteProfileDouble(profilePath, L"SplitViewer", L"StageAspect", stageAspect_);
    SplitViewerWriteProfileInt(profilePath, L"SplitViewer", L"BorderVisible", borderVisible_ ? 1 : 0);
    SplitViewerWriteProfileInt(profilePath, L"SplitViewer", L"SelectedLayer", selectedLayer_);
    SplitViewerWriteProfileInt(profilePath, L"SplitViewer", L"LayerCount", static_cast<int>(layers_.size()));
    SplitViewerWriteProfileInt(profilePath, L"SplitViewer", L"StageWidth", SplitViewerRectWidth(stage));
    SplitViewerWriteProfileInt(profilePath, L"SplitViewer", L"StageHeight", SplitViewerRectHeight(stage));

    SplitViewerWriteProfileInt(profilePath, L"Window", L"Left", windowRect.left);
    SplitViewerWriteProfileInt(profilePath, L"Window", L"Top", windowRect.top);
    SplitViewerWriteProfileInt(profilePath, L"Window", L"Right", windowRect.right);
    SplitViewerWriteProfileInt(profilePath, L"Window", L"Bottom", windowRect.bottom);

    int baseNextId = 0;
    const int baseRootId = SaveNode(profilePath, L"Base", baseRoot_, baseNextId);
    SplitViewerWriteProfileInt(profilePath, L"Base", L"Root", baseRootId);
    SplitViewerWriteProfileInt(profilePath, L"Base", L"NodeCount", baseNextId);

    for (size_t i = 0; i < layers_.size(); ++i)
    {
        std::wstring layerSection = std::wstring(L"Layer") + SplitViewerFormatInt(static_cast<int>(i));
        SplitViewerWriteProfileDouble(profilePath, layerSection, L"Left", layers_[i]->rect.left);
        SplitViewerWriteProfileDouble(profilePath, layerSection, L"Top", layers_[i]->rect.top);
        SplitViewerWriteProfileDouble(profilePath, layerSection, L"Right", layers_[i]->rect.right);
        SplitViewerWriteProfileDouble(profilePath, layerSection, L"Bottom", layers_[i]->rect.bottom);

        int nextId = 0;
        std::wstring prefix = layerSection + L"_";
        const int rootId = SaveNode(profilePath, prefix, layers_[i]->root, nextId);
        SplitViewerWriteProfileInt(profilePath, layerSection, L"Root", rootId);
        SplitViewerWriteProfileInt(profilePath, layerSection, L"NodeCount", nextId);
    }

    WritePrivateProfileStringW(NULL, NULL, NULL, profilePath.c_str());

    std::vector<BYTE> configBytes;
    if (!SplitViewerReadFileBytes(profilePath, configBytes))
    {
        SplitViewerDebugLogFormat(L"SaveConfigToFile read temp profile failed temp=%s path=%s elapsedMs=%u.",
            profilePath.c_str(),
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    SIZE thumbSize = GetThumbnailStageSize(stage);
    HBITMAP thumbnailBitmap = NULL;
    std::vector<BYTE> thumbnailPng;
    const bool thumbnailOk = RenderProjectedStageBitmap(stage, thumbSize.cx, thumbSize.cy, thumbnailBitmap, false) &&
        SaveBitmapToPngBytes(thumbnailBitmap, thumbnailPng);
    if (thumbnailBitmap)
    {
        DeleteObject(thumbnailBitmap);
    }
    SplitViewerDebugLogFormat(L"SaveConfigToFile thumbnail result=%d size=%dx%d bytes=%u configBytes=%u.",
        thumbnailOk ? 1 : 0,
        thumbSize.cx,
        thumbSize.cy,
        static_cast<unsigned int>(thumbnailPng.size()),
        static_cast<unsigned int>(configBytes.size()));

    if (!thumbnailOk || !SplitViewerWriteConfigPackage(path, thumbnailPng, configBytes))
    {
        SplitViewerDebugLogFormat(L"SaveConfigToFile package failed path=%s elapsedMs=%u.",
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    SplitViewerDebugLogFormat(L"SaveConfigToFile success path=%s elapsedMs=%u.",
        path.c_str(),
        static_cast<unsigned int>(GetTickCount() - startTick));
    return true;
}

int SplitViewerWindow::SaveNode(const std::wstring& file, const std::wstring& prefix, SplitViewerNode* node, int& nextId)
{
    const int id = nextId++;
    const std::wstring section = SplitViewerProfileSectionName(prefix, id);

    if (!node || node->IsLeaf())
    {
        SplitViewerWriteProfileString(file, section, L"Kind", L"Leaf");
        if (node)
        {
            SplitViewerWriteProfileString(file, section, L"Path", node->view.path);
            SplitViewerWriteProfileInt(file, section, L"HasImage", node->view.HasImage() ? 1 : 0);
            SplitViewerWriteProfileInt(file, section, L"AutoFit", node->view.autoFit ? 1 : 0);
            SplitViewerWriteProfileDouble(file, section, L"Scale", node->view.scale);
            SplitViewerWriteProfileDouble(file, section, L"OffsetX", node->view.offsetX);
            SplitViewerWriteProfileDouble(file, section, L"OffsetY", node->view.offsetY);
        }
        return id;
    }

    SplitViewerWriteProfileString(file, section, L"Kind", L"Split");
    SplitViewerWriteProfileInt(file, section, L"Direction", node->direction == SPLITVIEWER_SPLIT_HORIZONTAL ? 0 : 1);
    SplitViewerWriteProfileDouble(file, section, L"Ratio", node->ratio);
    const int firstId = SaveNode(file, prefix, node->first, nextId);
    const int secondId = SaveNode(file, prefix, node->second, nextId);
    SplitViewerWriteProfileInt(file, section, L"First", firstId);
    SplitViewerWriteProfileInt(file, section, L"Second", secondId);
    return id;
}

bool SplitViewerWindow::LoadConfigFromFile(const std::wstring& path)
{
    const DWORD startTick = GetTickCount();
    SplitViewerDebugLogFormat(L"LoadConfigFromFile begin path=%s.", path.c_str());
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        SplitViewerDebugLogFormat(L"LoadConfigFromFile missing file path=%s.", path.c_str());
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u4E0D\u5B58\u5728\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    SplitViewerTempFileGuard tempProfile;
    std::wstring profilePath;
    if (!SplitViewerPrepareProfileReadFile(path, profilePath, tempProfile))
    {
        SplitViewerDebugLogFormat(L"LoadConfigFromFile prepare profile failed path=%s elapsedMs=%u.",
            path.c_str(),
            static_cast<unsigned int>(GetTickCount() - startTick));
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u8BFB\u53D6\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    if (isFullScreen_)
    {
        ExitFullScreen();
    }

    SplitViewerNode* newBase = LoadNode(profilePath, L"Base", SplitViewerReadProfileIntValue(profilePath, L"Base", L"Root", 0));
    if (!newBase)
    {
        SplitViewerDebugLogFormat(L"LoadConfigFromFile base root failed path=%s profile=%s.",
            path.c_str(),
            profilePath.c_str());
        MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u8BFB\u53D6\u5931\u8D25\u3002", kSplitViewerAppTitle, MB_OK | MB_ICONERROR);
        return false;
    }

    std::vector<SplitViewerLayer*> newLayers;
    const int layerCount = SplitViewerClampInt(SplitViewerReadProfileIntValue(profilePath, L"SplitViewer", L"LayerCount", 0), 0, 256);
    SplitViewerDebugLogFormat(L"LoadConfigFromFile profile=%s layerCount=%d.", profilePath.c_str(), layerCount);
    for (int i = 0; i < layerCount; ++i)
    {
        std::wstring layerSection = std::wstring(L"Layer") + SplitViewerFormatInt(i);
        SplitViewerLayer* layer = new SplitViewerLayer();
        layer->rect.left = SplitViewerClampDouble(SplitViewerReadProfileDoubleValue(profilePath, layerSection, L"Left", layer->rect.left), 0.0, 1.0);
        layer->rect.top = SplitViewerClampDouble(SplitViewerReadProfileDoubleValue(profilePath, layerSection, L"Top", layer->rect.top), 0.0, 1.0);
        layer->rect.right = SplitViewerClampDouble(SplitViewerReadProfileDoubleValue(profilePath, layerSection, L"Right", layer->rect.right), 0.0, 1.0);
        layer->rect.bottom = SplitViewerClampDouble(SplitViewerReadProfileDoubleValue(profilePath, layerSection, L"Bottom", layer->rect.bottom), 0.0, 1.0);

        if (layer->rect.right <= layer->rect.left + 0.02)
        {
            layer->rect.right = std::min(1.0, layer->rect.left + 0.2);
        }
        if (layer->rect.bottom <= layer->rect.top + 0.02)
        {
            layer->rect.bottom = std::min(1.0, layer->rect.top + 0.2);
        }

        delete layer->root;
        std::wstring prefix = layerSection + L"_";
        layer->root = LoadNode(profilePath, prefix, SplitViewerReadProfileIntValue(profilePath, layerSection, L"Root", 0));
        if (!layer->root)
        {
            layer->root = new SplitViewerNode();
        }
        newLayers.push_back(layer);
    }

    delete baseRoot_;
    baseRoot_ = newBase;
    ClearLayers();
    layers_.swap(newLayers);
    ZeroMemory(&lastStageRect_, sizeof(lastStageRect_));

    stageAspect_ = SplitViewerReadProfileDoubleValue(profilePath, L"SplitViewer", L"StageAspect", stageAspect_);
    if (stageAspect_ < 0.1)
    {
        stageAspect_ = 4.0 / 3.0;
    }
    borderVisible_ = SplitViewerReadProfileIntValue(profilePath, L"SplitViewer", L"BorderVisible", 1) != 0;
    selectedLayer_ = SplitViewerReadProfileIntValue(profilePath, L"SplitViewer", L"SelectedLayer", -1);
    if (selectedLayer_ >= static_cast<int>(layers_.size()))
    {
        selectedLayer_ = -1;
    }

    const int left = SplitViewerReadProfileIntValue(profilePath, L"Window", L"Left", CW_USEDEFAULT);
    const int top = SplitViewerReadProfileIntValue(profilePath, L"Window", L"Top", CW_USEDEFAULT);
    const int right = SplitViewerReadProfileIntValue(profilePath, L"Window", L"Right", CW_USEDEFAULT);
    const int bottom = SplitViewerReadProfileIntValue(profilePath, L"Window", L"Bottom", CW_USEDEFAULT);
    if (right > left && bottom > top && left != CW_USEDEFAULT)
    {
        SetWindowPos(hwnd_, NULL, left, top, right - left, bottom - top, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    UpdateForCurrentStage(false);
    InvalidateRect(hwnd_, NULL, FALSE);
    currentConfigPath_ = path;
    UpdatePrimaryButtonText();
    UpdateMouseHookState();

    for (size_t i = 0; i < newLayers.size(); ++i)
    {
        delete newLayers[i];
    }
    SplitViewerDebugLogFormat(L"LoadConfigFromFile success path=%s layers=%d selectedLayer=%d border=%d aspect=%.6f elapsedMs=%u.",
        path.c_str(),
        static_cast<int>(layers_.size()),
        selectedLayer_,
        borderVisible_ ? 1 : 0,
        stageAspect_,
        static_cast<unsigned int>(GetTickCount() - startTick));
    return true;
}

SplitViewerNode* SplitViewerWindow::LoadNode(const std::wstring& file, const std::wstring& prefix, int id)
{
    const std::wstring section = SplitViewerProfileSectionName(prefix, id);
    const std::wstring kind = SplitViewerReadProfileString(file, section, L"Kind", L"Leaf");
    SplitViewerNode* node = new SplitViewerNode();

    if (SplitViewerSameText(kind.c_str(), L"Split"))
    {
        node->kind = SPLITVIEWER_NODE_SPLIT;
        node->direction = SplitViewerReadProfileIntValue(file, section, L"Direction", 0) == 0 ? SPLITVIEWER_SPLIT_HORIZONTAL : SPLITVIEWER_SPLIT_VERTICAL;
        node->ratio = SplitViewerClampDouble(SplitViewerReadProfileDoubleValue(file, section, L"Ratio", 0.5), 0.02, 0.98);
        delete node->first;
        delete node->second;
        node->first = LoadNode(file, prefix, SplitViewerReadProfileIntValue(file, section, L"First", 0));
        node->second = LoadNode(file, prefix, SplitViewerReadProfileIntValue(file, section, L"Second", 0));
        if (!node->first)
        {
            node->first = new SplitViewerNode();
        }
        if (!node->second)
        {
            node->second = new SplitViewerNode();
        }
        return node;
    }

    const std::wstring imagePath = SplitViewerReadProfileString(file, section, L"Path", L"");
    if (!imagePath.empty())
    {
        if (!node->view.Load(imagePath))
        {
            SplitViewerDebugLogFormat(L"LoadNode image unavailable section=%s path=%s; leaf remains empty.",
                section.c_str(),
                imagePath.c_str());
            node->view.Clear();
        }
    }
    node->view.autoFit = SplitViewerReadProfileIntValue(file, section, L"AutoFit", 1) != 0;
    node->view.scale = SplitViewerReadProfileDoubleValue(file, section, L"Scale", node->view.scale);
    node->view.offsetX = SplitViewerReadProfileDoubleValue(file, section, L"OffsetX", 0.0);
    node->view.offsetY = SplitViewerReadProfileDoubleValue(file, section, L"OffsetY", 0.0);
    return node;
}

void SplitViewerWindow::ClearLayers()
{
    for (size_t i = 0; i < layers_.size(); ++i)
    {
        delete layers_[i];
    }
    layers_.clear();
}

void SplitViewerWindow::AddLayer()
{
    SplitViewerLayer* layer = new SplitViewerLayer();
    const double offset = 0.03 * static_cast<double>(layers_.size() % 6);
    layer->rect.left = SplitViewerClampDouble(0.22 + offset, 0.0, 0.72);
    layer->rect.top = SplitViewerClampDouble(0.22 + offset, 0.0, 0.72);
    layer->rect.right = SplitViewerClampDouble(layer->rect.left + 0.46, 0.25, 1.0);
    layer->rect.bottom = SplitViewerClampDouble(layer->rect.top + 0.46, 0.25, 1.0);
    layers_.push_back(layer);
    selectedLayer_ = static_cast<int>(layers_.size()) - 1;
    UpdateForCurrentStage(true);
    UpdatePrimaryButtonText();
    UpdateMouseHookState();
    InvalidateRect(hwnd_, NULL, FALSE);
    SplitViewerDebugLogFormat(L"AddLayer index=%d rect=%.4f,%.4f,%.4f,%.4f total=%d.",
        selectedLayer_,
        layer->rect.left,
        layer->rect.top,
        layer->rect.right,
        layer->rect.bottom,
        static_cast<int>(layers_.size()));
}

void SplitViewerWindow::ToggleFullScreen()
{
    if (isFullScreen_)
    {
        ExitFullScreen();
    }
    else
    {
        EnterFullScreen();
    }
}

void SplitViewerWindow::EnterFullScreen()
{
    if (isFullScreen_)
    {
        return;
    }

    isFullScreen_ = true;
    savedStyle_ = GetWindowLongW(hwnd_, GWL_STYLE);
    savedExStyle_ = GetWindowLongW(hwnd_, GWL_EXSTYLE);
    savedPlacement_.length = sizeof(savedPlacement_);
    GetWindowPlacement(hwnd_, &savedPlacement_);

    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);
    RECT oldStage = GetStageRect();
    SplitViewerDebugLogFormat(L"EnterFullScreen begin oldStage=%d,%d,%d,%d monitor=%d,%d,%d,%d.",
        oldStage.left,
        oldStage.top,
        oldStage.right,
        oldStage.bottom,
        monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.top,
        monitorInfo.rcMonitor.right,
        monitorInfo.rcMonitor.bottom);

    SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_ & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_ & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
    LayoutToolbar();
    SetWindowPos(hwnd_,
        HWND_TOP,
        monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.top,
        SplitViewerRectWidth(monitorInfo.rcMonitor),
        SplitViewerRectHeight(monitorInfo.rcMonitor),
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    UpdateForCurrentStage(true);
    InvalidateRect(hwnd_, NULL, FALSE);
    RECT newStage = GetStageRect();
    SplitViewerDebugLogFormat(L"EnterFullScreen success newStage=%d,%d,%d,%d.",
        newStage.left,
        newStage.top,
        newStage.right,
        newStage.bottom);
}

void SplitViewerWindow::ExitFullScreen()
{
    if (!isFullScreen_)
    {
        return;
    }

    RECT oldStage = GetStageRect();
    SplitViewerDebugLogFormat(L"ExitFullScreen begin oldStage=%d,%d,%d,%d.",
        oldStage.left,
        oldStage.top,
        oldStage.right,
        oldStage.bottom);
    isFullScreen_ = false;
    SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
    SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
    SetWindowPlacement(hwnd_, &savedPlacement_);
    LayoutToolbar();
    SetWindowPos(hwnd_, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    UpdateForCurrentStage(true);
    InvalidateRect(hwnd_, NULL, FALSE);
    RECT newStage = GetStageRect();
    SplitViewerDebugLogFormat(L"ExitFullScreen success newStage=%d,%d,%d,%d.",
        newStage.left,
        newStage.top,
        newStage.right,
        newStage.bottom);
}

void SplitViewerWindow::ShowZoomTip(int layerIndex, const RECT& ownerRect, double scale, double fitScale)
{
    const double percent = fitScale > 0.00001 ? scale / fitScale * 100.0 : 100.0;
    tipText_ = CStringManager::Format(L"%.0f%%", percent);
    tipLayerIndex_ = layerIndex;
    tipOwnerRect_ = ownerRect;
    tipTick_ = GetTickCount();
    SetTimer(hwnd_, SPLITVIEWER_TIMER_TIP, 1000, NULL);
}

void SplitViewerWindow::OnTipTimer()
{
    if (!tipText_.empty() && GetTickCount() - tipTick_ >= 1000)
    {
        RECT client = { 0 };
        GetClientRect(hwnd_, &client);
        RECT invalidRect = zoomFinishInvalidRect_;
        RECT tipRect = GetTipRect(client);
        if (SplitViewerIsRectUsable(tipRect))
        {
            if (SplitViewerIsRectUsable(invalidRect))
            {
                UnionRect(&invalidRect, &invalidRect, &tipRect);
            }
            else
            {
                invalidRect = tipRect;
            }
        }

        tipText_.clear();
        KillTimer(hwnd_, SPLITVIEWER_TIMER_TIP);
        ZeroMemory(&zoomFinishInvalidRect_, sizeof(zoomFinishInvalidRect_));
        InvalidateStageRect(invalidRect);
    }
}

HWND SplitViewerWindow::hwnd() const
{
    return hwnd_;
}

bool SplitViewerWindow::FindEmbeddedLeafAtScreen(POINT screenPt, SplitViewerLeafHit& hit) const
{
    HWND hitWindow = WindowFromPoint(screenPt);
    if (!hitWindow || hitWindow == hwnd_ || !IsChild(hwnd_, hitWindow))
    {
        return false;
    }

    POINT clientPt = screenPt;
    ScreenToClient(hwnd_, &clientPt);

    if (!FindTopLeaf(clientPt, hit) || !hit.leaf || !hit.leaf->view.HasEmbeddedWindow())
    {
        return false;
    }

    return true;
}

bool SplitViewerWindow::EmbeddedDragMovedEnough(POINT screenPt) const
{
    return std::abs(screenPt.x - embeddedDragStartPoint_.x) >= kSplitViewerExternalDropMoveThreshold ||
        std::abs(screenPt.y - embeddedDragStartPoint_.y) >= kSplitViewerExternalDropMoveThreshold;
}

bool SplitViewerWindow::IsOutsideStageAtScreen(POINT screenPt) const
{
    POINT clientPt = screenPt;
    ScreenToClient(hwnd_, &clientPt);
    RECT stage = GetStageRect();
    return !SplitViewerPointInRect(stage, clientPt);
}

RECT SplitViewerWindow::MakeDetachedWindowRect(HWND window, POINT screenPt) const
{
    RECT current = { 0 };
    if (!window || !IsWindow(window) || !GetWindowRect(window, &current))
    {
        current = SplitViewerMakeRect(screenPt.x, screenPt.y, screenPt.x + 640, screenPt.y + 360);
    }

    const int width = std::max(160, SplitViewerRectWidth(current));
    const int height = std::max(120, SplitViewerRectHeight(current));
    const int captionOffset = std::min(height / 2, std::max(24, GetSystemMetrics(SM_CYCAPTION)));
    RECT rc = SplitViewerMakeRect(screenPt.x - width / 2,
        screenPt.y - captionOffset,
        screenPt.x - width / 2 + width,
        screenPt.y - captionOffset + height);

    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (GetMonitorInfoW(MonitorFromPoint(screenPt, MONITOR_DEFAULTTONEAREST), &monitorInfo))
    {
        ConstrainRectToBounds(rc, monitorInfo.rcWork);
    }

    return rc;
}

bool SplitViewerWindow::GetLeafScreenRect(SplitViewerNode* leaf, int layerIndex, RECT& screenRect) const
{
    if (!leaf || !hwnd_)
    {
        return false;
    }

    RECT stage = GetStageRect();
    RECT rootRect = ContentRect(stage);
    SplitViewerNode* root = baseRoot_;
    if (layerIndex >= 0)
    {
        if (layerIndex >= static_cast<int>(layers_.size()))
        {
            return false;
        }
        RECT layerRect = GetLayerRect(layerIndex, stage);
        rootRect = ContentRect(layerRect);
        root = layers_[layerIndex]->root;
    }

    RECT leafRect = { 0 };
    if (!FindNodeRect(root, rootRect, leaf, leafRect))
    {
        return false;
    }

    POINT topLeft = { leafRect.left, leafRect.top };
    POINT bottomRight = { leafRect.right, leafRect.bottom };
    ClientToScreen(hwnd_, &topLeft);
    ClientToScreen(hwnd_, &bottomRight);
    screenRect = SplitViewerMakeRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
    return SplitViewerIsRectUsable(screenRect);
}

HWND SplitViewerWindow::ExternalTopLevelWindowFromPoint(POINT screenPt) const
{
    HWND hitWindow = WindowFromPoint(screenPt);
    if (!hitWindow)
    {
        return NULL;
    }

    HWND root = GetAncestor(hitWindow, GA_ROOT);
    return root ? root : hitWindow;
}

bool SplitViewerWindow::IsUsableExternalWindow(HWND window) const
{
    if (!window || !IsWindow(window) || !IsWindowVisible(window) || IsIconic(window))
    {
        return false;
    }
    if (window == hwnd_ || window == GetDesktopWindow() || window == GetShellWindow())
    {
        return false;
    }
    if (IsChild(hwnd_, window) || GetAncestor(window, GA_ROOT) == hwnd_)
    {
        return false;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == GetCurrentProcessId())
    {
        return false;
    }

    const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if (style & WS_CHILD)
    {
        return false;
    }

    return true;
}

bool SplitViewerWindow::ExternalDragMovedEnough(POINT screenPt) const
{
    const int dx = std::abs(screenPt.x - externalDragStartPoint_.x);
    const int dy = std::abs(screenPt.y - externalDragStartPoint_.y);
    if (dx >= kSplitViewerExternalDropMoveThreshold || dy >= kSplitViewerExternalDropMoveThreshold)
    {
        return true;
    }

    RECT currentRect = { 0 };
    if (externalDragWindow_ && IsWindow(externalDragWindow_) && GetWindowRect(externalDragWindow_, &currentRect))
    {
        return std::abs(currentRect.left - externalDragStartRect_.left) >= kSplitViewerExternalDropMoveThreshold ||
            std::abs(currentRect.top - externalDragStartRect_.top) >= kSplitViewerExternalDropMoveThreshold;
    }

    return false;
}

bool SplitViewerWindow::FindExternalDropLeafAtScreen(POINT screenPt, SplitViewerLeafHit& hit) const
{
    POINT clientPt = screenPt;
    ScreenToClient(hwnd_, &clientPt);

    SplitViewerLeafHit found;
    if (!FindTopLeaf(clientPt, found))
    {
        return false;
    }
    if (!found.leaf || !found.leaf->IsLeaf() || found.leaf->view.HasContent())
    {
        return false;
    }

    hit = found;
    return true;
}

POINT SplitViewerWindow::MakePoint(LPARAM lparam) const
{
    POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
    return pt;
}

POINT SplitViewerWindow::MakeScreenPoint(LPARAM lparam) const
{
    POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
    return pt;
}

bool SplitViewerWindow::HasAnyContent() const
{
    if (!currentConfigPath_.empty() || NodeHasContent(baseRoot_) || !layers_.empty())
    {
        return true;
    }

    return false;
}

bool SplitViewerWindow::NodeHasContent(const SplitViewerNode* node) const
{
    if (!node)
    {
        return false;
    }
    if (node->IsLeaf())
    {
        return node->view.HasContent();
    }

    return NodeHasContent(node->first) || NodeHasContent(node->second);
}

RECT SplitViewerWindow::GetStageRect() const
{
    RECT client = { 0 };
    GetClientRect(hwnd_, &client);
    RECT available = client;
    if (!isFullScreen_)
    {
        available.left += kSplitViewerMargin;
        available.right -= kSplitViewerMargin;
        available.top += kSplitViewerToolbarHeight + kSplitViewerMargin;
        available.bottom -= kSplitViewerMargin;
    }

    if (SplitViewerRectWidth(available) <= 0 || SplitViewerRectHeight(available) <= 0)
    {
        return SplitViewerMakeRect(0, 0, 0, 0);
    }

    return FitRectToAspect(available, stageAspect_);
}

RECT SplitViewerWindow::FitRectToAspect(const RECT& bounds, double aspect) const
{
    const int boundsWidth = SplitViewerRectWidth(bounds);
    const int boundsHeight = SplitViewerRectHeight(bounds);
    int width = boundsWidth;
    int height = static_cast<int>(static_cast<double>(width) / aspect + 0.5);
    if (height > boundsHeight)
    {
        height = boundsHeight;
        width = static_cast<int>(static_cast<double>(height) * aspect + 0.5);
    }

    const int left = bounds.left + (boundsWidth - width) / 2;
    const int top = bounds.top + (boundsHeight - height) / 2;
    return SplitViewerMakeRect(left, top, left + width, top + height);
}

SIZE SplitViewerWindow::GetFullScreenStageSize() const
{
    MONITORINFO monitorInfo = { 0 };
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);

    RECT monitorRect = SplitViewerMakeRect(0,
        0,
        SplitViewerRectWidth(monitorInfo.rcMonitor),
        SplitViewerRectHeight(monitorInfo.rcMonitor));
    RECT fullStage = FitRectToAspect(monitorRect, stageAspect_);

    SIZE size = { std::max(1, SplitViewerRectWidth(fullStage)), std::max(1, SplitViewerRectHeight(fullStage)) };
    return size;
}

SIZE SplitViewerWindow::GetThumbnailStageSize(const RECT& sourceStage) const
{
    const int sourceWidth = std::max(1, SplitViewerRectWidth(sourceStage));
    const int sourceHeight = std::max(1, SplitViewerRectHeight(sourceStage));
    double scale = std::min(static_cast<double>(kSplitViewerConfigThumbnailMaxEdge) / static_cast<double>(sourceWidth),
        static_cast<double>(kSplitViewerConfigThumbnailMaxEdge) / static_cast<double>(sourceHeight));
    scale = std::min(1.0, std::max(0.01, scale));

    SIZE size = {
        std::max(1, static_cast<int>(sourceWidth * scale + 0.5)),
            std::max(1, static_cast<int>(sourceHeight * scale + 0.5))
    };
    return size;
}

bool SplitViewerWindow::RenderProjectedStageBitmap(const RECT& sourceStage, int width, int height, HBITMAP& bitmapHandle, bool includeEmbeddedWindows)
{
    const DWORD startTick = GetTickCount();
    bitmapHandle = NULL;
    if (!SplitViewerIsRectUsable(sourceStage) || width <= 0 || height <= 0)
    {
        SplitViewerDebugLogFormat(L"RenderProjectedStageBitmap rejected source=%d,%d,%d,%d size=%dx%d includeEmbedded=%d.",
            sourceStage.left,
            sourceStage.top,
            sourceStage.right,
            sourceStage.bottom,
            width,
            height,
            includeEmbeddedWindows ? 1 : 0);
        return false;
    }

    HDC screenDc = GetDC(hwnd_);
    HDC memDc = screenDc ? CreateCompatibleDC(screenDc) : NULL;
    bitmapHandle = screenDc ? CreateCompatibleBitmap(screenDc, width, height) : NULL;
    if (!screenDc || !memDc || !bitmapHandle)
    {
        SplitViewerDebugLogFormat(L"RenderProjectedStageBitmap resource failure source=%d,%d,%d,%d size=%dx%d screenDc=0x%p memDc=0x%p bitmap=0x%p elapsedMs=%u.",
            sourceStage.left,
            sourceStage.top,
            sourceStage.right,
            sourceStage.bottom,
            width,
            height,
            screenDc,
            memDc,
            bitmapHandle,
            static_cast<unsigned int>(GetTickCount() - startTick));
        if (bitmapHandle)
        {
            DeleteObject(bitmapHandle);
            bitmapHandle = NULL;
        }
        if (memDc)
        {
            DeleteDC(memDc);
        }
        if (screenDc)
        {
            ReleaseDC(hwnd_, screenDc);
        }
        return false;
    }

    HGDIOBJ oldBitmap = SelectObject(memDc, bitmapHandle);
    RECT targetStage = SplitViewerMakeRect(0, 0, width, height);
    DrawProjectedStage(memDc, sourceStage, targetStage, includeEmbeddedWindows);
    SelectObject(memDc, oldBitmap);

    DeleteDC(memDc);
    ReleaseDC(hwnd_, screenDc);
    SplitViewerDebugLogFormat(L"RenderProjectedStageBitmap success source=%d,%d,%d,%d size=%dx%d includeEmbedded=%d elapsedMs=%u.",
        sourceStage.left,
        sourceStage.top,
        sourceStage.right,
        sourceStage.bottom,
        width,
        height,
        includeEmbeddedWindows ? 1 : 0,
        static_cast<unsigned int>(GetTickCount() - startTick));
    return true;
}

RECT SplitViewerWindow::ContentRect(const RECT& rc) const
{
    RECT result = rc;
    if (borderVisible_ && SplitViewerRectWidth(result) > 2 && SplitViewerRectHeight(result) > 2)
    {
        InflateRect(&result, -1, -1);
    }
    return result;
}

RECT SplitViewerWindow::PlusButtonRect(const RECT& rc) const
{
    const int size = std::min(kSplitViewerPlusButtonSize, std::min(SplitViewerRectWidth(rc), SplitViewerRectHeight(rc)) - 8);
    if (size <= 8)
    {
        return SplitViewerMakeRect(0, 0, 0, 0);
    }

    const int left = rc.left + (SplitViewerRectWidth(rc) - size) / 2;
    const int top = rc.top + (SplitViewerRectHeight(rc) - size) / 2;
    return SplitViewerMakeRect(left, top, left + size, top + size);
}

RECT SplitViewerWindow::GetLayerRect(int index, const RECT& stage) const
{
    if (index < 0 || index >= static_cast<int>(layers_.size()))
    {
        return SplitViewerMakeRect(0, 0, 0, 0);
    }

    const SplitViewerFloatRect& nr = layers_[index]->rect;
    const int width = SplitViewerRectWidth(stage);
    const int height = SplitViewerRectHeight(stage);
    RECT rc = SplitViewerMakeRect(stage.left + static_cast<int>(nr.left * width + 0.5),
        stage.top + static_cast<int>(nr.top * height + 0.5),
        stage.left + static_cast<int>(nr.right * width + 0.5),
        stage.top + static_cast<int>(nr.bottom * height + 0.5));
    if (rc.right <= rc.left)
    {
        rc.right = rc.left + 1;
    }
    if (rc.bottom <= rc.top)
    {
        rc.bottom = rc.top + 1;
    }
    return rc;
}

void SplitViewerWindow::SplitNodeRects(const RECT& rc, SplitViewerNode* node, RECT& firstRect, RECT& splitterRect, RECT& secondRect) const
{
    if (!node || node->IsLeaf())
    {
        firstRect = rc;
        splitterRect = SplitViewerMakeRect(0, 0, 0, 0);
        secondRect = SplitViewerMakeRect(0, 0, 0, 0);
        return;
    }

    const double ratio = SplitViewerClampDouble(node->ratio, 0.02, 0.98);
    const int splitterSize = SplitterSize();
    if (node->direction == SPLITVIEWER_SPLIT_HORIZONTAL)
    {
        if (SplitViewerRectHeight(rc) <= splitterSize + 2)
        {
            const int middle = rc.top + SplitViewerRectHeight(rc) / 2;
            firstRect = SplitViewerMakeRect(rc.left, rc.top, rc.right, middle);
            splitterRect = SplitViewerMakeRect(rc.left, middle, rc.right, std::min<int>(static_cast<int>(rc.bottom), middle + splitterSize));
            secondRect = SplitViewerMakeRect(rc.left, splitterRect.bottom, rc.right, rc.bottom);
            return;
        }

        const int available = SplitViewerRectHeight(rc) - splitterSize;
        int firstHeight = static_cast<int>(available * ratio + 0.5);
        firstHeight = SplitViewerClampInt(firstHeight, 1, available - 1);
        firstRect = SplitViewerMakeRect(rc.left, rc.top, rc.right, rc.top + firstHeight);
        splitterRect = SplitViewerMakeRect(rc.left, firstRect.bottom, rc.right, firstRect.bottom + splitterSize);
        secondRect = SplitViewerMakeRect(rc.left, splitterRect.bottom, rc.right, rc.bottom);
    }
    else
    {
        if (SplitViewerRectWidth(rc) <= splitterSize + 2)
        {
            const int middle = rc.left + SplitViewerRectWidth(rc) / 2;
            firstRect = SplitViewerMakeRect(rc.left, rc.top, middle, rc.bottom);
            splitterRect = SplitViewerMakeRect(middle, rc.top, std::min<int>(static_cast<int>(rc.right), middle + splitterSize), rc.bottom);
            secondRect = SplitViewerMakeRect(splitterRect.right, rc.top, rc.right, rc.bottom);
            return;
        }

        const int available = SplitViewerRectWidth(rc) - splitterSize;
        int firstWidth = static_cast<int>(available * ratio + 0.5);
        firstWidth = SplitViewerClampInt(firstWidth, 1, available - 1);
        firstRect = SplitViewerMakeRect(rc.left, rc.top, rc.left + firstWidth, rc.bottom);
        splitterRect = SplitViewerMakeRect(firstRect.right, rc.top, firstRect.right + splitterSize, rc.bottom);
        secondRect = SplitViewerMakeRect(splitterRect.right, rc.top, rc.right, rc.bottom);
    }
}

int SplitViewerWindow::SplitterSize() const
{
    return borderVisible_ ? kSplitViewerSplitterSize : 0;
}

RECT SplitViewerWindow::SplitterDrawRect(const RECT& splitterRect, SplitViewerSplitDirection direction, int thickness) const
{
    RECT result = splitterRect;
    if (direction == SPLITVIEWER_SPLIT_HORIZONTAL)
    {
        const int center = (splitterRect.top + splitterRect.bottom) / 2;
        result.top = center - thickness / 2;
        result.bottom = result.top + thickness;
    }
    else
    {
        const int center = (splitterRect.left + splitterRect.right) / 2;
        result.left = center - thickness / 2;
        result.right = result.left + thickness;
    }
    return result;
}

bool SplitViewerWindow::GetSplitterVisualRect(SplitViewerNode* root, int layerIndex, SplitViewerNode* splitNode, bool hover, RECT& visualRect) const
{
    ZeroMemory(&visualRect, sizeof(visualRect));
    if (!borderVisible_ || !root || !splitNode || splitNode->IsLeaf())
    {
        return false;
    }

    RECT stage = GetStageRect();
    RECT ownerRect = stage;
    if (layerIndex >= 0)
    {
        if (layerIndex >= static_cast<int>(layers_.size()))
        {
            return false;
        }
        ownerRect = GetLayerRect(layerIndex, stage);
    }

    RECT nodeRect = { 0 };
    if (!FindNodeRect(root, ContentRect(ownerRect), splitNode, nodeRect))
    {
        return false;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(nodeRect, splitNode, firstRect, splitterRect, secondRect);
    visualRect = SplitterDrawRect(splitterRect,
        splitNode->direction,
        hover ? kSplitViewerSplitterHoverSize : SplitterSize());
    visualRect = SplitViewerInflatedRect(visualRect, 2, 2);
    return SplitViewerIsRectUsable(visualRect);
}

double SplitViewerWindow::FitImageScale(const SplitViewerImageView& view, const RECT& rc) const
{
    if (!view.HasImage() || SplitViewerRectWidth(rc) <= 0 || SplitViewerRectHeight(rc) <= 0)
    {
        return 1.0;
    }

    const double sx = static_cast<double>(SplitViewerRectWidth(rc)) / static_cast<double>(view.image->GetWidth());
    const double sy = static_cast<double>(SplitViewerRectHeight(rc)) / static_cast<double>(view.image->GetHeight());
    const double scale = std::min(sx, sy);
    return scale > 0.00001 ? scale : 1.0;
}

SplitViewerNode* SplitViewerWindow::FirstLeaf(SplitViewerNode* node) const
{
    if (!node)
    {
        return NULL;
    }
    if (node->IsLeaf())
    {
        return node;
    }
    return FirstLeaf(node->first);
}

bool SplitViewerWindow::FindLeafAt(SplitViewerNode* node, const RECT& rc, POINT pt, SplitViewerNode*& leaf, RECT& leafRect) const
{
    if (!node || !SplitViewerIsRectUsable(rc) || !SplitViewerPointInRect(rc, pt))
    {
        return false;
    }
    if (node->IsLeaf())
    {
        leaf = node;
        leafRect = rc;
        return true;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);
    if (FindLeafAt(node->first, firstRect, pt, leaf, leafRect))
    {
        return true;
    }
    if (FindLeafAt(node->second, secondRect, pt, leaf, leafRect))
    {
        return true;
    }
    return false;
}

bool SplitViewerWindow::FindSplitterAt(SplitViewerNode* node, const RECT& rc, POINT pt, SplitViewerNode*& splitNode, RECT& nodeRect) const
{
    if (!node || node->IsLeaf() || !SplitViewerIsRectUsable(rc))
    {
        return false;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);

    RECT hitRect = SplitViewerInflatedRect(splitterRect, kSplitViewerSplitterHitPadding, kSplitViewerSplitterHitPadding);
    if (SplitViewerPointInRect(hitRect, pt))
    {
        splitNode = node;
        nodeRect = rc;
        return true;
    }
    if (FindSplitterAt(node->first, firstRect, pt, splitNode, nodeRect))
    {
        return true;
    }
    if (FindSplitterAt(node->second, secondRect, pt, splitNode, nodeRect))
    {
        return true;
    }
    return false;
}

bool SplitViewerWindow::FindNodeRect(SplitViewerNode* current, const RECT& rc, SplitViewerNode* target, RECT& targetRect) const
{
    if (!current || !target || !SplitViewerIsRectUsable(rc))
    {
        return false;
    }
    if (current == target)
    {
        targetRect = rc;
        return true;
    }
    if (current->IsLeaf())
    {
        return false;
    }

    RECT firstRect = { 0 };
    RECT splitterRect = { 0 };
    RECT secondRect = { 0 };
    SplitNodeRects(rc, current, firstRect, splitterRect, secondRect);
    if (FindNodeRect(current->first, firstRect, target, targetRect))
    {
        return true;
    }
    return FindNodeRect(current->second, secondRect, target, targetRect);
}

bool SplitViewerWindow::CanDeleteLeafFromSplit(SplitViewerNode* root, SplitViewerNode* target) const
{
    if (!root || !target || root->IsLeaf())
    {
        return false;
    }
    if (root->first == target || root->second == target)
    {
        return true;
    }
    return CanDeleteLeafFromSplit(root->first, target) ||
        CanDeleteLeafFromSplit(root->second, target);
}

bool SplitViewerWindow::FindTopLayer(POINT pt, int& layerIndex, RECT& layerRect) const
{
    RECT stage = GetStageRect();
    for (int i = static_cast<int>(layers_.size()) - 1; i >= 0; --i)
    {
        RECT rc = GetLayerRect(i, stage);
        if (SplitViewerPointInRect(rc, pt))
        {
            layerIndex = i;
            layerRect = rc;
            return true;
        }
    }

    layerIndex = -1;
    layerRect = SplitViewerMakeRect(0, 0, 0, 0);
    return false;
}

bool SplitViewerWindow::FindTopLeaf(POINT pt, SplitViewerLeafHit& hit) const
{
    RECT stage = GetStageRect();
    for (int i = static_cast<int>(layers_.size()) - 1; i >= 0; --i)
    {
        RECT rc = GetLayerRect(i, stage);
        if (SplitViewerPointInRect(rc, pt))
        {
            RECT rootRect = ContentRect(rc);
            SplitViewerNode* leaf = NULL;
            RECT leafRect = { 0 };
            if (FindLeafAt(layers_[i]->root, rootRect, pt, leaf, leafRect))
            {
                hit.layerIndex = i;
                hit.root = layers_[i]->root;
                hit.leaf = leaf;
                hit.ownerRect = rc;
                hit.rootRect = rootRect;
                hit.leafRect = leafRect;
                return true;
            }
            return false;
        }
    }

    if (!SplitViewerPointInRect(stage, pt))
    {
        return false;
    }

    RECT rootRect = ContentRect(stage);
    SplitViewerNode* leaf = NULL;
    RECT leafRect = { 0 };
    if (FindLeafAt(baseRoot_, rootRect, pt, leaf, leafRect))
    {
        hit.layerIndex = -1;
        hit.root = baseRoot_;
        hit.leaf = leaf;
        hit.ownerRect = stage;
        hit.rootRect = rootRect;
        hit.leafRect = leafRect;
        return true;
    }

    return false;
}

bool SplitViewerWindow::FindTopSplitter(POINT pt, SplitViewerSplitterHit& hit) const
{
    RECT stage = GetStageRect();
    int layerIndex = -1;
    RECT layerRect = { 0 };
    if (FindTopLayer(pt, layerIndex, layerRect))
    {
        RECT rootRect = ContentRect(layerRect);
        SplitViewerNode* splitNode = NULL;
        RECT nodeRect = { 0 };
        if (FindSplitterAt(layers_[layerIndex]->root, rootRect, pt, splitNode, nodeRect))
        {
            hit.layerIndex = layerIndex;
            hit.root = layers_[layerIndex]->root;
            hit.splitNode = splitNode;
            hit.direction = splitNode->direction;
            hit.ownerRect = layerRect;
            hit.rootRect = rootRect;
            hit.nodeRect = nodeRect;
            return true;
        }
        return false;
    }

    if (!SplitViewerPointInRect(stage, pt))
    {
        return false;
    }

    RECT rootRect = ContentRect(stage);
    SplitViewerNode* splitNode = NULL;
    RECT nodeRect = { 0 };
    if (FindSplitterAt(baseRoot_, rootRect, pt, splitNode, nodeRect))
    {
        hit.layerIndex = -1;
        hit.root = baseRoot_;
        hit.splitNode = splitNode;
        hit.direction = splitNode->direction;
        hit.ownerRect = stage;
        hit.rootRect = rootRect;
        hit.nodeRect = nodeRect;
        return true;
    }

    return false;
}

int SplitViewerWindow::HitLayerResize(const RECT& layerRect, POINT pt) const
{
    if (!SplitViewerPointInRect(layerRect, pt))
    {
        return SPLITVIEWER_RESIZE_NONE;
    }

    int hit = SPLITVIEWER_RESIZE_NONE;
    if (pt.x - layerRect.left <= kSplitViewerLayerHitSize)
    {
        hit |= SPLITVIEWER_RESIZE_LEFT;
    }
    if (layerRect.right - pt.x <= kSplitViewerLayerHitSize)
    {
        hit |= SPLITVIEWER_RESIZE_RIGHT;
    }
    if (pt.y - layerRect.top <= kSplitViewerLayerHitSize)
    {
        hit |= SPLITVIEWER_RESIZE_TOP;
    }
    if (layerRect.bottom - pt.y <= kSplitViewerLayerHitSize)
    {
        hit |= SPLITVIEWER_RESIZE_BOTTOM;
    }
    return hit;
}

void SplitViewerWindow::NormalizeLayerResizeRect(RECT& rc, const RECT& bounds) const
{
    if (SplitViewerRectWidth(rc) < kSplitViewerMinLayerWidth)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_LEFT)
        {
            rc.left = rc.right - kSplitViewerMinLayerWidth;
        }
        else
        {
            rc.right = rc.left + kSplitViewerMinLayerWidth;
        }
    }

    if (SplitViewerRectHeight(rc) < kSplitViewerMinLayerHeight)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_TOP)
        {
            rc.top = rc.bottom - kSplitViewerMinLayerHeight;
        }
        else
        {
            rc.bottom = rc.top + kSplitViewerMinLayerHeight;
        }
    }

    if (rc.left < bounds.left)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_LEFT)
        {
            rc.left = bounds.left;
        }
        else
        {
            OffsetRect(&rc, bounds.left - rc.left, 0);
        }
    }
    if (rc.top < bounds.top)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_TOP)
        {
            rc.top = bounds.top;
        }
        else
        {
            OffsetRect(&rc, 0, bounds.top - rc.top);
        }
    }
    if (rc.right > bounds.right)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_RIGHT)
        {
            rc.right = bounds.right;
        }
        else
        {
            OffsetRect(&rc, bounds.right - rc.right, 0);
        }
    }
    if (rc.bottom > bounds.bottom)
    {
        if (dragResizeHit_ & SPLITVIEWER_RESIZE_BOTTOM)
        {
            rc.bottom = bounds.bottom;
        }
        else
        {
            OffsetRect(&rc, 0, bounds.bottom - rc.bottom);
        }
    }

    if (SplitViewerRectWidth(rc) < kSplitViewerMinLayerWidth)
    {
        rc.right = std::min(bounds.right, rc.left + kSplitViewerMinLayerWidth);
        rc.left = std::max(bounds.left, rc.right - kSplitViewerMinLayerWidth);
    }
    if (SplitViewerRectHeight(rc) < kSplitViewerMinLayerHeight)
    {
        rc.bottom = std::min(bounds.bottom, rc.top + kSplitViewerMinLayerHeight);
        rc.top = std::max(bounds.top, rc.bottom - kSplitViewerMinLayerHeight);
    }

    ConstrainRectToBounds(rc, bounds);
}

void SplitViewerWindow::ConstrainRectToBounds(RECT& rc, const RECT& bounds) const
{
    const int width = SplitViewerRectWidth(rc);
    const int height = SplitViewerRectHeight(rc);
    if (width > SplitViewerRectWidth(bounds))
    {
        rc.left = bounds.left;
        rc.right = bounds.right;
    }
    else
    {
        if (rc.left < bounds.left)
        {
            OffsetRect(&rc, bounds.left - rc.left, 0);
        }
        if (rc.right > bounds.right)
        {
            OffsetRect(&rc, bounds.right - rc.right, 0);
        }
    }

    if (height > SplitViewerRectHeight(bounds))
    {
        rc.top = bounds.top;
        rc.bottom = bounds.bottom;
    }
    else
    {
        if (rc.top < bounds.top)
        {
            OffsetRect(&rc, 0, bounds.top - rc.top);
        }
        if (rc.bottom > bounds.bottom)
        {
            OffsetRect(&rc, 0, bounds.bottom - rc.bottom);
        }
    }
}

SplitViewerWindow* SplitViewerWindow::hookOwner_ = NULL;