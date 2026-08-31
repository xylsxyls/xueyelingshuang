#include "PlayerWindow.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"

namespace LoopPlayer
{
    PlayerWindow::PlayerWindow()
        : hinst_(NULL),
          hwnd_(NULL),
          videoHost_(NULL),
          videoPanel_(NULL),
          openButton_(NULL),
          playButton_(NULL),
          pauseButton_(NULL),
          stopButton_(NULL),
          setAButton_(NULL),
          setBButton_(NULL),
          aPrevFrameButton_(NULL),
          aNextFrameButton_(NULL),
          bPrevFrameButton_(NULL),
          bNextFrameButton_(NULL),
          clearButton_(NULL),
          loopCheck_(NULL),
          fullScreenButton_(NULL),
          seekSlider_(NULL),
          timeText_(NULL),
          abText_(NULL),
          overlayPanel_(NULL),
          topOverlayPanel_(NULL),
          zoomTipPanel_(NULL),
          player_(NULL),
          hasMedia_(false),
          isPlaying_(false),
          suppressReplay_(false),
          segmentStopApplied_(false),
          loopReplayPending_(false),
          loopReplayFastAttempt_(false),
          markerSeekPending_(false),
          manualPauseRequest_(false),
          draggingSeek_(false),
          progressMenuActive_(false),
          wasPlayingBeforeDrag_(false),
          restoreSegmentAfterDrag_(false),
          isFullScreen_(false),
          overlayTargetVisible_(false),
          topOverlayTargetVisible_(false),
          openFileDialogActive_(false),
          emptyPlusHovered_(false),
          topOverlayDragActive_(false),
          movingOrSizing_(false),
          mouseInsideWindow_(false),
          haveLastMouseActivityScreen_(false),
          pauseIndicatorWasDrawn_(false),
          savedStyle_(0),
          savedExStyle_(0),
          lastLoopReplayTick_(0),
          markerSeekPendingTick_(0),
          topOverlayKeepVisibleUntil_(0),
          lastMouseActivityTick_(0),
          zoomTipHideTick_(0),
          loopReplayCount_(0),
          topOverlayDragLastApplyTick_(0),
          playbackRateTenths_(10),
          nativeVideoWidth_(0),
          nativeVideoHeight_(0),
          videoZoomPercent_(100),
          videoPanX_(0),
          videoPanY_(0),
          pauseIndicatorAlpha_(0),
          videoMouseDown_(false),
          draggingVideo_(false),
          uiPosition_(0),
          overlayVisiblePixels_(0),
          topOverlayVisiblePixels_(0),
          topHoveredButton_(TOP_BUTTON_NONE),
          hoveredMarker_(PROGRESS_MARKER_NONE),
          hoverMarkerWindow_(NULL),
          contextMenuPosition_(0),
          duration_(0),
          loopA_(-1),
          loopB_(-1),
          frameDuration_(DEFAULT_FRAME_DURATION)
    {
        ZeroMemory(&savedPlacement_, sizeof(savedPlacement_));
        savedPlacement_.length = sizeof(savedPlacement_);
        ZeroMemory(&lastMouseActivityScreen_, sizeof(lastMouseActivityScreen_));
        ZeroMemory(&topOverlayDragStartCursor_, sizeof(topOverlayDragStartCursor_));
        ZeroMemory(&topOverlayDragStartWindow_, sizeof(topOverlayDragStartWindow_));
        ZeroMemory(&videoDragStart_, sizeof(videoDragStart_));
        ZeroMemory(&videoDragLast_, sizeof(videoDragLast_));
    }

    bool PlayerWindow::Create(HINSTANCE hinst, int cmdShow)
    {
        hinst_ = hinst;

        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = PlayerWindow::StaticWndProc;
        wc.hInstance = hinst_;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = kWindowClass;
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            Logf(L"RegisterClassEx failed: lastError=%lu", GetLastError());
            return false;
        }

        RECT workArea = { 0 };
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
        const int workW = max(640, workArea.right - workArea.left);
        const int workH = max(360, workArea.bottom - workArea.top);
        const int initialW = max(320, workW / 2);
        const int initialH = max(240, workH / 2);
        const int initialX = workArea.left + (workW - initialW) / 2;
        const int initialY = workArea.top + (workH - initialH) / 2;

        hwnd_ = CreateWindowExW(
            WS_EX_APPWINDOW,
            kWindowClass,
            kAppTitle,
            WS_POPUP | WS_CLIPCHILDREN | WS_MINIMIZEBOX,
            initialX,
            initialY,
            initialW,
            initialH,
            NULL,
            NULL,
            hinst_,
            this);

        if (!hwnd_)
        {
            Logf(L"CreateWindowEx failed: lastError=%lu", GetLastError());
            return false;
        }

        ShowWindow(hwnd_, cmdShow);
        UpdateWindow(hwnd_);
        return true;
    }

    HWND PlayerWindow::hwnd() const
    {
        return hwnd_;
    }

    bool PlayerWindow::LoadFile(const wchar_t* path)
    {
        if (!path || !path[0])
        {
            Logf(L"LoadFile ignored: empty path");
            return false;
        }

        Logf(L"LoadFile begin with Media Foundation: %s", path);
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = { 0 };
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &fileInfo))
        {
            ULONGLONG fileSize = (static_cast<ULONGLONG>(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow;
            Logf(L"Input file size: %I64u bytes", fileSize);
        }
        else
        {
            Logf(L"GetFileAttributesEx failed: lastError=%lu", GetLastError());
        }

        ClosePlayer();

        callback_.SetWindow(hwnd_);
        HRESULT hr = MFPCreateMediaPlayer(NULL, FALSE, MFP_OPTION_NONE, &callback_, videoPanel_, &player_);
        if (FAILED(hr))
        {
            return ReportFailure(L"MFPCreateMediaPlayer failed", hr);
        }
        Logf(L"MFPCreateMediaPlayer succeeded");

        hr = player_->SetAspectRatioMode(MFVideoARMode_PreservePicture);
        Logf(L"SetAspectRatioMode(PreservePicture) returned 0x%08X", static_cast<unsigned int>(hr));

        IMFPMediaItem* item = NULL;
        hr = player_->CreateMediaItemFromURL(path, TRUE, 0, &item);
        if (FAILED(hr))
        {
            SafeRelease(item);
            ClosePlayer();
            return ReportFailure(L"CreateMediaItemFromURL failed", hr);
        }
        Logf(L"CreateMediaItemFromURL succeeded");

        hr = player_->SetMediaItem(item);
        if (FAILED(hr))
        {
            SafeRelease(item);
            ClosePlayer();
            return ReportFailure(L"SetMediaItem failed", hr);
        }
        Logf(L"SetMediaItem succeeded");

        duration_ = 0;
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        ResetVideoTransform();
        ReadDuration();
        DetectFrameDuration(item);
        ReadNativeVideoSize();
        SafeRelease(item);

        loopA_ = -1;
        loopB_ = -1;
        hasMedia_ = true;
        isPlaying_ = false;
        emptyPlusHovered_ = false;
        pauseIndicatorAlpha_ = 0;
        pauseIndicatorWasDrawn_ = false;
        HideZoomTip();
        suppressReplay_ = false;
        segmentStopApplied_ = false;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        manualPauseRequest_ = false;
        markerSeekPendingTick_ = 0;
        loopReplayCount_ = 0;
        playbackRateTenths_ = 10;
        filePath_ = path;

        uiPosition_ = 0;
        InvalidateProgressViews();
        InvalidateRect(videoHost_, NULL, TRUE);
        InvalidateRect(videoPanel_, NULL, TRUE);

        UpdateTitle();
        UpdateLoopText();
        UpdateTimeText(0);
        UpdateControls();
        ResizeWindowToVideoDefault();
        ResizeVideoWindow();

        hr = player_->Play();
        if (SUCCEEDED(hr))
        {
            isPlaying_ = true;
            ApplyPlaybackRate(false);
            SetStatus(L"Playing");
            Logf(L"Initial Play succeeded: 0x%08X", static_cast<unsigned int>(hr));
        }
        else
        {
            isPlaying_ = false;
            ReportFailure(L"File opened, but playback failed", hr);
        }

        UpdateControls();
        Logf(L"LoadFile end: hr=0x%08X", static_cast<unsigned int>(hr));
        return SUCCEEDED(hr);
    }

    LRESULT CALLBACK PlayerWindow::StaticProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->ProgressWndProc(hwnd, msg, wparam, lparam, false);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK PlayerWindow::StaticOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->ProgressWndProc(hwnd, msg, wparam, lparam, true);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK PlayerWindow::StaticTopOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->TopOverlayWndProc(hwnd, msg, wparam, lparam);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK PlayerWindow::StaticZoomTipWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->ZoomTipWndProc(hwnd, msg, wparam, lparam);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK PlayerWindow::StaticVideoWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->VideoWndProc(hwnd, msg, wparam, lparam);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK PlayerWindow::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = NULL;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = reinterpret_cast<PlayerWindow*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->hwnd_ = hwnd;
        }
        else
        {
            self = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (self)
        {
            return self->WndProc(msg, wparam, lparam);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT PlayerWindow::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_CREATE:
            OnCreate();
            return 0;
        case WM_COMMAND:
            OnCommand(LOWORD(wparam), HIWORD(wparam));
            return 0;
        case WM_SIZE:
            LayoutControls();
            ResizeVideoWindow();
            return 0;
        case WM_ENTERSIZEMOVE:
            movingOrSizing_ = true;
            topOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
            return 0;
        case WM_EXITSIZEMOVE:
            movingOrSizing_ = false;
            topOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
            LayoutControls();
            ResizeVideoWindow();
            UpdateOverlayState();
            UpdateTopOverlayState();
            UpdatePauseIndicatorState();
            UpdateZoomTipState();
            return 0;
        case WM_MOUSEMOVE:
            TouchMouseActivity();
            return 0;
        case WM_LBUTTONDBLCLK:
            TouchMouseActivity(true);
            ToggleFullScreen();
            return 0;
        case WM_GETMINMAXINFO:
            OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lparam));
            return 0;
        case WM_NCHITTEST:
            return OnNcHitTest(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        case WM_DROPFILES:
            OnDropFiles(reinterpret_cast<HDROP>(wparam));
            return 0;
        case WM_TIMER:
            if (wparam == TIMER_POSITION)
            {
                if (movingOrSizing_)
                {
                    if (IsActiveAbLoop() || loopReplayPending_)
                    {
                        OnTimer(false);
                    }
                    return 0;
                }

                OnTimer();
                UpdateOverlayState();
                UpdateTopOverlayState();
                UpdatePauseIndicatorState();
                UpdateZoomTipState();
                return 0;
            }
            if (wparam == TIMER_VIDEO_CLICK)
            {
                KillTimer(hwnd_, TIMER_VIDEO_CLICK);
                TogglePlayback();
                return 0;
            }
            if (wparam == TIMER_TITLE_DRAG)
            {
                UpdateTopOverlayDrag(false);
                return 0;
            }
            break;
        case WM_PLAYER_EVENT:
            OnPlayerEvent(static_cast<MFP_EVENT_TYPE>(wparam), static_cast<HRESULT>(lparam));
            return 0;
        case WM_KEYDOWN:
            OnKeyDown(wparam);
            return 0;
        case WM_SYSKEYDOWN:
            if (wparam == VK_RETURN)
            {
                ToggleFullScreen();
                return 0;
            }
            break;
        case WM_DESTROY:
            KillTimer(hwnd_, TIMER_POSITION);
            KillTimer(hwnd_, TIMER_VIDEO_CLICK);
            KillTimer(hwnd_, TIMER_TITLE_DRAG);
            callback_.SetWindow(NULL);
            ClosePlayer();
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }

        return DefWindowProcW(hwnd_, msg, wparam, lparam);
    }

    void PlayerWindow::RegisterChildClasses()
    {
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticProgressWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = kProgressClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticOverlayWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kOverlayClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticTopOverlayWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kTopOverlayClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticZoomTipWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kZoomTipClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = PlayerWindow::StaticVideoWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kVideoClass;
        RegisterClassExW(&wc);
    }

    void PlayerWindow::OnCreate()
    {
        InitCommonControls();
        RegisterChildClasses();
        Logf(L"Window created. Log path: %s", GetLogFilePath().c_str());

        openButton_ = CreateButton(L"加载", IDC_OPEN);
        playButton_ = CreateButton(L"播放", IDC_PLAY);
        pauseButton_ = CreateButton(L"暂停", IDC_PAUSE);
        stopButton_ = CreateButton(L"停止", IDC_STOP);
        setAButton_ = CreateButton(L"设置A", IDC_SET_A);
        setBButton_ = CreateButton(L"设置B", IDC_SET_B);
        aPrevFrameButton_ = CreateButton(L"A -1F", IDC_A_PREV_FRAME);
        aNextFrameButton_ = CreateButton(L"A +1F", IDC_A_NEXT_FRAME);
        bPrevFrameButton_ = CreateButton(L"B -1F", IDC_B_PREV_FRAME);
        bNextFrameButton_ = CreateButton(L"B +1F", IDC_B_NEXT_FRAME);
        clearButton_ = CreateButton(L"清除AB", IDC_CLEAR_AB);
        loopCheck_ = CreateWindowExW(0, L"BUTTON", L"AB循环", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                     0, 0, 90, 24, hwnd_, reinterpret_cast<HMENU>(IDC_LOOP_AB), hinst_, NULL);
        SendMessageW(loopCheck_, BM_SETCHECK, BST_CHECKED, 0);
        fullScreenButton_ = CreateButton(L"全屏", IDC_FULLSCREEN);

        seekSlider_ = CreateWindowExW(0, kProgressClass, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      0, 0, 100, NORMAL_PROGRESS_HEIGHT, hwnd_, reinterpret_cast<HMENU>(IDC_SEEK), hinst_, this);

        timeText_ = CreateWindowExW(0, L"STATIC", L"--:--.--- / --:--.---", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                    0, 0, 220, 20, hwnd_, reinterpret_cast<HMENU>(IDC_TIME_TEXT), hinst_, NULL);
        abText_ = CreateWindowExW(0, L"STATIC", L"A: --:--.---   B: --:--.---", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  0, 0, 320, 20, hwnd_, reinterpret_cast<HMENU>(IDC_AB_TEXT), hinst_, NULL);
        videoHost_ = CreateWindowExW(0, kVideoClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                     0, 0, 100, 100, hwnd_, reinterpret_cast<HMENU>(IDC_VIDEO_HOST), hinst_, this);
        videoPanel_ = CreateWindowExW(0, kVideoClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                      0, 0, 100, 100, videoHost_, reinterpret_cast<HMENU>(IDC_VIDEO_PANEL), hinst_, this);
        overlayPanel_ = CreateWindowExW(0,
                                        kOverlayClass,
                                        L"",
                                        WS_CHILD | WS_CLIPSIBLINGS,
                                        0, 0, 100, FULLSCREEN_OVERLAY_HEIGHT, hwnd_,
                                        reinterpret_cast<HMENU>(IDC_FULLSCREEN_OVERLAY),
                                        hinst_,
                                        this);
        ShowWindow(overlayPanel_, SW_HIDE);
        topOverlayPanel_ = CreateWindowExW(0,
                                           kTopOverlayClass,
                                           L"",
                                           WS_CHILD | WS_CLIPSIBLINGS,
                                           0, 0, 100, TOP_OVERLAY_HEIGHT, hwnd_,
                                           NULL,
                                           hinst_,
                                           this);
        ShowWindow(topOverlayPanel_, SW_HIDE);

        zoomTipPanel_ = CreateWindowExW(0,
                                        kZoomTipClass,
                                        L"",
                                        WS_CHILD | WS_CLIPSIBLINGS,
                                        0, 0, ZOOM_TIP_WIDTH, ZOOM_TIP_HEIGHT, hwnd_,
                                        reinterpret_cast<HMENU>(IDC_ZOOM_TIP),
                                        hinst_,
                                        this);
        ShowWindow(zoomTipPanel_, SW_HIDE);

        DragAcceptFiles(hwnd_, TRUE);
        SetTimer(hwnd_, TIMER_POSITION, 10, NULL);

        LayoutControls();
        UpdateControls();
        SetStatus(L"加载或拖入视频文件");
    }

    HWND PlayerWindow::CreateButton(const wchar_t* text, int id)
    {
        return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                               0, 0, 70, 26, hwnd_, reinterpret_cast<HMENU>(id), hinst_, NULL);
    }

    bool PlayerWindow::ReportFailure(const wchar_t* action, HRESULT hr)
    {
        std::wstring message = action ? action : L"操作失败";
        message += L": ";
        message += HResultText(hr);

        Logf(L"%s", message.c_str());
        SetStatus(message.c_str());

        if (hwnd_)
        {
            MessageBoxW(hwnd_, message.c_str(), kAppTitle, MB_ICONERROR | MB_OK);
        }

        return false;
    }

    void PlayerWindow::LayoutControls()
    {
        RECT rc = { 0 };
        GetClientRect(hwnd_, &rc);

        ShowControls(SW_HIDE);
        MoveWindow(videoHost_, 0, 0, max(1, rc.right), max(1, rc.bottom), TRUE);
        ApplyVideoTransform();
        LayoutOverlay();
        LayoutTopOverlay();
        LayoutZoomTip();
    }

    bool PlayerWindow::GetOverlayAnchorRect(RECT& screenRect) const
    {
        ZeroMemory(&screenRect, sizeof(screenRect));

        if (isFullScreen_)
        {
            return GetWindowRect(hwnd_, &screenRect) != FALSE;
        }

        if (!videoHost_)
        {
            return false;
        }

        return GetWindowRect(videoHost_, &screenRect) != FALSE;
    }

    void PlayerWindow::LayoutOverlay()
    {
        if (!overlayPanel_)
        {
            return;
        }

        const int visible = max(0, min(FULLSCREEN_OVERLAY_HEIGHT, overlayVisiblePixels_));
        RECT screenRect = { 0 };
        if (visible <= 0 || !GetOverlayAnchorRect(screenRect))
        {
            ShowWindow(overlayPanel_, SW_HIDE);
            return;
        }

        RECT client = { 0 };
        GetClientRect(hwnd_, &client);
        const int width = max(1, client.right - client.left);
        const int y = max(0, client.bottom - visible);
        SetWindowPos(overlayPanel_,
                     HWND_TOP,
                     0,
                     y,
                     width,
                     visible,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void PlayerWindow::LayoutTopOverlay()
    {
        if (!topOverlayPanel_)
        {
            return;
        }

        const int visible = max(0, min(TOP_OVERLAY_HEIGHT, topOverlayVisiblePixels_));
        if (visible <= 0)
        {
            ShowWindow(topOverlayPanel_, SW_HIDE);
            return;
        }

        RECT client = { 0 };
        GetClientRect(hwnd_, &client);
        const int width = max(1, client.right - client.left);
        SetWindowPos(topOverlayPanel_,
                     HWND_TOP,
                     0,
                     0,
                     width,
                     visible,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    void PlayerWindow::LayoutZoomTip()
    {
        if (!zoomTipPanel_)
        {
            return;
        }

        if (zoomTipHideTick_ == 0 || zoomTipText_.empty() || !videoHost_)
        {
            ShowWindow(zoomTipPanel_, SW_HIDE);
            return;
        }

        RECT hostScreen = { 0 };
        if (!GetWindowRect(videoHost_, &hostScreen))
        {
            ShowWindow(zoomTipPanel_, SW_HIDE);
            return;
        }

        POINT hostTopLeft = { hostScreen.left, hostScreen.top };
        ScreenToClient(hwnd_, &hostTopLeft);

        const int hostW = max(1, hostScreen.right - hostScreen.left);
        const int hostH = max(1, hostScreen.bottom - hostScreen.top);
        const int tipW = max(48, min(ZOOM_TIP_WIDTH, max(48, hostW - 16)));
        const int tipH = max(28, min(ZOOM_TIP_HEIGHT, max(28, hostH - 16)));
        const int x = hostTopLeft.x + (hostW - tipW) / 2;
        const int y = hostTopLeft.y + (hostH - tipH) / 2;

        SetWindowPos(zoomTipPanel_,
                     HWND_TOP,
                     x,
                     y,
                     tipW,
                     tipH,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    RECT PlayerWindow::GetTopButtonRect(HWND hwnd, int button) const
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        RECT rect = { 0 };
        const int top = 5;
        const int height = 17;
        if (button == TOP_BUTTON_LOAD)
        {
            rect.left = 7;
            rect.top = top;
            rect.right = 59;
            rect.bottom = top + height;
            return rect;
        }

        const int buttonW = 19;
        const int gap = 2;
        rect.top = top;
        rect.bottom = top + height;
        rect.right = rc.right - 5;
        rect.left = rect.right - buttonW;
        if (button == TOP_BUTTON_MAXIMIZE)
        {
            OffsetRect(&rect, -(buttonW + gap), 0);
        }
        else if (button == TOP_BUTTON_MINIMIZE)
        {
            OffsetRect(&rect, -(buttonW + gap) * 2, 0);
        }
        return rect;
    }

    int PlayerWindow::HitTestTopButton(HWND hwnd, POINT pt) const
    {
        RECT rect = GetTopButtonRect(hwnd, TOP_BUTTON_LOAD);
        if (PtInRect(&rect, pt))
        {
            return TOP_BUTTON_LOAD;
        }
        rect = GetTopButtonRect(hwnd, TOP_BUTTON_MINIMIZE);
        if (PtInRect(&rect, pt))
        {
            return TOP_BUTTON_MINIMIZE;
        }
        rect = GetTopButtonRect(hwnd, TOP_BUTTON_MAXIMIZE);
        if (PtInRect(&rect, pt))
        {
            return TOP_BUTTON_MAXIMIZE;
        }
        rect = GetTopButtonRect(hwnd, TOP_BUTTON_CLOSE);
        if (PtInRect(&rect, pt))
        {
            return TOP_BUTTON_CLOSE;
        }
        return TOP_BUTTON_NONE;
    }

    void PlayerWindow::PaintTopButton(HDC hdc, HWND hwnd, int button)
    {
        RECT rect = GetTopButtonRect(hwnd, button);
        const bool hovered = topHoveredButton_ == button;
        COLORREF fill = hovered ? RGB(58, 58, 62) : RGB(40, 40, 44);
        if (button == TOP_BUTTON_CLOSE && hovered)
        {
            fill = RGB(138, 46, 46);
        }

        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(88, 88, 94));
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 4, 4);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(brush);

        SetTextColor(hdc, RGB(245, 245, 245));
        if (button == TOP_BUTTON_LOAD)
        {
            HFONT font = CreateFontW(-11,
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
                                     L"Microsoft YaHei UI");
            HGDIOBJ oldFont = NULL;
            if (font)
            {
                oldFont = SelectObject(hdc, font);
            }
            DrawTextW(hdc, L"加载视频", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (oldFont)
            {
                SelectObject(hdc, oldFont);
            }
            if (font)
            {
                DeleteObject(font);
            }
            return;
        }

        HPEN iconPen = CreatePen(PS_SOLID, 1, RGB(245, 245, 245));
        oldPen = SelectObject(hdc, iconPen);
        const int cx = (rect.left + rect.right) / 2;
        const int cy = (rect.top + rect.bottom) / 2;
        if (button == TOP_BUTTON_MINIMIZE)
        {
            MoveToEx(hdc, cx - 4, cy + 3, NULL);
            LineTo(hdc, cx + 5, cy + 3);
        }
        else if (button == TOP_BUTTON_MAXIMIZE)
        {
            Rectangle(hdc, cx - 4, cy - 4, cx + 5, cy + 5);
        }
        else if (button == TOP_BUTTON_CLOSE)
        {
            MoveToEx(hdc, cx - 4, cy - 4, NULL);
            LineTo(hdc, cx + 5, cy + 5);
            MoveToEx(hdc, cx + 4, cy - 4, NULL);
            LineTo(hdc, cx - 5, cy + 5);
        }
        SelectObject(hdc, oldPen);
        DeleteObject(iconPen);
    }

    void PlayerWindow::PaintTopOverlay(HDC hdc, HWND hwnd)
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(26, 26, 28));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);
        SetBkMode(hdc, TRANSPARENT);
        PaintTopButton(hdc, hwnd, TOP_BUTTON_LOAD);
        PaintTopButton(hdc, hwnd, TOP_BUTTON_MINIMIZE);
        PaintTopButton(hdc, hwnd, TOP_BUTTON_MAXIMIZE);
        PaintTopButton(hdc, hwnd, TOP_BUTTON_CLOSE);
    }

    void PlayerWindow::BeginTopOverlayDrag(HWND hwnd, POINT pt)
    {
        if (!hwnd_ || isFullScreen_ || IsZoomed(hwnd_))
        {
            return;
        }

        ClientToScreen(hwnd, &pt);
        if (!GetWindowRect(hwnd_, &topOverlayDragStartWindow_))
        {
            return;
        }

        topOverlayDragStartCursor_ = pt;
        topOverlayDragLastApplyTick_ = 0;
        topOverlayDragActive_ = true;
        movingOrSizing_ = true;
        topOverlayTargetVisible_ = true;
        topOverlayVisiblePixels_ = TOP_OVERLAY_HEIGHT;
        topOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
        SetForegroundWindow(hwnd_);
        SetCapture(hwnd);
        LayoutTopOverlay();
        SetTimer(hwnd_, TIMER_TITLE_DRAG, 15, NULL);
        Logf(L"Top overlay drag begin: cursor=(%ld,%ld), window=(%ld,%ld)-(%ld,%ld)",
             pt.x,
             pt.y,
             topOverlayDragStartWindow_.left,
             topOverlayDragStartWindow_.top,
             topOverlayDragStartWindow_.right,
             topOverlayDragStartWindow_.bottom);
    }

    void PlayerWindow::UpdateTopOverlayDrag(bool force)
    {
        if (!topOverlayDragActive_ || !hwnd_)
        {
            return;
        }

        const DWORD now = GetTickCount();
        if (!force &&
            topOverlayDragLastApplyTick_ != 0 &&
            now - topOverlayDragLastApplyTick_ < 15)
        {
            return;
        }

        POINT cursor = { 0 };
        GetCursorPos(&cursor);
        const int x = topOverlayDragStartWindow_.left + (cursor.x - topOverlayDragStartCursor_.x);
        const int y = topOverlayDragStartWindow_.top + (cursor.y - topOverlayDragStartCursor_.y);
        SetWindowPos(hwnd_,
                     NULL,
                     x,
                     y,
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
        topOverlayDragLastApplyTick_ = now;
    }

    void PlayerWindow::EndTopOverlayDrag(bool applyFinal)
    {
        if (!topOverlayDragActive_)
        {
            return;
        }

        if (applyFinal)
        {
            UpdateTopOverlayDrag(true);
        }

        KillTimer(hwnd_, TIMER_TITLE_DRAG);
        topOverlayDragActive_ = false;
        movingOrSizing_ = false;
        if (GetCapture() == topOverlayPanel_)
        {
            ReleaseCapture();
        }

        topOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
        LayoutControls();
        ResizeVideoWindow();
        UpdateOverlayState();
        UpdateTopOverlayState();
        Logf(L"Top overlay drag end");
    }

    RECT PlayerWindow::GetEmptyPlusButtonRect(HWND hwnd) const
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        const int size = EMPTY_PLUS_BUTTON_SIZE;
        RECT button = { 0 };
        button.left = rc.left + max(0, (rc.right - rc.left - size) / 2);
        button.top = rc.top + max(0, (rc.bottom - rc.top - size) / 2);
        button.right = button.left + size;
        button.bottom = button.top + size;
        return button;
    }

    bool PlayerWindow::HitTestEmptyPlusButton(HWND hwnd, POINT pt) const
    {
        RECT button = GetEmptyPlusButtonRect(hwnd);
        return PtInRect(&button, pt) != FALSE;
    }

    void PlayerWindow::PaintEmptyPlusButton(HDC hdc, HWND hwnd)
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        RECT button = GetEmptyPlusButtonRect(hwnd);
        const COLORREF fill = emptyPlusHovered_ ? RGB(54, 54, 58) : RGB(38, 38, 42);
        const COLORREF border = emptyPlusHovered_ ? RGB(138, 138, 145) : RGB(96, 96, 104);
        HBRUSH buttonBrush = CreateSolidBrush(fill);
        HPEN borderPen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldBrush = SelectObject(hdc, buttonBrush);
        HGDIOBJ oldPen = SelectObject(hdc, borderPen);
        RoundRect(hdc, button.left, button.top, button.right, button.bottom, 8, 8);

        HPEN plusPen = CreatePen(PS_SOLID, 3, RGB(214, 214, 218));
        SelectObject(hdc, plusPen);
        const int cx = (button.left + button.right) / 2;
        const int cy = (button.top + button.bottom) / 2;
        const int half = EMPTY_PLUS_LINE_LENGTH;
        MoveToEx(hdc, cx - half, cy, NULL);
        LineTo(hdc, cx + half + 1, cy);
        MoveToEx(hdc, cx, cy - half, NULL);
        LineTo(hdc, cx, cy + half + 1);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(plusPen);
        DeleteObject(borderPen);
        DeleteObject(buttonBrush);
    }

    bool PlayerWindow::IsWindowFromThisInstance(HWND window) const
    {
        if (!window)
        {
            return false;
        }

        if (window == hwnd_ || window == overlayPanel_ || window == topOverlayPanel_ || window == zoomTipPanel_)
        {
            return true;
        }

        if ((hwnd_ && IsChild(hwnd_, window)) ||
            (overlayPanel_ && IsChild(overlayPanel_, window)) ||
            (topOverlayPanel_ && IsChild(topOverlayPanel_, window)) ||
            (zoomTipPanel_ && IsChild(zoomTipPanel_, window)))
        {
            return true;
        }

        HWND root = GetAncestor(window, GA_ROOT);
        if (root == hwnd_ || root == overlayPanel_ || root == topOverlayPanel_ || root == zoomTipPanel_)
        {
            return true;
        }

        HWND rootOwner = GetAncestor(window, GA_ROOTOWNER);
        if (rootOwner == hwnd_ || rootOwner == overlayPanel_ || rootOwner == topOverlayPanel_ || rootOwner == zoomTipPanel_)
        {
            return true;
        }

        HWND owner = window;
        while ((owner = GetWindow(owner, GW_OWNER)) != NULL)
        {
            if (owner == hwnd_ || owner == overlayPanel_ || owner == topOverlayPanel_ || owner == zoomTipPanel_)
            {
                return true;
            }
        }

        return false;
    }

    void PlayerWindow::TouchMouseActivity(bool force)
    {
        POINT screenPt = { 0 };
        GetCursorPos(&screenPt);
        const bool moved = !haveLastMouseActivityScreen_ ||
                           screenPt.x != lastMouseActivityScreen_.x ||
                           screenPt.y != lastMouseActivityScreen_.y;
        lastMouseActivityScreen_ = screenPt;
        haveLastMouseActivityScreen_ = true;
        mouseInsideWindow_ = true;

        if (!force && !moved)
        {
            return;
        }

        const DWORD now = GetTickCount();
        lastMouseActivityTick_ = now;
        topOverlayKeepVisibleUntil_ = now + MOUSE_UI_IDLE_HIDE_MS;
    }

    bool PlayerWindow::IsMouseUiActive(DWORD now) const
    {
        if (!mouseInsideWindow_ && topOverlayKeepVisibleUntil_ == 0)
        {
            return false;
        }

        if (lastMouseActivityTick_ != 0 &&
            static_cast<DWORD>(now - lastMouseActivityTick_) <= MOUSE_UI_IDLE_HIDE_MS)
        {
            return true;
        }

        return topOverlayKeepVisibleUntil_ != 0 &&
               static_cast<DWORD>(topOverlayKeepVisibleUntil_ - now) < 0x80000000;
    }

    bool PlayerWindow::BeginResizeIfNeeded(HWND hwnd, POINT pt)
    {
        if (!hwnd_ || isFullScreen_ || IsZoomed(hwnd_))
        {
            return false;
        }

        ClientToScreen(hwnd, &pt);
        const LRESULT hit = OnNcHitTest(pt.x, pt.y);
        if (hit == HTCLIENT)
        {
            return false;
        }

        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        SetForegroundWindow(hwnd_);
        SendMessageW(hwnd_, WM_NCLBUTTONDOWN, static_cast<WPARAM>(hit), MAKELPARAM(pt.x, pt.y));
        return true;
    }

    void PlayerWindow::PaintPauseIndicator(int alpha)
    {
        if (!videoPanel_ || alpha <= 0)
        {
            return;
        }

        RECT panelRect = { 0 };
        GetClientRect(videoPanel_, &panelRect);
        const int panelW = panelRect.right - panelRect.left;
        const int panelH = panelRect.bottom - panelRect.top;
        if (panelW <= 0 || panelH <= 0)
        {
            return;
        }

        const int size = max(24, min(PAUSE_INDICATOR_SIZE, min(panelW, panelH)));
        const int left = (panelW - size) / 2;
        const int top = (panelH - size) / 2;

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = size;
        bmi.bmiHeader.biHeight = -size;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = NULL;
        HDC screenDc = GetDC(NULL);
        HBITMAP bitmap = CreateDIBSection(screenDc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
        HDC memoryDc = CreateCompatibleDC(screenDc);
        ReleaseDC(NULL, screenDc);
        if (!bitmap || !memoryDc || !bits)
        {
            if (memoryDc)
            {
                DeleteDC(memoryDc);
            }
            if (bitmap)
            {
                DeleteObject(bitmap);
            }
            return;
        }

        HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);
        DWORD* pixels = reinterpret_cast<DWORD*>(bits);
        ZeroMemory(pixels, size * size * sizeof(DWORD));

        const int x1 = size * 36 / 100;
        const int y1 = size * 26 / 100;
        const int x2 = x1;
        const int y2 = size * 74 / 100;
        const int x3 = size * 74 / 100;
        const int y3 = size / 2;
        const int area = (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1);

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const int w1 = (x2 - x) * (y3 - y) - (x3 - x) * (y2 - y);
                const int w2 = (x3 - x) * (y1 - y) - (x1 - x) * (y3 - y);
                const int w3 = (x1 - x) * (y2 - y) - (x2 - x) * (y1 - y);
                const bool inside = area < 0 ? (w1 <= 0 && w2 <= 0 && w3 <= 0) : (w1 >= 0 && w2 >= 0 && w3 >= 0);
                if (inside)
                {
                    pixels[y * size + x] = 0xFFFFFFFF;
                }
            }
        }

        HDC targetDc = GetDC(videoPanel_);
        BLENDFUNCTION blend = { 0 };
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = static_cast<BYTE>(alpha);
        blend.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(targetDc, left, top, size, size, memoryDc, 0, 0, size, size, blend);
        ReleaseDC(videoPanel_, targetDc);

        SelectObject(memoryDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        pauseIndicatorWasDrawn_ = true;
    }

    void PlayerWindow::UpdatePauseIndicatorState()
    {
        const DWORD now = GetTickCount();
        const bool targetVisible = !openFileDialogActive_ && hasMedia_ && player_ && !isPlaying_ && IsMouseUiActive(now);
        const int targetAlpha = targetVisible ? PAUSE_INDICATOR_MAX_ALPHA : 0;

        if (pauseIndicatorAlpha_ < targetAlpha)
        {
            pauseIndicatorAlpha_ += PAUSE_INDICATOR_ALPHA_STEP;
            if (pauseIndicatorAlpha_ > targetAlpha)
            {
                pauseIndicatorAlpha_ = targetAlpha;
            }
        }
        else if (pauseIndicatorAlpha_ > targetAlpha)
        {
            pauseIndicatorAlpha_ -= PAUSE_INDICATOR_ALPHA_STEP;
            if (pauseIndicatorAlpha_ < targetAlpha)
            {
                pauseIndicatorAlpha_ = targetAlpha;
            }
        }

        if (!player_)
        {
            pauseIndicatorAlpha_ = 0;
            pauseIndicatorWasDrawn_ = false;
            return;
        }

        if (pauseIndicatorAlpha_ > 0)
        {
            player_->UpdateVideo();
            PaintPauseIndicator(pauseIndicatorAlpha_);
        }
        else if (pauseIndicatorWasDrawn_)
        {
            player_->UpdateVideo();
            pauseIndicatorWasDrawn_ = false;
        }
    }

    void PlayerWindow::ShowZoomTip()
    {
        if (!zoomTipPanel_)
        {
            return;
        }

        wchar_t text[32] = { 0 };
        StringCchPrintfW(text, ARRAYSIZE(text), L"%d%%", videoZoomPercent_);
        zoomTipText_ = text;
        zoomTipHideTick_ = GetTickCount() + ZOOM_TIP_VISIBLE_MS;

        LayoutZoomTip();
        InvalidateRect(zoomTipPanel_, NULL, TRUE);
    }

    void PlayerWindow::HideZoomTip()
    {
        zoomTipHideTick_ = 0;
        zoomTipText_.clear();
        if (zoomTipPanel_)
        {
            ShowWindow(zoomTipPanel_, SW_HIDE);
        }
    }

    void PlayerWindow::UpdateZoomTipState()
    {
        if (!zoomTipPanel_ || zoomTipHideTick_ == 0)
        {
            if (zoomTipPanel_)
            {
                ShowWindow(zoomTipPanel_, SW_HIDE);
            }
            return;
        }

        const DWORD now = GetTickCount();
        if (openFileDialogActive_ ||
            zoomTipText_.empty() ||
            static_cast<DWORD>(zoomTipHideTick_ - now) >= 0x80000000)
        {
            HideZoomTip();
            return;
        }

        LayoutZoomTip();
    }

    void PlayerWindow::PaintZoomTip(HDC hdc, HWND hwnd)
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        HBRUSH background = CreateSolidBrush(RGB(20, 20, 24));
        HPEN border = CreatePen(PS_SOLID, 1, RGB(82, 82, 90));
        HGDIOBJ oldBrush = SelectObject(hdc, background);
        HGDIOBJ oldPen = SelectObject(hdc, border);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);

        HFONT font = CreateFontW(-20,
                                 0,
                                 0,
                                 0,
                                 FW_SEMIBOLD,
                                 FALSE,
                                 FALSE,
                                 FALSE,
                                 DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE,
                                 L"Microsoft YaHei UI");
        HGDIOBJ oldFont = NULL;
        if (font)
        {
            oldFont = SelectObject(hdc, font);
        }

        RECT textRect = rc;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(245, 245, 248));
        DrawTextW(hdc,
                  zoomTipText_.c_str(),
                  -1,
                  &textRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        if (oldFont)
        {
            SelectObject(hdc, oldFont);
        }
        if (font)
        {
            DeleteObject(font);
        }
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(border);
        DeleteObject(background);
    }

    void PlayerWindow::UpdateOverlayState()
    {
        if (!overlayPanel_)
        {
            return;
        }

        if (openFileDialogActive_)
        {
            if (overlayVisiblePixels_ != 0)
            {
                overlayVisiblePixels_ = 0;
                overlayTargetVisible_ = false;
                progressMenuActive_ = false;
                LayoutOverlay();
            }
            else
            {
                ShowWindow(overlayPanel_, SW_HIDE);
            }
            return;
        }

        RECT surfaceScreen = { 0 };
        if (!GetOverlayAnchorRect(surfaceScreen))
        {
            if (overlayVisiblePixels_ != 0)
            {
                overlayVisiblePixels_ = 0;
                overlayTargetVisible_ = false;
                progressMenuActive_ = false;
                LayoutOverlay();
            }
            return;
        }

        POINT screenPt = { 0 };
        GetCursorPos(&screenPt);
        const HWND cursorWindow = WindowFromPoint(screenPt);
        const HWND foregroundWindow = GetForegroundWindow();
        const bool cursorOverThisInstance = IsWindowFromThisInstance(cursorWindow);
        const bool foregroundThisInstance = IsWindowFromThisInstance(foregroundWindow);

        RECT visibleOverlay = { 0 };
        if (overlayVisiblePixels_ > 0 && overlayPanel_)
        {
            GetWindowRect(overlayPanel_, &visibleOverlay);
        }
        const bool mouseInHotZone = cursorOverThisInstance &&
                                    PtInRect(&surfaceScreen, screenPt) &&
                                    screenPt.y >= surfaceScreen.bottom - FULLSCREEN_OVERLAY_HOT_ZONE;
        const bool mouseInOverlay = cursorOverThisInstance &&
                                    overlayVisiblePixels_ > 0 &&
                                    PtInRect(&visibleOverlay, screenPt);
        const bool targetVisible = hasMedia_ &&
                                   (draggingSeek_ ||
                                    progressMenuActive_ ||
                                    (foregroundThisInstance && (mouseInHotZone || mouseInOverlay)));
        const int target = targetVisible ? FULLSCREEN_OVERLAY_HEIGHT : 0;

        if (overlayTargetVisible_ != targetVisible)
        {
            overlayTargetVisible_ = targetVisible;
            Logf(L"Progress overlay target %s: cursorScreen=(%ld,%ld), cursorClient=(%ld,%ld), cursorWindow=%p, foreground=%p, overThis=%d, foregroundThis=%d, visible=%d",
                 targetVisible ? L"show" : L"hide",
                 screenPt.x,
                 screenPt.y,
                 screenPt.x - surfaceScreen.left,
                 screenPt.y - surfaceScreen.top,
                 cursorWindow,
                 foregroundWindow,
                 cursorOverThisInstance ? 1 : 0,
                 foregroundThisInstance ? 1 : 0,
                 overlayVisiblePixels_);
        }

        if (overlayVisiblePixels_ < target)
        {
            overlayVisiblePixels_ += OVERLAY_ANIMATION_STEP;
            if (overlayVisiblePixels_ > target)
            {
                overlayVisiblePixels_ = target;
            }
            LayoutOverlay();
        }
        else if (overlayVisiblePixels_ > target)
        {
            overlayVisiblePixels_ -= OVERLAY_ANIMATION_STEP;
            if (overlayVisiblePixels_ < target)
            {
                overlayVisiblePixels_ = target;
            }
            LayoutOverlay();
        }
    }

    void PlayerWindow::UpdateTopOverlayState()
    {
        if (!topOverlayPanel_)
        {
            return;
        }

        if (openFileDialogActive_)
        {
            if (topOverlayVisiblePixels_ != 0)
            {
                topOverlayVisiblePixels_ = 0;
                topOverlayTargetVisible_ = false;
                topHoveredButton_ = TOP_BUTTON_NONE;
                LayoutTopOverlay();
            }
            else
            {
                ShowWindow(topOverlayPanel_, SW_HIDE);
            }
            return;
        }

        RECT windowRect = { 0 };
        GetWindowRect(hwnd_, &windowRect);

        POINT screenPt = { 0 };
        GetCursorPos(&screenPt);
        const DWORD now = GetTickCount();
        const HWND cursorWindow = WindowFromPoint(screenPt);
        const HWND foregroundWindow = GetForegroundWindow();
        const bool cursorOverThisInstance = IsWindowFromThisInstance(cursorWindow);
        const bool foregroundThisInstance = IsWindowFromThisInstance(foregroundWindow);
        const bool cursorInsideWindow = PtInRect(&windowRect, screenPt) != FALSE;
        mouseInsideWindow_ = cursorInsideWindow && cursorOverThisInstance;

        if (topOverlayDragActive_)
        {
            topOverlayKeepVisibleUntil_ = now + MOUSE_UI_IDLE_HIDE_MS;
        }
        const bool targetVisible = foregroundThisInstance &&
                                   (topOverlayDragActive_ || IsMouseUiActive(now));
        const int target = targetVisible ? TOP_OVERLAY_HEIGHT : 0;

        if (topOverlayTargetVisible_ != targetVisible)
        {
            topOverlayTargetVisible_ = targetVisible;
            Logf(L"Top overlay target %s: cursor=(%ld,%ld), visible=%d",
                 targetVisible ? L"show" : L"hide",
                 screenPt.x,
                 screenPt.y,
                 topOverlayVisiblePixels_);
        }

        if (topOverlayVisiblePixels_ < target)
        {
            topOverlayVisiblePixels_ += TOP_OVERLAY_ANIMATION_STEP;
            if (topOverlayVisiblePixels_ > target)
            {
                topOverlayVisiblePixels_ = target;
            }
            LayoutTopOverlay();
        }
        else if (topOverlayVisiblePixels_ > target)
        {
            topOverlayVisiblePixels_ -= TOP_OVERLAY_ANIMATION_STEP;
            if (topOverlayVisiblePixels_ < target)
            {
                topOverlayVisiblePixels_ = target;
            }
            LayoutTopOverlay();
        }

        if (!targetVisible)
        {
            topOverlayKeepVisibleUntil_ = 0;
        }
    }

    void PlayerWindow::ResetVideoTransform()
    {
        videoZoomPercent_ = 100;
        videoPanX_ = 0;
        videoPanY_ = 0;
        videoMouseDown_ = false;
        draggingVideo_ = false;
    }

    void PlayerWindow::GetBaseVideoSize(int hostW, int hostH, int& baseW, int& baseH) const
    {
        baseW = max(1, hostW);
        baseH = max(1, hostH);

        if (nativeVideoWidth_ <= 0 || nativeVideoHeight_ <= 0 || hostW <= 0 || hostH <= 0)
        {
            return;
        }

        const LONGLONG wideByWidth = static_cast<LONGLONG>(hostW) * nativeVideoHeight_;
        const LONGLONG wideByHeight = static_cast<LONGLONG>(hostH) * nativeVideoWidth_;
        if (wideByWidth <= wideByHeight)
        {
            baseW = max(1, hostW);
            baseH = max(1, static_cast<int>(static_cast<LONGLONG>(hostW) * nativeVideoHeight_ / nativeVideoWidth_));
        }
        else
        {
            baseH = max(1, hostH);
            baseW = max(1, static_cast<int>(static_cast<LONGLONG>(hostH) * nativeVideoWidth_ / nativeVideoHeight_));
        }
    }

    void PlayerWindow::ClampVideoPan(int videoW, int videoH, int hostW, int hostH)
    {
        if (hostW <= 0 || hostH <= 0 || videoW <= 0 || videoH <= 0)
        {
            videoPanX_ = 0;
            videoPanY_ = 0;
            return;
        }

        int left = (hostW - videoW) / 2 + videoPanX_;
        int right = left + videoW;
        const int minVisibleX = min(32, max(1, videoW));
        if (left > hostW - minVisibleX)
        {
            videoPanX_ -= left - (hostW - minVisibleX);
        }
        if (right < minVisibleX)
        {
            videoPanX_ += minVisibleX - right;
        }

        int top = (hostH - videoH) / 2 + videoPanY_;
        int bottom = top + videoH;
        const int minVisibleY = min(32, max(1, videoH));
        if (top > hostH - minVisibleY)
        {
            videoPanY_ -= top - (hostH - minVisibleY);
        }
        if (bottom < minVisibleY)
        {
            videoPanY_ += minVisibleY - bottom;
        }
    }

    RECT PlayerWindow::GetVideoRenderRect(int hostW, int hostH)
    {
        int baseW = 0;
        int baseH = 0;
        GetBaseVideoSize(hostW, hostH, baseW, baseH);

        const int videoW = max(1, MulDiv(baseW, videoZoomPercent_, 100));
        const int videoH = max(1, MulDiv(baseH, videoZoomPercent_, 100));
        ClampVideoPan(videoW, videoH, hostW, hostH);

        RECT rect = { 0 };
        rect.left = (hostW - videoW) / 2 + videoPanX_;
        rect.top = (hostH - videoH) / 2 + videoPanY_;
        rect.right = rect.left + videoW;
        rect.bottom = rect.top + videoH;
        return rect;
    }

    void PlayerWindow::ApplyVideoTransform()
    {
        if (!videoHost_ || !videoPanel_)
        {
            return;
        }

        RECT host = { 0 };
        GetClientRect(videoHost_, &host);
        const int hostW = max(1, host.right - host.left);
        const int hostH = max(1, host.bottom - host.top);
        RECT rect = GetVideoRenderRect(hostW, hostH);
        MoveWindow(videoPanel_,
                   rect.left,
                   rect.top,
                   max(1, rect.right - rect.left),
                   max(1, rect.bottom - rect.top),
                   TRUE);
        InvalidateRect(videoHost_, NULL, TRUE);

        if (player_)
        {
            HRESULT hr = player_->UpdateVideo();
            if (FAILED(hr))
            {
                Logf(L"UpdateVideo failed: 0x%08X", static_cast<unsigned int>(hr));
            }
        }
    }

    bool PlayerWindow::ReadNativeVideoSize()
    {
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        if (!player_)
        {
            return false;
        }

        SIZE videoSize = { 0 };
        SIZE aspectSize = { 0 };
        HRESULT hr = player_->GetNativeVideoSize(&videoSize, &aspectSize);
        if (SUCCEEDED(hr) && videoSize.cx > 0 && videoSize.cy > 0)
        {
            nativeVideoWidth_ = videoSize.cx;
            nativeVideoHeight_ = videoSize.cy;
        }

        Logf(L"GetNativeVideoSize returned 0x%08X, video=%ldx%ld, aspect=%ldx%ld, stored=%dx%d",
             static_cast<unsigned int>(hr),
             videoSize.cx,
             videoSize.cy,
             aspectSize.cx,
             aspectSize.cy,
             nativeVideoWidth_,
             nativeVideoHeight_);
        ApplyVideoTransform();
        return nativeVideoWidth_ > 0 && nativeVideoHeight_ > 0;
    }

    void PlayerWindow::ResizeWindowToVideoDefault()
    {
        if (!hwnd_ || isFullScreen_ || nativeVideoWidth_ <= 0 || nativeVideoHeight_ <= 0)
        {
            return;
        }

        HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { 0 };
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(monitor, &mi);
        const int workW = max(320, mi.rcWork.right - mi.rcWork.left);
        const int workH = max(240, mi.rcWork.bottom - mi.rcWork.top);

        double scale = 1.0;
        if (nativeVideoWidth_ > workW / 2 || nativeVideoHeight_ > workH / 2)
        {
            scale = 0.5;
        }

        int targetW = max(1, static_cast<int>(nativeVideoWidth_ * scale + 0.5));
        int targetH = max(1, static_cast<int>(nativeVideoHeight_ * scale + 0.5));
        if (targetW > workW || targetH > workH)
        {
            const double sx = static_cast<double>(workW) / targetW;
            const double sy = static_cast<double>(workH) / targetH;
            const double fitScale = sx < sy ? sx : sy;
            targetW = max(1, static_cast<int>(targetW * fitScale + 0.5));
            targetH = max(1, static_cast<int>(targetH * fitScale + 0.5));
        }

        if (IsZoomed(hwnd_))
        {
            ShowWindow(hwnd_, SW_RESTORE);
        }

        const int x = mi.rcWork.left + (workW - targetW) / 2;
        const int y = mi.rcWork.top + (workH - targetH) / 2;
        SetWindowPos(hwnd_,
                     NULL,
                     x,
                     y,
                     targetW,
                     targetH,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
        ResetVideoTransform();
        LayoutControls();
        Logf(L"Window resized for video: native=%dx%d, window=%dx%d",
             nativeVideoWidth_,
             nativeVideoHeight_,
             targetW,
             targetH);
    }

    void PlayerWindow::ResizeVideoWindow()
    {
        ApplyVideoTransform();
    }

    void PlayerWindow::ShowControls(int cmdShow)
    {
        ShowWindow(openButton_, cmdShow);
        ShowWindow(playButton_, cmdShow);
        ShowWindow(pauseButton_, cmdShow);
        ShowWindow(stopButton_, cmdShow);
        ShowWindow(setAButton_, cmdShow);
        ShowWindow(setBButton_, cmdShow);
        ShowWindow(aPrevFrameButton_, cmdShow);
        ShowWindow(aNextFrameButton_, cmdShow);
        ShowWindow(bPrevFrameButton_, cmdShow);
        ShowWindow(bNextFrameButton_, cmdShow);
        ShowWindow(clearButton_, cmdShow);
        ShowWindow(loopCheck_, cmdShow);
        ShowWindow(fullScreenButton_, cmdShow);
        ShowWindow(seekSlider_, cmdShow);
        ShowWindow(timeText_, cmdShow);
        ShowWindow(abText_, cmdShow);
    }

    void PlayerWindow::ToggleFullScreen()
    {
        if (GetParent(hwnd_) != NULL || (GetWindowLongPtrW(hwnd_, GWL_STYLE) & WS_CHILD))
        {
            return;
        }

        if (isFullScreen_)
        {
            ExitFullScreen();
        }
        else
        {
            EnterFullScreen();
        }
    }

    void PlayerWindow::EnterFullScreen()
    {
        if (isFullScreen_)
        {
            return;
        }

        savedPlacement_.length = sizeof(savedPlacement_);
        GetWindowPlacement(hwnd_, &savedPlacement_);
        savedStyle_ = GetWindowLongW(hwnd_, GWL_STYLE);
        savedExStyle_ = GetWindowLongW(hwnd_, GWL_EXSTYLE);

        HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { 0 };
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(monitor, &mi);

        isFullScreen_ = true;
        overlayVisiblePixels_ = 0;
        overlayTargetVisible_ = false;
        progressMenuActive_ = false;
        SetWindowLongW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_ & ~WS_EX_WINDOWEDGE);
        SetWindowPos(hwnd_, HWND_TOP,
                     mi.rcMonitor.left,
                     mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        LayoutControls();
        ResizeVideoWindow();
        Logf(L"EnterFullScreen: monitor=(%ld,%ld)-(%ld,%ld), hwnd=%p, overlay=%p",
             mi.rcMonitor.left,
             mi.rcMonitor.top,
             mi.rcMonitor.right,
             mi.rcMonitor.bottom,
             hwnd_,
             overlayPanel_);
    }

    void PlayerWindow::ExitFullScreen()
    {
        if (!isFullScreen_)
        {
            return;
        }

        isFullScreen_ = false;
        overlayVisiblePixels_ = 0;
        overlayTargetVisible_ = false;
        progressMenuActive_ = false;
        ShowWindow(overlayPanel_, SW_HIDE);
        SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
        SetWindowPlacement(hwnd_, &savedPlacement_);
        SetWindowPos(hwnd_, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        LayoutControls();
        ResizeVideoWindow();
        Logf(L"ExitFullScreen: hwnd=%p, overlay=%p", hwnd_, overlayPanel_);
    }

    void PlayerWindow::OnGetMinMaxInfo(MINMAXINFO* info)
    {
        if (!info)
        {
            return;
        }

        info->ptMinTrackSize.x = 320;
        info->ptMinTrackSize.y = 240;
    }

    LRESULT PlayerWindow::OnNcHitTest(int screenX, int screenY) const
    {
        if (IsZoomed(hwnd_) || isFullScreen_)
        {
            return HTCLIENT;
        }

        RECT rc = { 0 };
        GetWindowRect(hwnd_, &rc);
        const int border = 8;
        const bool left = screenX >= rc.left && screenX < rc.left + border;
        const bool right = screenX <= rc.right && screenX > rc.right - border;
        const bool top = screenY >= rc.top && screenY < rc.top + border;
        const bool bottom = screenY <= rc.bottom && screenY > rc.bottom - border;

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

        return HTCLIENT;
    }

    void PlayerWindow::OnCommand(WORD id, WORD notifyCode)
    {
        if (id == IDC_VIDEO_PANEL && notifyCode == STN_DBLCLK)
        {
            KillTimer(hwnd_, TIMER_VIDEO_CLICK);
            ToggleFullScreen();
            return;
        }
        if (id == IDC_VIDEO_PANEL && notifyCode == STN_CLICKED)
        {
            SetTimer(hwnd_, TIMER_VIDEO_CLICK, GetDoubleClickTime() + 30, NULL);
            return;
        }

        switch (id)
        {
        case IDC_OPEN:
            OpenFileDialog();
            break;
        case IDC_PLAY:
            Play();
            break;
        case IDC_PAUSE:
            Pause();
            break;
        case IDC_STOP:
            Stop();
            break;
        case IDC_SET_A:
            SetLoopA();
            break;
        case IDC_SET_B:
            SetLoopB();
            break;
        case IDC_A_PREV_FRAME:
            NudgeLoopPoint(true, -1);
            break;
        case IDC_A_NEXT_FRAME:
            NudgeLoopPoint(true, 1);
            break;
        case IDC_B_PREV_FRAME:
            NudgeLoopPoint(false, -1);
            break;
        case IDC_B_NEXT_FRAME:
            NudgeLoopPoint(false, 1);
            break;
        case IDC_CLEAR_AB:
            ClearLoop();
            break;
        case IDC_LOOP_AB:
            UpdateLoopText();
            ApplyPlaybackSegment();
            break;
        case IDC_FULLSCREEN:
            ToggleFullScreen();
            break;
        default:
            break;
        }
    }

    void PlayerWindow::OpenFileDialog()
    {
        wchar_t path[MAX_PATH] = { 0 };

        openFileDialogActive_ = true;
        topOverlayTargetVisible_ = false;
        topOverlayVisiblePixels_ = 0;
        topOverlayKeepVisibleUntil_ = 0;
        topHoveredButton_ = TOP_BUTTON_NONE;
        if (topOverlayPanel_)
        {
            ShowWindow(topOverlayPanel_, SW_HIDE);
        }
        HideZoomTip();

        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd_;
        ofn.lpstrFilter = L"视频文件 (*.mp4;*.avi)\0*.mp4;*.avi\0MP4 文件 (*.mp4)\0*.mp4\0AVI 文件 (*.avi)\0*.avi\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = ARRAYSIZE(path);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = L"mp4";

        if (GetOpenFileNameW(&ofn))
        {
            Logf(L"OpenFileDialog selected: %s", path);
            LoadFile(path);
        }
        else
        {
            Logf(L"OpenFileDialog canceled or failed. CommDlgExtendedError=%lu", CommDlgExtendedError());
        }
        openFileDialogActive_ = false;
    }

    void PlayerWindow::OnDropFiles(HDROP drop)
    {
        wchar_t path[MAX_PATH] = { 0 };
        if (DragQueryFileW(drop, 0, path, ARRAYSIZE(path)))
        {
            Logf(L"Drop file selected: %s", path);
            LoadFile(path);
        }
        DragFinish(drop);
    }

    void PlayerWindow::Play()
    {
        if (!player_)
        {
            return;
        }

        suppressReplay_ = false;
        markerSeekPending_ = false;
        if (IsActiveAbLoop())
        {
            REFERENCE_TIME pos = 0;
            const REFERENCE_TIME trigger = LoopTriggerPosition();
            if (trigger >= 0 && GetPosition(pos) && pos >= trigger)
            {
                Logf(L"Play requested at/after AB trigger: pos=%s (%I64d), trigger=%s (%I64d). Starting from A=%s (%I64d)",
                     FormatTime(pos).c_str(),
                     pos,
                     FormatTime(trigger).c_str(),
                     trigger,
                     FormatTime(loopA_).c_str(),
                     loopA_);
                SeekTo(loopA_, false);
                UpdatePositionUi(loopA_);
            }
        }

        HRESULT hr = player_->Play();
        if (SUCCEEDED(hr))
        {
            isPlaying_ = true;
            SetStatus(L"Playing");
            Logf(L"Play succeeded");
        }
        else
        {
            Logf(L"Play failed: 0x%08X", static_cast<unsigned int>(hr));
            SetStatus((L"Play failed: " + HResultText(hr)).c_str());
        }

        UpdateControls();
    }

    float PlayerWindow::PlaybackRate() const
    {
        return static_cast<float>(playbackRateTenths_) / 10.0f;
    }

    void PlayerWindow::FormatPlaybackRate(wchar_t* buffer, size_t count) const
    {
        if (!buffer || count == 0)
        {
            return;
        }

        StringCchPrintfW(buffer,
                         count,
                         L"%d.%dx",
                         playbackRateTenths_ / 10,
                         playbackRateTenths_ % 10);
    }

    bool PlayerWindow::ApplyPlaybackRate(bool showStatus)
    {
        if (!player_)
        {
            return false;
        }

        const float rate = PlaybackRate();
        HRESULT hr = player_->SetRate(rate);
        wchar_t rateText[32] = { 0 };
        FormatPlaybackRate(rateText, ARRAYSIZE(rateText));
        Logf(L"Set playback rate %s returned 0x%08X", rateText, static_cast<unsigned int>(hr));
        if (SUCCEEDED(hr))
        {
            if (showStatus)
            {
                std::wstring status = L"Speed ";
                status += rateText;
                SetStatus(status.c_str());
            }
            return true;
        }

        if (showStatus)
        {
            std::wstring status = L"Speed change failed: ";
            status += HResultText(hr);
            SetStatus(status.c_str());
        }
        return false;
    }

    void PlayerWindow::AdjustPlaybackRate(int steps)
    {
        if (!hasMedia_ || !player_)
        {
            return;
        }

        const int oldTenths = playbackRateTenths_;
        int nextTenths = playbackRateTenths_ + steps;
        if (nextTenths < 1)
        {
            nextTenths = 1;
        }
        if (nextTenths > 30)
        {
            nextTenths = 30;
        }

        if (nextTenths == oldTenths)
        {
            wchar_t rateText[32] = { 0 };
            FormatPlaybackRate(rateText, ARRAYSIZE(rateText));
            std::wstring status = L"Speed ";
            status += rateText;
            SetStatus(status.c_str());
            return;
        }

        playbackRateTenths_ = nextTenths;
        if (!ApplyPlaybackRate(true))
        {
            playbackRateTenths_ = oldTenths;
        }
    }

    void PlayerWindow::TogglePlayback()
    {
        if (!hasMedia_)
        {
            return;
        }

        if (isPlaying_)
        {
            Pause();
        }
        else
        {
            Play();
        }
    }

    void PlayerWindow::Pause()
    {
        if (!player_)
        {
            return;
        }

        manualPauseRequest_ = true;
        HRESULT hr = player_->Pause();
        if (SUCCEEDED(hr))
        {
            isPlaying_ = false;
            loopReplayPending_ = false;
            loopReplayFastAttempt_ = false;
            markerSeekPending_ = false;
            SetStatus(L"Paused");
            Logf(L"Pause succeeded");
        }
        else
        {
            manualPauseRequest_ = false;
            Logf(L"Pause failed: 0x%08X", static_cast<unsigned int>(hr));
            SetStatus((L"Pause failed: " + HResultText(hr)).c_str());
        }

        UpdateControls();
    }

    void PlayerWindow::Stop()
    {
        if (!player_)
        {
            return;
        }

        suppressReplay_ = true;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        manualPauseRequest_ = false;
        HRESULT hr = player_->Stop();
        Logf(L"Stop returned 0x%08X", static_cast<unsigned int>(hr));
        isPlaying_ = false;
        SeekTo(0, false);
        UpdatePositionUi(0);
        SetStatus(L"Stopped");
        UpdateControls();
    }

    bool PlayerWindow::ReadDuration()
    {
        duration_ = 0;
        if (!player_)
        {
            return false;
        }

        PROPVARIANT value;
        PropVariantInit(&value);
        HRESULT hr = player_->GetDuration(MFP_POSITIONTYPE_100NS, &value);
        LONGLONG duration = 0;
        const bool ok = SUCCEEDED(hr) && ReadInt64PropVariant(value, duration);
        if (ok)
        {
            duration_ = duration;
        }
        Logf(L"GetDuration returned 0x%08X, vt=%u, duration=%s (%I64d)",
             static_cast<unsigned int>(hr),
             static_cast<unsigned int>(value.vt),
             FormatTime(duration_).c_str(),
             duration_);
        PropVariantClear(&value);
        return ok;
    }

    void PlayerWindow::DetectFrameDuration(IMFPMediaItem* item)
    {
        frameDuration_ = DEFAULT_FRAME_DURATION;

        if (!item)
        {
            Logf(L"Frame rate unavailable: no media item. Fallback frame duration=%I64d", frameDuration_);
            return;
        }

        DWORD streamCount = 0;
        HRESULT hr = item->GetNumberOfStreams(&streamCount);
        if (FAILED(hr))
        {
            Logf(L"GetNumberOfStreams failed: 0x%08X. Fallback frame duration=%I64d", static_cast<unsigned int>(hr), frameDuration_);
            return;
        }

        for (DWORD i = 0; i < streamCount; ++i)
        {
            PROPVARIANT value;
            PropVariantInit(&value);
            hr = item->GetStreamAttribute(i, MF_MT_FRAME_RATE, &value);
            if (SUCCEEDED(hr) && value.vt == VT_UI8)
            {
                const ULONGLONG packed = value.uhVal.QuadPart;
                const DWORD numerator = static_cast<DWORD>(packed >> 32);
                const DWORD denominator = static_cast<DWORD>(packed & 0xffffffff);
                if (numerator > 0 && denominator > 0)
                {
                    frameDuration_ = static_cast<REFERENCE_TIME>((static_cast<double>(ONE_SECOND) * denominator / numerator) + 0.5);
                    Logf(L"Detected frame rate from stream %lu: %lu/%lu fps, frameDuration=%I64d",
                         i,
                         numerator,
                         denominator,
                         frameDuration_);
                    PropVariantClear(&value);
                    return;
                }
            }
            PropVariantClear(&value);
        }

        Logf(L"Frame rate not found. Fallback frame duration=%I64d, about %.3f fps",
             frameDuration_,
             static_cast<double>(ONE_SECOND) / frameDuration_);
    }

    bool PlayerWindow::GetPosition(REFERENCE_TIME& pos) const
    {
        pos = 0;
        if (!player_)
        {
            return false;
        }

        PROPVARIANT value;
        PropVariantInit(&value);
        HRESULT hr = player_->GetPosition(MFP_POSITIONTYPE_100NS, &value);
        LONGLONG current = 0;
        const bool ok = SUCCEEDED(hr) && ReadInt64PropVariant(value, current);
        if (ok)
        {
            pos = current;
        }
        PropVariantClear(&value);
        return ok;
    }

    REFERENCE_TIME PlayerWindow::LoopMinLength() const
    {
        return frameDuration_ > 0 ? frameDuration_ : DEFAULT_FRAME_DURATION;
    }

    REFERENCE_TIME PlayerWindow::ClampMediaPosition(REFERENCE_TIME pos) const
    {
        if (pos < 0)
        {
            pos = 0;
        }
        if (duration_ > 0 && pos > duration_)
        {
            pos = duration_;
        }

        return pos;
    }

    REFERENCE_TIME PlayerWindow::SnapToNearestFrame(REFERENCE_TIME pos) const
    {
        const REFERENCE_TIME oneFrame = LoopMinLength();
        if (oneFrame <= 1)
        {
            return ClampMediaPosition(pos);
        }

        pos = ClampMediaPosition(pos);
        const REFERENCE_TIME snapped = ((pos + oneFrame / 2) / oneFrame) * oneFrame;
        return ClampMediaPosition(snapped);
    }

    REFERENCE_TIME PlayerWindow::LoopBFrameEnd() const
    {
        if (loopB_ < 0)
        {
            return -1;
        }

        return ClampMediaPosition(loopB_ + LoopMinLength());
    }

    REFERENCE_TIME PlayerWindow::LoopTriggerPosition() const
    {
        if (!HasValidAbLoop())
        {
            return -1;
        }

        const REFERENCE_TIME oneFrame = LoopMinLength();
        const REFERENCE_TIME triggerLead = oneFrame * LOOP_TRIGGER_EARLY_FRAMES;
        const REFERENCE_TIME bFrameEnd = LoopBFrameEnd();
        REFERENCE_TIME trigger = bFrameEnd - triggerLead;
        REFERENCE_TIME halfFrame = oneFrame / 2;
        if (halfFrame < 1)
        {
            halfFrame = 1;
        }
        const REFERENCE_TIME minTrigger = loopA_ + halfFrame;

        if (trigger < minTrigger)
        {
            trigger = minTrigger;
        }
        if (trigger > bFrameEnd)
        {
            trigger = bFrameEnd;
        }

        return trigger;
    }

    bool PlayerWindow::HasValidAbLoop() const
    {
        return loopA_ >= 0 && loopB_ > loopA_;
    }

    bool PlayerWindow::IsActiveAbLoop() const
    {
        return IsLoopEnabled() && HasValidAbLoop();
    }

    REFERENCE_TIME PlayerWindow::ActiveLoopLength() const
    {
        return HasValidAbLoop() ? loopB_ - loopA_ : 0;
    }

    bool PlayerWindow::IsShortActiveAbLoop() const
    {
        return IsActiveAbLoop() && ActiveLoopLength() <= ONE_SECOND;
    }

    bool PlayerWindow::ShouldLogLoopReplay() const
    {
        return loopReplayCount_ <= 5 || (loopReplayCount_ % 20) == 0;
    }

    bool PlayerWindow::ShouldLogPlayerEvent(MFP_EVENT_TYPE eventType) const
    {
        if (!IsShortActiveAbLoop())
        {
            return true;
        }

        switch (eventType)
        {
        case MFP_EVENT_TYPE_PLAY:
        case MFP_EVENT_TYPE_PAUSE:
        case MFP_EVENT_TYPE_POSITION_SET:
            return ShouldLogLoopReplay();
        default:
            return true;
        }
    }

    bool PlayerWindow::IsMarkerSeekStillPending()
    {
        if (!markerSeekPending_)
        {
            return false;
        }

        const DWORD now = GetTickCount();
        if ((now - markerSeekPendingTick_) > MARKER_SEEK_PENDING_TIMEOUT_MS)
        {
            markerSeekPending_ = false;
            Logf(L"Marker nudge seek pending timed out; accepting next key");
            return false;
        }

        return true;
    }

    bool PlayerWindow::ApplyPlaybackSegment()
    {
        const bool hadSegment = segmentStopApplied_;
        segmentStopApplied_ = false;
        if (!player_)
        {
            return false;
        }

        const bool activeAb = IsActiveAbLoop();
        if (!activeAb && !hadSegment)
        {
            return false;
        }

        IMFPMediaItem* item = NULL;
        HRESULT hr = player_->GetMediaItem(&item);
        if (FAILED(hr) || !item)
        {
            Logf(L"GetMediaItem for segment failed: 0x%08X", static_cast<unsigned int>(hr));
            return false;
        }

        REFERENCE_TIME start = 0;
        REFERENCE_TIME stop = duration_;
        if (activeAb)
        {
            start = loopA_;
            stop = LoopBFrameEnd();
        }

        if (stop <= start)
        {
            Logf(L"Skip %s segment: invalid start/stop start=%s (%I64d), stop=%s (%I64d)",
                 activeAb ? L"AB" : L"full",
                 FormatTime(start).c_str(),
                 start,
                 FormatTime(stop).c_str(),
                 stop);
            SafeRelease(item);
            return false;
        }

        PROPVARIANT startValue;
        PROPVARIANT stopValue;
        InitInt64PropVariant(startValue, start);
        InitInt64PropVariant(stopValue, stop);
        hr = item->SetStartStopPosition(&MFP_POSITIONTYPE_100NS,
                                        &startValue,
                                        &MFP_POSITIONTYPE_100NS,
                                        &stopValue);
        PropVariantClear(&startValue);
        PropVariantClear(&stopValue);

        segmentStopApplied_ = activeAb && SUCCEEDED(hr);
        Logf(L"SetStartStopPosition %s returned 0x%08X, start=%s (%I64d), stop=%s (%I64d), nativeStop=%d",
             activeAb ? L"AB" : L"full",
             static_cast<unsigned int>(hr),
             FormatTime(start).c_str(),
             start,
             FormatTime(stop).c_str(),
             stop,
             segmentStopApplied_ ? 1 : 0);

        SafeRelease(item);
        return segmentStopApplied_;
    }

    void PlayerWindow::SetLoopA()
    {
        if (!hasMedia_)
        {
            return;
        }

        REFERENCE_TIME pos = 0;
        if (!GetPosition(pos))
        {
            return;
        }

        SetLoopAAt(pos, pos);
    }

    void PlayerWindow::SetLoopAAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos)
    {
        pos = SnapToNearestFrame(pos);
        loopA_ = pos;
        if (loopB_ >= 0 && loopB_ <= loopA_)
        {
            loopB_ = -1;
        }
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        loopReplayCount_ = 0;

        UpdateLoopText();
        ApplyPlaybackSegment();
        SetStatus(L"A point set");
        Logf(L"A point set: %s (%I64d), raw=%s (%I64d), frameDuration=%I64d",
             FormatTime(loopA_).c_str(),
             loopA_,
             FormatTime(rawPos).c_str(),
             rawPos,
             LoopMinLength());
        LogLoopTrigger();
    }

    void PlayerWindow::SetLoopB()
    {
        if (!hasMedia_)
        {
            return;
        }

        REFERENCE_TIME pos = 0;
        if (!GetPosition(pos))
        {
            return;
        }

        SetLoopBAt(pos, pos);
    }

    void PlayerWindow::SetLoopBAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos)
    {
        pos = SnapToNearestFrame(pos);
        if (loopA_ < 0)
        {
            loopA_ = 0;
            Logf(L"B set before A. A defaults to 00:00.000 (0)");
        }

        if (pos <= loopA_)
        {
            SetStatus(L"B must be after A");
            return;
        }

        loopB_ = pos;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        loopReplayCount_ = 0;
        UpdateLoopText();
        ApplyPlaybackSegment();
        SetStatus(L"B point set");
        Logf(L"B point set: %s (%I64d), raw=%s (%I64d), frameDuration=%I64d",
             FormatTime(loopB_).c_str(),
             loopB_,
             FormatTime(rawPos).c_str(),
             rawPos,
             LoopMinLength());
        LogLoopTrigger();
    }

    void PlayerWindow::ClearLoop()
    {
        loopA_ = -1;
        loopB_ = -1;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        loopReplayCount_ = 0;
        UpdateLoopText();
        ApplyPlaybackSegment();
        SetStatus(L"AB loop cleared");
        Logf(L"AB loop cleared");
    }

    bool PlayerWindow::NudgeLoopPoint(bool isA, int frames, bool previewFrame)
    {
        if (!hasMedia_)
        {
            return false;
        }

        REFERENCE_TIME& point = isA ? loopA_ : loopB_;
        if (point < 0)
        {
            SetStatus(isA ? L"Set A first" : L"Set B first");
            return false;
        }

        const REFERENCE_TIME oneFrame = LoopMinLength();
        point = SnapToNearestFrame(point);
        REFERENCE_TIME next = SnapToNearestFrame(point + oneFrame * frames);

        if (next < 0)
        {
            next = 0;
        }
        if (duration_ > 0 && next > duration_)
        {
            next = duration_;
        }

        if (isA)
        {
            if (loopB_ >= 0 && next >= loopB_)
            {
                next = loopB_ - oneFrame;
            }
            if (next < 0)
            {
                next = 0;
            }
            loopA_ = next;
        }
        else
        {
            if (loopA_ >= 0 && next <= loopA_)
            {
                next = loopA_ + oneFrame;
            }
            if (duration_ > 0 && next > duration_)
            {
                next = duration_;
            }
            if (loopA_ >= 0 && next <= loopA_)
            {
                SetStatus(L"No room to move B");
                return false;
            }
            loopB_ = next;
        }

        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        UpdateLoopText();
        ApplyPlaybackSegment();

        if (previewFrame && isPlaying_ && player_)
        {
            manualPauseRequest_ = true;
            HRESULT pauseHr = player_->Pause();
            Logf(L"%s nudge pause for frame preview returned 0x%08X",
                 isA ? L"A" : L"B",
                 static_cast<unsigned int>(pauseHr));
            if (SUCCEEDED(pauseHr))
            {
                isPlaying_ = false;
            }
            else
            {
                manualPauseRequest_ = false;
            }
        }

        SeekTo(next, !previewFrame);
        UpdatePositionUi(next);
        SetStatus(isA ? L"A moved one frame" : L"B moved one frame");
        Logf(L"%s moved by %d frame(s): %s (%I64d)", isA ? L"A" : L"B", frames, FormatTime(next).c_str(), next);
        LogLoopTrigger();
        return true;
    }

    RECT PlayerWindow::GetProgressTrackRect(HWND hwnd, bool overlay) const
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        RECT track = rc;
        const int edge = overlay ? 10 : 6;
        track.left += edge + PROGRESS_TIME_LABEL_WIDTH + PROGRESS_TIME_LABEL_GAP;
        track.right -= edge + PROGRESS_TIME_LABEL_WIDTH + PROGRESS_TIME_LABEL_GAP;
        const int centerY = overlay ? 13 : 11;
        track.top = centerY - 2;
        track.bottom = centerY + 2;
        if (track.right <= track.left)
        {
            track.left = rc.left + edge;
            track.right = rc.right - edge;
        }
        if (track.right <= track.left)
        {
            track.right = track.left + 1;
        }
        return track;
    }

    RECT PlayerWindow::GetOverlayPlayRect(HWND hwnd) const
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        const int buttonW = 41;
        const int buttonH = 15;
        RECT button = { 0 };
        button.left = (rc.right - buttonW) / 2;
        button.top = 24;
        button.right = button.left + buttonW;
        button.bottom = button.top + buttonH;
        return button;
    }

    int PlayerWindow::TimeToProgressX(REFERENCE_TIME pos, const RECT& track) const
    {
        if (duration_ <= 0)
        {
            return track.left;
        }

        pos = ClampMediaPosition(pos);
        const int width = max(1, track.right - track.left);
        return track.left + static_cast<int>((pos * width) / duration_);
    }

    REFERENCE_TIME PlayerWindow::ProgressXToTime(int x, const RECT& track) const
    {
        if (duration_ <= 0)
        {
            return 0;
        }

        if (x < track.left)
        {
            x = track.left;
        }
        if (x > track.right)
        {
            x = track.right;
        }

        const int width = max(1, track.right - track.left);
        return ClampMediaPosition(duration_ * (x - track.left) / width);
    }

    bool PlayerWindow::IsPointInProgressArea(HWND hwnd, bool overlay, POINT pt) const
    {
        RECT track = GetProgressTrackRect(hwnd, overlay);
        RECT area = track;
        InflateRect(&area, 8, overlay ? 11 : 13);
        return PtInRect(&area, pt) != FALSE;
    }

    int PlayerWindow::HitTestProgressMarker(HWND hwnd, bool overlay, POINT pt) const
    {
        if (!hasMedia_ || duration_ <= 0)
        {
            return PROGRESS_MARKER_NONE;
        }

        RECT track = GetProgressTrackRect(hwnd, overlay);
        const int cy = (track.top + track.bottom) / 2;
        const int hitRadius = 7;
        int best = PROGRESS_MARKER_NONE;
        int bestDistance = hitRadius + 1;

        if (loopA_ >= 0)
        {
            const int ax = TimeToProgressX(loopA_, track);
            const int dx = abs(pt.x - ax);
            const int dy = abs(pt.y - cy);
            if (dx <= hitRadius && dy <= hitRadius && dx < bestDistance)
            {
                best = PROGRESS_MARKER_A;
                bestDistance = dx;
            }
        }

        if (loopB_ >= 0)
        {
            const int bx = TimeToProgressX(loopB_, track);
            const int dx = abs(pt.x - bx);
            const int dy = abs(pt.y - cy);
            if (dx <= hitRadius && dy <= hitRadius && dx < bestDistance)
            {
                best = PROGRESS_MARKER_B;
                bestDistance = dx;
            }
        }

        return best;
    }

    void PlayerWindow::PaintProgressMarker(HDC hdc, HWND hwnd, bool overlay, const RECT& track, int marker, REFERENCE_TIME pos)
    {
        if (pos < 0)
        {
            return;
        }

        const bool hovered = hoveredMarker_ == marker && hoverMarkerWindow_ == hwnd;
        const int x = TimeToProgressX(pos, track);
        const int cy = (track.top + track.bottom) / 2;
        const int radius = hovered ? 5 : 3;

        COLORREF fill = marker == PROGRESS_MARKER_A ? RGB(255, 196, 61) : RGB(89, 195, 255);
        COLORREF outline = overlay ? RGB(20, 20, 20) : RGB(255, 255, 255);
        HPEN pen = CreatePen(PS_SOLID, 1, outline);
        HBRUSH brush = CreateSolidBrush(fill);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, x - radius, cy - radius, x + radius + 1, cy + radius + 1);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        wchar_t label[2] = { marker == PROGRESS_MARKER_A ? L'A' : L'B', 0 };
        RECT labelRect = { x - 8, track.bottom + 3, x + 8, track.bottom + 15 };
        SetTextColor(hdc, overlay ? RGB(235, 235, 235) : RGB(60, 60, 60));
        DrawTextW(hdc, label, -1, &labelRect, DT_CENTER | DT_TOP | DT_SINGLELINE);

        if (hovered)
        {
            std::wstring time = FormatTime(pos);
            RECT timeRect = { x - 34, max(0, track.top - 12), x + 34, track.top - 2 };
            RECT client = { 0 };
            GetClientRect(hwnd, &client);
            if (timeRect.left < client.left + 2)
            {
                OffsetRect(&timeRect, client.left + 2 - timeRect.left, 0);
            }
            if (timeRect.right > client.right - 2)
            {
                OffsetRect(&timeRect, client.right - 2 - timeRect.right, 0);
            }
            SetTextColor(hdc, overlay ? RGB(255, 255, 255) : RGB(20, 20, 20));
            DrawTextW(hdc, time.c_str(), -1, &timeRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    void PlayerWindow::PaintProgressTrack(HDC hdc, HWND hwnd, bool overlay)
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        if (!overlay)
        {
            HBRUSH bg = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);
        }

        RECT track = GetProgressTrackRect(hwnd, overlay);
        const int cy = (track.top + track.bottom) / 2;
        const COLORREF textColor = overlay ? RGB(235, 235, 235) : RGB(45, 45, 45);
        HFONT font = CreateFontW(-11,
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
                                 L"Microsoft YaHei UI");
        HGDIOBJ oldFont = NULL;
        if (font)
        {
            oldFont = SelectObject(hdc, font);
        }
        RECT currentTimeRect = { max(rc.left + 2, track.left - PROGRESS_TIME_LABEL_GAP - PROGRESS_TIME_LABEL_WIDTH),
                                 cy - 6,
                                 track.left - PROGRESS_TIME_LABEL_GAP,
                                 cy + 7 };
        RECT durationRect = { track.right + PROGRESS_TIME_LABEL_GAP,
                              cy - 6,
                              min(rc.right - 2, track.right + PROGRESS_TIME_LABEL_GAP + PROGRESS_TIME_LABEL_WIDTH),
                              cy + 7 };
        SetTextColor(hdc, textColor);
        DrawTextW(hdc, FormatClockTime(uiPosition_).c_str(), -1, &currentTimeRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        DrawTextW(hdc, FormatClockTime(duration_).c_str(), -1, &durationRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        RECT bar = { track.left, cy - 1, track.right, cy + 1 };
        HBRUSH baseBrush = CreateSolidBrush(overlay ? RGB(80, 80, 80) : RGB(188, 188, 188));
        FillRect(hdc, &bar, baseBrush);
        DeleteObject(baseBrush);

        RECT played = bar;
        played.right = TimeToProgressX(uiPosition_, track);
        HBRUSH playedBrush = CreateSolidBrush(overlay ? RGB(235, 235, 235) : RGB(40, 120, 220));
        FillRect(hdc, &played, playedBrush);
        DeleteObject(playedBrush);

        const int thumbX = TimeToProgressX(uiPosition_, track);
        HPEN thumbPen = CreatePen(PS_SOLID, 1, overlay ? RGB(20, 20, 20) : RGB(255, 255, 255));
        HBRUSH thumbBrush = CreateSolidBrush(overlay ? RGB(255, 255, 255) : RGB(40, 120, 220));
        HGDIOBJ oldPen = SelectObject(hdc, thumbPen);
        HGDIOBJ oldBrush = SelectObject(hdc, thumbBrush);
        Ellipse(hdc, thumbX - 3, cy - 3, thumbX + 4, cy + 4);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(thumbBrush);
        DeleteObject(thumbPen);

        PaintProgressMarker(hdc, hwnd, overlay, track, PROGRESS_MARKER_A, loopA_);
        PaintProgressMarker(hdc, hwnd, overlay, track, PROGRESS_MARKER_B, loopB_);
        if (oldFont)
        {
            SelectObject(hdc, oldFont);
        }
        if (font)
        {
            DeleteObject(font);
        }
    }

    void PlayerWindow::PaintOverlay(HDC hdc, HWND hwnd)
    {
        RECT rc = { 0 };
        GetClientRect(hwnd, &rc);

        HBRUSH bg = CreateSolidBrush(RGB(26, 26, 28));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        SetBkMode(hdc, TRANSPARENT);
        PaintProgressTrack(hdc, hwnd, true);

        RECT button = GetOverlayPlayRect(hwnd);
        HBRUSH buttonBrush = CreateSolidBrush(RGB(48, 48, 52));
        HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 125));
        HGDIOBJ oldPen = SelectObject(hdc, buttonPen);
        HGDIOBJ oldBrush = SelectObject(hdc, buttonBrush);
        RoundRect(hdc, button.left, button.top, button.right, button.bottom, 4, 4);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(buttonBrush);
        DeleteObject(buttonPen);

        SetTextColor(hdc, RGB(245, 245, 245));
        HFONT font = CreateFontW(-11,
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
                                 L"Microsoft YaHei UI");
        HGDIOBJ oldFont = NULL;
        if (font)
        {
            oldFont = SelectObject(hdc, font);
        }
        DrawTextW(hdc, isPlaying_ ? L"暂停" : L"播放", -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (oldFont)
        {
            SelectObject(hdc, oldFont);
        }
        if (font)
        {
            DeleteObject(font);
        }
    }

    void PlayerWindow::InvalidateProgressViews()
    {
        if (seekSlider_)
        {
            InvalidateRect(seekSlider_, NULL, TRUE);
        }
        if (overlayPanel_)
        {
            InvalidateRect(overlayPanel_, NULL, TRUE);
        }
    }

    void PlayerWindow::UpdateProgressHover(HWND hwnd, bool overlay, POINT pt)
    {
        const int marker = HitTestProgressMarker(hwnd, overlay, pt);
        if (hoveredMarker_ != marker || hoverMarkerWindow_ != hwnd)
        {
            hoveredMarker_ = marker;
            hoverMarkerWindow_ = marker == PROGRESS_MARKER_NONE ? NULL : hwnd;
            InvalidateProgressViews();
        }

        if (marker != PROGRESS_MARKER_NONE)
        {
            SetFocus(hwnd_);
        }

        TRACKMOUSEEVENT tme = { 0 };
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
    }

    void PlayerWindow::ClearProgressHover(HWND hwnd)
    {
        if (hoverMarkerWindow_ == hwnd)
        {
            hoveredMarker_ = PROGRESS_MARKER_NONE;
            hoverMarkerWindow_ = NULL;
            InvalidateProgressViews();
        }
    }

    void PlayerWindow::MoveCursorToHoveredMarker()
    {
        if (hoveredMarker_ == PROGRESS_MARKER_NONE || !hoverMarkerWindow_)
        {
            return;
        }

        const bool overlay = hoverMarkerWindow_ == overlayPanel_;
        const REFERENCE_TIME markerPos = hoveredMarker_ == PROGRESS_MARKER_A ? loopA_ : loopB_;
        if (markerPos < 0)
        {
            return;
        }

        RECT track = GetProgressTrackRect(hoverMarkerWindow_, overlay);
        POINT pt = { TimeToProgressX(markerPos, track), (track.top + track.bottom) / 2 };
        ClientToScreen(hoverMarkerWindow_, &pt);
        SetCursorPos(pt.x, pt.y);
    }

    bool PlayerWindow::NudgeHoveredMarker(int frames)
    {
        if (IsMarkerSeekStillPending())
        {
            return true;
        }

        bool moved = false;
        if (hoveredMarker_ == PROGRESS_MARKER_A)
        {
            moved = NudgeLoopPoint(true, frames);
        }
        else if (hoveredMarker_ == PROGRESS_MARKER_B)
        {
            moved = NudgeLoopPoint(false, frames);
        }

        if (moved)
        {
            markerSeekPending_ = true;
            markerSeekPendingTick_ = GetTickCount();
            MoveCursorToHoveredMarker();
        }

        return moved;
    }

    void PlayerWindow::ClearNativePlaybackSegmentForSeek()
    {
        if (!player_ || !segmentStopApplied_)
        {
            return;
        }

        IMFPMediaItem* item = NULL;
        HRESULT hr = player_->GetMediaItem(&item);
        if (SUCCEEDED(hr) && item)
        {
            PROPVARIANT startValue;
            PROPVARIANT stopValue;
            InitInt64PropVariant(startValue, 0);
            InitInt64PropVariant(stopValue, duration_);
            hr = item->SetStartStopPosition(&MFP_POSITIONTYPE_100NS,
                                            &startValue,
                                            &MFP_POSITIONTYPE_100NS,
                                            &stopValue);
            PropVariantClear(&startValue);
            PropVariantClear(&stopValue);
            Logf(L"SetStartStopPosition full for seek drag returned 0x%08X, stop=%s (%I64d)",
                 static_cast<unsigned int>(hr),
                 FormatTime(duration_).c_str(),
                 duration_);
            SafeRelease(item);
        }
        else
        {
            Logf(L"GetMediaItem for seek drag segment clear failed: 0x%08X", static_cast<unsigned int>(hr));
        }

        segmentStopApplied_ = false;
    }

    void PlayerWindow::SeekFromProgressPoint(HWND hwnd, bool overlay, int x, bool logSeek)
    {
        if (!hasMedia_ || duration_ <= 0)
        {
            return;
        }

        RECT track = GetProgressTrackRect(hwnd, overlay);
        const REFERENCE_TIME target = ProgressXToTime(x, track);
        SeekTo(target, false, logSeek);
        UpdatePositionUi(target);
    }

    void PlayerWindow::BeginSeekDrag(HWND hwnd, bool overlay, POINT pt)
    {
        if (!hasMedia_ || duration_ <= 0 || !IsPointInProgressArea(hwnd, overlay, pt))
        {
            return;
        }

        draggingSeek_ = true;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        wasPlayingBeforeDrag_ = isPlaying_;
        restoreSegmentAfterDrag_ = segmentStopApplied_;
        SetFocus(hwnd_);
        SetCapture(hwnd);

        if (isPlaying_ && player_)
        {
            manualPauseRequest_ = true;
            HRESULT hr = player_->Pause();
            Logf(L"Seek drag pause returned 0x%08X", static_cast<unsigned int>(hr));
            if (SUCCEEDED(hr))
            {
                isPlaying_ = false;
            }
            else
            {
                manualPauseRequest_ = false;
            }
        }

        ClearNativePlaybackSegmentForSeek();
        SeekFromProgressPoint(hwnd, overlay, pt.x, false);
        UpdateControls();
    }

    void PlayerWindow::ContinueSeekDrag(HWND hwnd, bool overlay, POINT pt)
    {
        if (!draggingSeek_ || GetCapture() != hwnd)
        {
            return;
        }

        SeekFromProgressPoint(hwnd, overlay, pt.x, false);
    }

    void PlayerWindow::EndSeekDrag(HWND hwnd, bool overlay, POINT pt)
    {
        if (!draggingSeek_ || GetCapture() != hwnd)
        {
            return;
        }

        ReleaseCapture();
        draggingSeek_ = false;
        SeekFromProgressPoint(hwnd, overlay, pt.x, true);

        if (restoreSegmentAfterDrag_)
        {
            ApplyPlaybackSegment();
        }
        restoreSegmentAfterDrag_ = false;

        if (wasPlayingBeforeDrag_ && player_)
        {
            HRESULT hr = player_->Play();
            Logf(L"Seek drag resume Play returned 0x%08X", static_cast<unsigned int>(hr));
            isPlaying_ = SUCCEEDED(hr);
            if (isPlaying_)
            {
                SetStatus(L"Playing");
            }
        }
        wasPlayingBeforeDrag_ = false;
        UpdateControls();
    }

    void PlayerWindow::ShowProgressContextMenu(HWND hwnd, bool overlay, POINT pt)
    {
        if (!hasMedia_ || duration_ <= 0 || !IsPointInProgressArea(hwnd, overlay, pt))
        {
            return;
        }

        RECT track = GetProgressTrackRect(hwnd, overlay);
        contextMenuPosition_ = ProgressXToTime(pt.x, track);

        HMENU menu = CreatePopupMenu();
        if (!menu)
        {
            return;
        }

        AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_A, L"设置A点");
        AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_B, L"设置B点");

        POINT screen = pt;
        ClientToScreen(hwnd, &screen);
        const bool lockOverlay = overlay && overlayPanel_;
        HWND menuOwner = lockOverlay ? overlayPanel_ : hwnd_;
        if (lockOverlay)
        {
            progressMenuActive_ = true;
            overlayVisiblePixels_ = FULLSCREEN_OVERLAY_HEIGHT;
            overlayTargetVisible_ = true;
            LayoutOverlay();
            Logf(L"Progress context menu opened with overlay locked: screen=(%ld,%ld), owner=%p",
                 screen.x,
                 screen.y,
                 menuOwner);
        }

        SetForegroundWindow(hwnd_);
        const int command = TrackPopupMenu(menu,
                                           TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                           screen.x,
                                           screen.y,
                                           0,
                                           menuOwner,
                                           NULL);
        PostMessageW(menuOwner, WM_NULL, 0, 0);
        DestroyMenu(menu);

        if (lockOverlay)
        {
            progressMenuActive_ = false;
            Logf(L"Progress context menu closed: command=%d", command);
            UpdateOverlayState();
        }

        if (command == IDM_PROGRESS_SET_A)
        {
            SetLoopAAt(contextMenuPosition_, contextMenuPosition_);
            SeekTo(contextMenuPosition_, true);
            UpdatePositionUi(contextMenuPosition_);
        }
        else if (command == IDM_PROGRESS_SET_B)
        {
            SetLoopBAt(contextMenuPosition_, contextMenuPosition_);
            SeekTo(contextMenuPosition_, true);
            UpdatePositionUi(contextMenuPosition_);
        }
    }

    POINT PlayerWindow::VideoPointToHost(HWND hwnd, POINT pt) const
    {
        ClientToScreen(hwnd, &pt);
        ScreenToClient(videoHost_, &pt);
        return pt;
    }

    void PlayerWindow::ZoomVideoAt(POINT hostPt, int wheelSteps, bool fine)
    {
        if (!hasMedia_ || !videoHost_ || !videoPanel_ || wheelSteps == 0)
        {
            return;
        }

        RECT host = { 0 };
        GetClientRect(videoHost_, &host);
        const int hostW = max(1, host.right - host.left);
        const int hostH = max(1, host.bottom - host.top);
        RECT before = GetVideoRenderRect(hostW, hostH);
        const int beforeW = max(1, before.right - before.left);
        const int beforeH = max(1, before.bottom - before.top);
        const double relX = static_cast<double>(hostPt.x - before.left) / beforeW;
        const double relY = static_cast<double>(hostPt.y - before.top) / beforeH;

        const int oldZoom = videoZoomPercent_;
        const int delta = fine ? 1 : 5;
        int nextZoom = videoZoomPercent_ + wheelSteps * delta;
        if (nextZoom < 10)
        {
            nextZoom = 10;
        }
        if (nextZoom > 500)
        {
            nextZoom = 500;
        }
        if (nextZoom == videoZoomPercent_)
        {
            return;
        }

        int baseW = 0;
        int baseH = 0;
        GetBaseVideoSize(hostW, hostH, baseW, baseH);
        const int nextW = max(1, MulDiv(baseW, nextZoom, 100));
        const int nextH = max(1, MulDiv(baseH, nextZoom, 100));
        const int nextLeft = hostPt.x - static_cast<int>(relX * nextW + 0.5);
        const int nextTop = hostPt.y - static_cast<int>(relY * nextH + 0.5);

        videoZoomPercent_ = nextZoom;
        videoPanX_ = nextLeft - (hostW - nextW) / 2;
        videoPanY_ = nextTop - (hostH - nextH) / 2;
        ApplyVideoTransform();

        wchar_t status[64] = { 0 };
        StringCchPrintfW(status, ARRAYSIZE(status), L"缩放 %d%%", videoZoomPercent_);
        SetStatus(status);
        ShowZoomTip();
        Logf(L"Video zoom changed: %d%% -> %d%%, mouse=(%ld,%ld), fine=%d",
             oldZoom,
             videoZoomPercent_,
             hostPt.x,
             hostPt.y,
             fine ? 1 : 0);
    }

    void PlayerWindow::BeginVideoDrag(HWND hwnd, POINT pt)
    {
        if (!hasMedia_ || !videoHost_)
        {
            return;
        }

        videoMouseDown_ = true;
        draggingVideo_ = false;
        videoDragStart_ = VideoPointToHost(hwnd, pt);
        videoDragLast_ = videoDragStart_;
        SetCapture(hwnd);
    }

    void PlayerWindow::ContinueVideoDrag(HWND hwnd, POINT pt)
    {
        if (!videoMouseDown_ || GetCapture() != hwnd)
        {
            return;
        }

        POINT current = VideoPointToHost(hwnd, pt);
        const int totalDx = current.x - videoDragStart_.x;
        const int totalDy = current.y - videoDragStart_.y;
        if (!draggingVideo_ && (abs(totalDx) >= 4 || abs(totalDy) >= 4))
        {
            draggingVideo_ = true;
            KillTimer(hwnd_, TIMER_VIDEO_CLICK);
            Logf(L"Video drag begin: zoom=%d%%", videoZoomPercent_);
        }

        if (draggingVideo_)
        {
            videoPanX_ += current.x - videoDragLast_.x;
            videoPanY_ += current.y - videoDragLast_.y;
            videoDragLast_ = current;
            ApplyVideoTransform();
        }
    }

    void PlayerWindow::EndVideoDrag(HWND hwnd, POINT pt)
    {
        const bool hadMouseDown = videoMouseDown_;
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }

        if (!hadMouseDown)
        {
            return;
        }

        const bool wasDragging = draggingVideo_;
        if (videoMouseDown_ && wasDragging)
        {
            POINT current = VideoPointToHost(hwnd, pt);
            Logf(L"Video drag end: mouse=(%ld,%ld), pan=(%d,%d), zoom=%d%%",
                 current.x,
                 current.y,
                 videoPanX_,
                 videoPanY_,
                 videoZoomPercent_);
        }

        videoMouseDown_ = false;
        draggingVideo_ = false;
        if (!wasDragging)
        {
            SetTimer(hwnd_, TIMER_VIDEO_CLICK, GetDoubleClickTime() + 30, NULL);
        }
    }

    LRESULT PlayerWindow::VideoWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
        {
            const LRESULT hit = OnNcHitTest(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            if (hit != HTCLIENT)
            {
                return HTTRANSPARENT;
            }
            break;
        }
        case WM_ERASEBKGND:
        {
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect(reinterpret_cast<HDC>(wparam), &rc, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            return 1;
        }
        case WM_PAINT:
        {
            if (!hasMedia_)
            {
                PAINTSTRUCT ps = { 0 };
                HDC hdc = BeginPaint(hwnd, &ps);
                PaintEmptyPlusButton(hdc, hwnd);
                EndPaint(hwnd, &ps);
                return 0;
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            POINT resizePt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (BeginResizeIfNeeded(hwnd, resizePt))
            {
                return 0;
            }
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (!hasMedia_)
            {
                return 0;
            }
            BeginVideoDrag(hwnd, pt);
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            TouchMouseActivity();
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (!hasMedia_)
            {
                const bool hovered = HitTestEmptyPlusButton(hwnd, pt);
                if (emptyPlusHovered_ != hovered)
                {
                    emptyPlusHovered_ = hovered;
                    InvalidateRect(videoHost_, NULL, TRUE);
                    InvalidateRect(videoPanel_, NULL, TRUE);
                }

                TRACKMOUSEEVENT tme = { 0 };
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                if (hovered)
                {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                }
                return 0;
            }
            ContinueVideoDrag(hwnd, pt);
            return 0;
        }
        case WM_MOUSELEAVE:
            if (!hasMedia_ && emptyPlusHovered_)
            {
                emptyPlusHovered_ = false;
                InvalidateRect(videoHost_, NULL, TRUE);
                InvalidateRect(videoPanel_, NULL, TRUE);
            }
            return 0;
        case WM_LBUTTONUP:
        {
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (!hasMedia_)
            {
                if (HitTestEmptyPlusButton(hwnd, pt))
                {
                    OpenFileDialog();
                }
                return 0;
            }
            EndVideoDrag(hwnd, pt);
            return 0;
        }
        case WM_LBUTTONDBLCLK:
            TouchMouseActivity(true);
            KillTimer(hwnd_, TIMER_VIDEO_CLICK);
            videoMouseDown_ = false;
            draggingVideo_ = false;
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();
            }
            ToggleFullScreen();
            return 0;
        case WM_MOUSEWHEEL:
        {
            TouchMouseActivity(true);
            const int wheel = GET_WHEEL_DELTA_WPARAM(wparam);
            int steps = wheel / WHEEL_DELTA;
            if (steps == 0 && wheel != 0)
            {
                steps = wheel > 0 ? 1 : -1;
            }
            POINT screen = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            POINT hostPt = screen;
            ScreenToClient(videoHost_, &hostPt);
            ZoomVideoAt(hostPt, steps, (GET_KEYSTATE_WPARAM(wparam) & MK_CONTROL) != 0);
            return 0;
        }
        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT PlayerWindow::TopOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
        {
            const LRESULT hit = OnNcHitTest(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            if (hit != HTCLIENT)
            {
                return HTTRANSPARENT;
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintTopOverlay(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (topOverlayDragActive_)
            {
                TouchMouseActivity(true);
                UpdateTopOverlayDrag(false);
                return 0;
            }

            TouchMouseActivity();
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            const int button = HitTestTopButton(hwnd, pt);
            if (topHoveredButton_ != button)
            {
                topHoveredButton_ = button;
                InvalidateRect(hwnd, NULL, TRUE);
            }

            TRACKMOUSEEVENT tme = { 0 };
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            return 0;
        }
        case WM_MOUSELEAVE:
        {
            POINT screenPt = { 0 };
            GetCursorPos(&screenPt);
            const bool moved = !haveLastMouseActivityScreen_ ||
                               screenPt.x != lastMouseActivityScreen_.x ||
                               screenPt.y != lastMouseActivityScreen_.y;
            lastMouseActivityScreen_ = screenPt;
            haveLastMouseActivityScreen_ = true;
            if (moved)
            {
                topOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
            }
            topHoveredButton_ = TOP_BUTTON_NONE;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        case WM_LBUTTONDOWN:
        {
            POINT resizePt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (BeginResizeIfNeeded(hwnd, resizePt))
            {
                return 0;
            }
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (HitTestTopButton(hwnd, pt) == TOP_BUTTON_NONE)
            {
                BeginTopOverlayDrag(hwnd, pt);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        {
            if (topOverlayDragActive_)
            {
                EndTopOverlayDrag(true);
                return 0;
            }

            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            const int button = HitTestTopButton(hwnd, pt);
            SetForegroundWindow(hwnd_);
            if (button == TOP_BUTTON_LOAD)
            {
                OpenFileDialog();
            }
            else if (button == TOP_BUTTON_MINIMIZE)
            {
                ShowWindow(hwnd_, SW_MINIMIZE);
            }
            else if (button == TOP_BUTTON_MAXIMIZE)
            {
                ShowWindow(hwnd_, IsZoomed(hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
                LayoutControls();
            }
            else if (button == TOP_BUTTON_CLOSE)
            {
                PostMessageW(hwnd_, WM_CLOSE, 0, 0);
            }
            return 0;
        }
        case WM_CAPTURECHANGED:
            EndTopOverlayDrag(false);
            return 0;
        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT PlayerWindow::ZoomTipWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
        {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintZoomTip(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    LRESULT PlayerWindow::ProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool overlay)
    {
        switch (msg)
        {
        case WM_NCHITTEST:
        {
            const LRESULT hit = OnNcHitTest(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
            if (hit != HTCLIENT)
            {
                return HTTRANSPARENT;
            }
            break;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd, &ps);
            SetBkMode(hdc, TRANSPARENT);
            if (overlay)
            {
                PaintOverlay(hdc, hwnd);
            }
            else
            {
                PaintProgressTrack(hdc, hwnd, false);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            TouchMouseActivity();
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (draggingSeek_ && GetCapture() == hwnd)
            {
                ContinueSeekDrag(hwnd, overlay, pt);
            }
            else
            {
                UpdateProgressHover(hwnd, overlay, pt);
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            ClearProgressHover(hwnd);
            return 0;
        case WM_LBUTTONDOWN:
        {
            POINT resizePt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            if (BeginResizeIfNeeded(hwnd, resizePt))
            {
                return 0;
            }
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            RECT playButton = GetOverlayPlayRect(hwnd);
            if (overlay && PtInRect(&playButton, pt))
            {
                TogglePlayback();
                return 0;
            }
            BeginSeekDrag(hwnd, overlay, pt);
            return 0;
        }
        case WM_LBUTTONUP:
        {
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            EndSeekDrag(hwnd, overlay, pt);
            return 0;
        }
        case WM_RBUTTONUP:
        {
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            ShowProgressContextMenu(hwnd, overlay, pt);
            return 0;
        }
        case WM_CONTEXTMENU:
        {
            TouchMouseActivity(true);
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            ScreenToClient(hwnd, &pt);
            ShowProgressContextMenu(hwnd, overlay, pt);
            return 0;
        }
        case WM_KEYDOWN:
            OnKeyDown(wparam);
            return 0;
        default:
            break;
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }

    void PlayerWindow::SeekTo(REFERENCE_TIME pos, bool keepPlaying, bool logSeek)
    {
        if (!player_)
        {
            return;
        }

        pos = ClampMediaPosition(pos);

        PROPVARIANT value;
        InitInt64PropVariant(value, pos);
        HRESULT hr = player_->SetPosition(MFP_POSITIONTYPE_100NS, &value);
        PropVariantClear(&value);

        if (logSeek)
        {
            Logf(L"SeekTo target=%s (%I64d), hr=0x%08X",
                 FormatTime(pos).c_str(),
                 pos,
                 static_cast<unsigned int>(hr));
        }

        if (keepPlaying && isPlaying_)
        {
            HRESULT playHr = player_->Play();
            if (logSeek)
            {
                Logf(L"SeekTo resume Play returned 0x%08X", static_cast<unsigned int>(playHr));
            }
        }
    }

    void PlayerWindow::LogLoopTrigger() const
    {
        if (!HasValidAbLoop())
        {
            return;
        }

        const REFERENCE_TIME trigger = LoopTriggerPosition();
        const REFERENCE_TIME bFrameEnd = LoopBFrameEnd();
        if (trigger < 0)
        {
            return;
        }

        Logf(L"AB trigger point: trigger=%s (%I64d), B=%s (%I64d), BEnd=%s (%I64d), earlyToEnd=%I64d, earlyFrames=%d",
             FormatTime(trigger).c_str(),
             trigger,
             FormatTime(loopB_).c_str(),
             loopB_,
             FormatTime(bFrameEnd).c_str(),
             bFrameEnd,
             bFrameEnd - trigger,
             LOOP_TRIGGER_EARLY_FRAMES);
    }

    void PlayerWindow::ReplayFrom(REFERENCE_TIME pos, bool forcePauseBeforeSeek)
    {
        if (!player_)
        {
            return;
        }

        lastLoopReplayTick_ = GetTickCount();
        ++loopReplayCount_;
        loopReplayPending_ = IsActiveAbLoop();
        loopReplayFastAttempt_ = loopReplayPending_ && isPlaying_ && IsShortActiveAbLoop() && !forcePauseBeforeSeek;
        const bool logReplay = ShouldLogLoopReplay();
        if (logReplay)
        {
            Logf(L"ReplayFrom #%lu: %s (%I64d), pending=%d, fast=%d, forcePause=%d, loopLength=%s (%I64d)",
                 loopReplayCount_,
                 FormatTime(pos).c_str(),
                 pos,
                 loopReplayPending_ ? 1 : 0,
                 loopReplayFastAttempt_ ? 1 : 0,
                 forcePauseBeforeSeek ? 1 : 0,
                 FormatTime(ActiveLoopLength()).c_str(),
                 ActiveLoopLength());
        }
        suppressReplay_ = false;

        const bool pauseBeforeSeek = isPlaying_ && (forcePauseBeforeSeek || !IsShortActiveAbLoop());
        if (pauseBeforeSeek)
        {
            HRESULT pauseHr = player_->Pause();
            if (logReplay)
            {
                Logf(L"Replay pause-before-seek returned 0x%08X", static_cast<unsigned int>(pauseHr));
            }
        }
        else if (isPlaying_ && logReplay)
        {
            Logf(L"Replay short-loop fast seek without pause");
        }

        SeekTo(pos, false, logReplay);
        HRESULT hr = player_->Play();
        if (logReplay)
        {
            Logf(L"Replay Play returned 0x%08X", static_cast<unsigned int>(hr));
        }
        isPlaying_ = SUCCEEDED(hr);
        if (!isPlaying_)
        {
            loopReplayPending_ = false;
            loopReplayFastAttempt_ = false;
        }
        UpdatePositionUi(pos);
    }

    void PlayerWindow::OnTimer(bool updateUi)
    {
        if (!hasMedia_ || !player_)
        {
            return;
        }

        REFERENCE_TIME pos = 0;
        if (!GetPosition(pos))
        {
            return;
        }

        const DWORD now = GetTickCount();
        if (loopReplayPending_)
        {
            if (!IsActiveAbLoop())
            {
                loopReplayPending_ = false;
                loopReplayFastAttempt_ = false;
            }
            else
            {
                const REFERENCE_TIME pendingTrigger = LoopTriggerPosition();
                if (pendingTrigger >= 0 && pos < pendingTrigger)
                {
                    loopReplayPending_ = false;
                    loopReplayFastAttempt_ = false;
                    lastLoopReplayTick_ = now;
                    if (ShouldLogLoopReplay())
                    {
                        Logf(L"AB replay rearmed by position: pos=%s (%I64d), trigger=%s (%I64d)",
                             FormatTime(pos).c_str(),
                             pos,
                             FormatTime(pendingTrigger).c_str(),
                             pendingTrigger);
                    }
                }
                else if (loopReplayFastAttempt_ && (now - lastLoopReplayTick_) >= LOOP_FAST_REPLAY_VERIFY_MS)
                {
                    Logf(L"AB fast replay did not land before trigger; forcing pause replay. pos=%s (%I64d), trigger=%s (%I64d), elapsed=%lu",
                         FormatTime(pos).c_str(),
                         pos,
                         FormatTime(pendingTrigger).c_str(),
                         pendingTrigger,
                         now - lastLoopReplayTick_);
                    loopReplayFastAttempt_ = false;
                    ReplayFrom(loopA_, true);
                    if (updateUi)
                    {
                        UpdatePositionUi(loopA_);
                    }
                    return;
                }
                else if ((now - lastLoopReplayTick_) < LOOP_REPLAY_PENDING_TIMEOUT_MS)
                {
                    if (updateUi)
                    {
                        UpdatePositionUi(loopA_);
                    }
                    return;
                }
                else
                {
                    loopReplayPending_ = false;
                    loopReplayFastAttempt_ = false;
                    Logf(L"AB replay pending timed out; accepting next trigger. pos=%s (%I64d)",
                         FormatTime(pos).c_str(),
                         pos);
                }
            }
        }

        if (isPlaying_ && IsActiveAbLoop() && (now - lastLoopReplayTick_) >= LOOP_REARM_DELAY_MS)
        {
            const REFERENCE_TIME trigger = LoopTriggerPosition();
            if (trigger >= 0 && pos >= trigger)
            {
                const REFERENCE_TIME bFrameEnd = LoopBFrameEnd();
                if (ShouldLogLoopReplay())
                {
                    Logf(L"AB loop trigger: pos=%s (%I64d), trigger=%s (%I64d), B=%s (%I64d), BEnd=%s (%I64d), earlyToEnd=%I64d",
                         FormatTime(pos).c_str(),
                         pos,
                         FormatTime(trigger).c_str(),
                         trigger,
                         FormatTime(loopB_).c_str(),
                         loopB_,
                         FormatTime(bFrameEnd).c_str(),
                         bFrameEnd,
                         bFrameEnd - trigger);
                }
                ReplayFrom(loopA_);
                pos = loopA_;
            }
        }

        if (updateUi)
        {
            UpdatePositionUi(pos);
        }
    }

    void PlayerWindow::UpdatePositionUi(REFERENCE_TIME pos)
    {
        uiPosition_ = ClampMediaPosition(pos);
        InvalidateProgressViews();
        UpdateTimeText(pos);
    }

    void PlayerWindow::UpdateTimeText(REFERENCE_TIME pos)
    {
        if (hasMedia_)
        {
            pos = SnapToNearestFrame(pos);
        }

        const std::wstring text = FormatTime(pos) + L" / " + FormatTime(duration_);
        SetWindowTextW(timeText_, text.c_str());
    }

    bool PlayerWindow::IsLoopEnabled() const
    {
        return SendMessageW(loopCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    void PlayerWindow::UpdateLoopText()
    {
        std::wstring text = L"A: " + FormatTime(loopA_) + L"   B: " + FormatTime(loopB_);
        if (loopA_ >= 0 && loopB_ > loopA_ && IsLoopEnabled())
        {
            text += L"   ON";
        }
        else if (loopA_ >= 0 || loopB_ >= 0)
        {
            text += L"   OFF/WAIT";
        }

        SetWindowTextW(abText_, text.c_str());
        InvalidateProgressViews();
        UpdateControls();
    }

    void PlayerWindow::UpdateControls()
    {
        EnableWindow(playButton_, hasMedia_);
        EnableWindow(pauseButton_, hasMedia_);
        EnableWindow(stopButton_, hasMedia_);
        EnableWindow(setAButton_, hasMedia_);
        EnableWindow(setBButton_, hasMedia_);
        EnableWindow(aPrevFrameButton_, hasMedia_ && loopA_ >= 0);
        EnableWindow(aNextFrameButton_, hasMedia_ && loopA_ >= 0);
        EnableWindow(bPrevFrameButton_, hasMedia_ && loopB_ >= 0);
        EnableWindow(bNextFrameButton_, hasMedia_ && loopB_ >= 0);
        EnableWindow(clearButton_, hasMedia_);
        EnableWindow(loopCheck_, hasMedia_);
        EnableWindow(seekSlider_, hasMedia_);
        EnableWindow(fullScreenButton_, TRUE);
    }

    void PlayerWindow::SetStatus(const wchar_t* status)
    {
        std::wstring title = kAppTitle;
        if (status && status[0])
        {
            title += L" - ";
            title += status;
        }
        SetWindowTextW(hwnd_, title.c_str());
    }

    void PlayerWindow::UpdateTitle()
    {
        if (filePath_.empty())
        {
            SetWindowTextW(hwnd_, kAppTitle);
            return;
        }

        const wchar_t* name = PathFindFileNameW(filePath_.c_str());
        std::wstring title = kAppTitle;
        title += L" - ";
        title += name;
        SetWindowTextW(hwnd_, title.c_str());
    }

    void PlayerWindow::OnPlayerEvent(MFP_EVENT_TYPE eventType, HRESULT eventHr)
    {
        if (FAILED(eventHr) || ShouldLogPlayerEvent(eventType))
        {
            Logf(L"MFPlay window event: event=%s(%d), hr=0x%08X",
                 MediaPlayerEventName(eventType),
                 static_cast<int>(eventType),
                 static_cast<unsigned int>(eventHr));
        }

        if (FAILED(eventHr))
        {
            isPlaying_ = false;
            manualPauseRequest_ = false;
            SetStatus((std::wstring(L"Playback error: ") + HResultText(eventHr)).c_str());
            UpdateControls();
            return;
        }

        switch (eventType)
        {
        case MFP_EVENT_TYPE_PLAY:
            isPlaying_ = true;
            manualPauseRequest_ = false;
            break;
        case MFP_EVENT_TYPE_PAUSE:
            isPlaying_ = false;
            if (manualPauseRequest_)
            {
                manualPauseRequest_ = false;
                break;
            }
            if (loopReplayPending_)
            {
                break;
            }
            if (IsActiveAbLoop())
            {
                REFERENCE_TIME current = 0;
                const REFERENCE_TIME trigger = LoopTriggerPosition();
                if (trigger >= 0 && GetPosition(current) && current >= trigger)
                {
                    Logf(L"AB loop triggered by PAUSE event: pos=%s (%I64d), trigger=%s (%I64d)",
                         FormatTime(current).c_str(),
                         current,
                         FormatTime(trigger).c_str(),
                         trigger);
                    ReplayFrom(loopA_, true);
                    SetStatus(L"AB looping");
                }
            }
            break;
        case MFP_EVENT_TYPE_STOP:
            isPlaying_ = false;
            manualPauseRequest_ = false;
            break;
        case MFP_EVENT_TYPE_MEDIAITEM_SET:
            ReadDuration();
            ReadNativeVideoSize();
            ResizeWindowToVideoDefault();
            ResizeVideoWindow();
            break;
        case MFP_EVENT_TYPE_POSITION_SET:
            if (loopReplayPending_)
            {
                REFERENCE_TIME current = 0;
                const bool gotPosition = GetPosition(current);
                const REFERENCE_TIME trigger = LoopTriggerPosition();
                if (!loopReplayFastAttempt_ || !gotPosition || trigger < 0 || current < trigger)
                {
                    loopReplayPending_ = false;
                    loopReplayFastAttempt_ = false;
                    lastLoopReplayTick_ = GetTickCount();
                    if (ShouldLogLoopReplay())
                    {
                        Logf(L"AB replay rearmed after POSITION_SET: pos=%s (%I64d), trigger=%s (%I64d)",
                             gotPosition ? FormatTime(current).c_str() : L"--:--.---",
                             gotPosition ? current : -1,
                             FormatTime(trigger).c_str(),
                             trigger);
                    }
                }
                else if (ShouldLogLoopReplay())
                {
                    Logf(L"AB replay POSITION_SET still after trigger; waiting for fallback: pos=%s (%I64d), trigger=%s (%I64d)",
                         FormatTime(current).c_str(),
                         current,
                         FormatTime(trigger).c_str(),
                         trigger);
                }
            }
            if (markerSeekPending_)
            {
                markerSeekPending_ = false;
                if (!IsShortActiveAbLoop() || ShouldLogLoopReplay())
                {
                    Logf(L"Marker nudge seek confirmed by POSITION_SET");
                }
            }
            break;
        case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            if (suppressReplay_)
            {
                suppressReplay_ = false;
                isPlaying_ = false;
                break;
            }
            if (IsActiveAbLoop())
            {
                const DWORD now = GetTickCount();
                if ((now - lastLoopReplayTick_) < LOOP_ENDED_EVENT_SUPPRESS_MS)
                {
                    Logf(L"Ignore PLAYBACK_ENDED after recent AB replay: elapsed=%lu", now - lastLoopReplayTick_);
                    break;
                }
                ReplayFrom(loopA_);
                SetStatus(L"AB looping");
            }
            else
            {
                ReplayFrom(0);
                SetStatus(L"Looping from start");
            }
            break;
        case MFP_EVENT_TYPE_ERROR:
            isPlaying_ = false;
            SetStatus((std::wstring(L"Playback error: ") + HResultText(eventHr)).c_str());
            break;
        default:
            break;
        }

        UpdateControls();
    }

    void PlayerWindow::OnKeyDown(WPARAM key)
    {
        switch (key)
        {
        case VK_F11:
            ToggleFullScreen();
            break;
        case VK_SPACE:
            TogglePlayback();
            break;
        case VK_LEFT:
            if (!NudgeHoveredMarker(-1))
            {
                return;
            }
            break;
        case VK_RIGHT:
            if (!NudgeHoveredMarker(1))
            {
                return;
            }
            break;
        case VK_UP:
            if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                AdjustPlaybackRate(1);
            }
            break;
        case VK_DOWN:
            if (GetKeyState(VK_CONTROL) & 0x8000)
            {
                AdjustPlaybackRate(-1);
            }
            break;
        case 'A':
            SetLoopA();
            break;
        case 'B':
            SetLoopB();
            break;
        case VK_OEM_4:
            NudgeLoopPoint(true, -1);
            break;
        case VK_OEM_6:
            NudgeLoopPoint(true, 1);
            break;
        case VK_OEM_COMMA:
            NudgeLoopPoint(false, -1);
            break;
        case VK_OEM_PERIOD:
            NudgeLoopPoint(false, 1);
            break;
        case VK_ESCAPE:
            if (isFullScreen_)
            {
                ExitFullScreen();
            }
            else
            {
                ClearLoop();
            }
            break;
        default:
            break;
        }
    }

    void PlayerWindow::ClosePlayer()
    {
        Logf(L"ClosePlayer begin");
        if (player_)
        {
            player_->Stop();
            player_->Shutdown();
        }

        SafeRelease(player_);

        hasMedia_ = false;
        isPlaying_ = false;
        emptyPlusHovered_ = false;
        pauseIndicatorAlpha_ = 0;
        pauseIndicatorWasDrawn_ = false;
        HideZoomTip();
        suppressReplay_ = false;
        segmentStopApplied_ = false;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        manualPauseRequest_ = false;
        markerSeekPendingTick_ = 0;
        loopReplayCount_ = 0;
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        ResetVideoTransform();
        duration_ = 0;
        loopA_ = -1;
        loopB_ = -1;
        filePath_.clear();

        uiPosition_ = 0;
        InvalidateProgressViews();
        if (timeText_)
        {
            SetWindowTextW(timeText_, L"--:--.--- / --:--.---");
        }
        if (abText_)
        {
            SetWindowTextW(abText_, L"A: --:--.---   B: --:--.---");
        }
        if (videoHost_)
        {
            InvalidateRect(videoHost_, NULL, TRUE);
        }
        if (videoPanel_)
        {
            InvalidateRect(videoPanel_, NULL, TRUE);
        }
        Logf(L"ClosePlayer end");
    }

}
