#include "PlayerWindow.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"

namespace LoopPlayer
{
    /** 后台seek完成后投递给UI线程的结果
    */
    struct AsyncSeekResult
    {
        DWORD serial;
        REFERENCE_TIME target;
        bool keepPlaying;
        bool logSeek;
        size_t previewMaxReadCount;
        HRESULT seekHr;
        HRESULT playHr;
        DWORD elapsedMs;
    };

    PlayerWindow::PlayerWindow()
        : hinst_(nullptr),
          hwnd_(nullptr),
          videoHost_(nullptr),
          videoPanel_(nullptr),
          openButton_(nullptr),
          playButton_(nullptr),
          pauseButton_(nullptr),
          stopButton_(nullptr),
          setAButton_(nullptr),
          setBButton_(nullptr),
          aPrevFrameButton_(nullptr),
          aNextFrameButton_(nullptr),
          bPrevFrameButton_(nullptr),
          bNextFrameButton_(nullptr),
          clearButton_(nullptr),
          loopCheck_(nullptr),
          fullScreenButton_(nullptr),
          seekSlider_(nullptr),
          timeText_(nullptr),
          abText_(nullptr),
          overlayPanel_(nullptr),
          topOverlayPanel_(nullptr),
          zoomTipPanel_(nullptr),
          topLoadToolTip_(nullptr),
          uiFont_(nullptr),
          player_(nullptr),
          seekWorkerThread_(nullptr),
          seekRequestEvent_(nullptr),
          seekExitEvent_(nullptr),
          hasMedia_(false),
          mediaItemReady_(false),
          autoPlayWhenMediaReady_(false),
          isPlaying_(false),
          suppressReplay_(false),
          segmentStopApplied_(false),
          loopReplayPending_(false),
          loopReplayFastAttempt_(false),
          markerSeekPending_(false),
          manualPauseRequest_(false),
          draggingSeek_(false),
          progressMenuActive_(false),
          asyncSeekPending_(false),
          asyncSeekBusy_(false),
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
          progressOverlayKeepVisibleUntil_(0),
          topOverlayKeepVisibleUntil_(0),
          lastMouseActivityTick_(0),
          zoomTipHideTick_(0),
          loopReplayCount_(0),
          topOverlayDragLastApplyTick_(0),
          lastPositionLogTick_(0),
          lastSeekDragLogTick_(0),
          seekDragLastPreviewTick_(0),
          asyncSeekSerial_(0),
          asyncSeekActiveSerial_(0),
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
          hoverMarkerWindow_(nullptr),
          contextMenuPosition_(0),
          asyncSeekTarget_(0),
          seekDragLastPreviewTarget_(-1),
          seekDragPendingTarget_(-1),
          duration_(0),
          loopA_(-1),
          loopB_(-1),
          frameDuration_(DEFAULT_FRAME_DURATION),
          asyncSeekPreviewMaxReadCount_(0),
          asyncSeekKeepPlaying_(false),
          asyncSeekLog_(false)
    {
        InitializeCriticalSection(&seekWorkerLock_);
        player_ = new MfSourcePlaybackEngine();
        ZeroMemory(&savedPlacement_, sizeof(savedPlacement_));
        savedPlacement_.length = sizeof(savedPlacement_);
        ZeroMemory(&lastMouseActivityScreen_, sizeof(lastMouseActivityScreen_));
        ZeroMemory(&topOverlayDragStartCursor_, sizeof(topOverlayDragStartCursor_));
        ZeroMemory(&topOverlayDragStartWindow_, sizeof(topOverlayDragStartWindow_));
        ZeroMemory(&videoDragStart_, sizeof(videoDragStart_));
        ZeroMemory(&videoDragLast_, sizeof(videoDragLast_));
    }

    PlayerWindow::~PlayerWindow()
    {
        StopSeekWorker();
        ClosePlayer();
        if (player_)
        {
            delete player_;
            player_ = nullptr;
        }
        if (topLoadToolTip_)
        {
            DestroyWindow(topLoadToolTip_);
            topLoadToolTip_ = nullptr;
        }
        if (uiFont_)
        {
            DeleteObject(uiFont_);
            uiFont_ = nullptr;
        }
        DeleteCriticalSection(&seekWorkerLock_);
    }

    void PlayerWindow::GetPrimaryMonitorLayout(MONITORINFO& info) const
    {
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);

        POINT point = { 0, 0 };
        HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
        if (monitor && GetMonitorInfoW(monitor, &info))
        {
            return;
        }

        const int screenW = max(640, GetSystemMetrics(SM_CXSCREEN));
        const int screenH = max(360, GetSystemMetrics(SM_CYSCREEN));
        SetRect(&info.rcMonitor, 0, 0, screenW, screenH);

        RECT workArea = { 0 };
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)
            && workArea.right > workArea.left
            && workArea.bottom > workArea.top)
        {
            info.rcWork = workArea;
        }
        else
        {
            info.rcWork = info.rcMonitor;
        }
    }

    void PlayerWindow::GetWindowMonitorLayout(MONITORINFO& info) const
    {
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);

        HMONITOR monitor = nullptr;
        if (hwnd_)
        {
            monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        }
        if (monitor && GetMonitorInfoW(monitor, &info))
        {
            return;
        }

        GetPrimaryMonitorLayout(info);
    }

    bool PlayerWindow::Create(HINSTANCE hinst, int cmdShow)
    {
        hinst_ = hinst;

        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = PlayerWindow::StaticWndProc;
        wc.hInstance = hinst_;
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = kWindowClass;
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            Logf(L"RegisterClassEx failed: lastError=%lu", GetLastError());
            return false;
        }

        MONITORINFO monitorInfo = { 0 };
        GetPrimaryMonitorLayout(monitorInfo);
        const int monitorW = max(640, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
        const int monitorH = max(360, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
        const int workW = max(320, monitorInfo.rcWork.right - monitorInfo.rcWork.left);
        const int workH = max(240, monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
        const int initialW = max(320, monitorW / 2);
        const int initialH = max(240, monitorH / 2);
        const int initialX = monitorInfo.rcWork.left + (workW - initialW) / 2;
        const int initialY = monitorInfo.rcWork.top + (workH - initialH) / 2;

        hwnd_ = CreateWindowExW(
            WS_EX_APPWINDOW,
            kWindowClass,
            kAppTitle,
            WS_POPUP | WS_CLIPCHILDREN | WS_MINIMIZEBOX,
            initialX,
            initialY,
            initialW,
            initialH,
            nullptr,
            nullptr,
            hinst_,
            this);

        if (!hwnd_)
        {
            Logf(L"CreateWindowEx failed: lastError=%lu", GetLastError());
            return false;
        }

        Logf(L"Initial window layout: monitor=(%ld,%ld)-(%ld,%ld), work=(%ld,%ld)-(%ld,%ld), window=(%d,%d,%d,%d)",
             monitorInfo.rcMonitor.left,
             monitorInfo.rcMonitor.top,
             monitorInfo.rcMonitor.right,
             monitorInfo.rcMonitor.bottom,
             monitorInfo.rcWork.left,
             monitorInfo.rcWork.top,
             monitorInfo.rcWork.right,
             monitorInfo.rcWork.bottom,
             initialX,
             initialY,
             initialW,
             initialH);
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

        Logf(L"LoadFile begin with custom SourceReader engine: %s", path);
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

        VideoTimelineInfo sourceTimeline;
        VideoTimelineProbe::Probe(path, DEFAULT_FRAME_DURATION, sourceTimeline);

        videoTimeline_ = sourceTimeline;
        REFERENCE_TIME videoTimelineOffset = 0;
        NormalizeVideoTimelineForPlayback(videoTimeline_, videoTimelineOffset);

        playbackPath_ = path;
        filePath_ = path;
        Logf(L"LoadFile playback path selected: original=%s, playback=%s, videoOffset=%s (%I64d)",
             path,
             playbackPath_.c_str(),
             FormatTime(videoTimelineOffset).c_str(),
             videoTimelineOffset);

        if (!player_)
        {
            player_ = new MfSourcePlaybackEngine();
        }
        if (!player_)
        {
            return ReportFailure(L"创建播放引擎失败", E_OUTOFMEMORY);
        }

        PlaybackEngineInitParam initParam;
        initParam.m_filePath = playbackPath_;
        initParam.m_videoWindow = videoPanel_;
        initParam.m_eventWindow = hwnd_;
        initParam.m_eventMessage = WM_PLAYBACK_ENGINE_EVENT;
        initParam.m_videoTimeline = sourceTimeline;
        initParam.m_videoTimelineOffset = videoTimelineOffset;
        initParam.m_fallbackFrameDuration = DEFAULT_FRAME_DURATION;

        HRESULT hr = player_->init(initParam);
        if (FAILED(hr))
        {
            std::wstring action = L"播放引擎打开文件失败";
            const std::wstring lastError = player_->lastError();
            if (!lastError.empty())
            {
                action += L"：";
                action += lastError;
            }
            ClosePlayer();
            return ReportFailure(action.c_str(), hr);
        }

        duration_ = 0;
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        ResetVideoTransform();
        ReadDuration();
        DetectFrameDurationFromEngine();
        ApplyVideoTimelineDuration(L"load-file");

        loopA_ = -1;
        loopB_ = -1;
        hasMedia_ = true;
        mediaItemReady_ = true;
        autoPlayWhenMediaReady_ = false;
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
        progressOverlayKeepVisibleUntil_ = 0;
        loopReplayCount_ = 0;
        lastPositionLogTick_ = 0;
        lastSeekDragLogTick_ = 0;
        seekDragLastPreviewTick_ = 0;
        seekDragLastPreviewTarget_ = -1;
        seekDragPendingTarget_ = -1;
        playbackRateTenths_ = 10;

        uiPosition_ = 0;
        InvalidateProgressViews();
        InvalidateRect(videoHost_, nullptr, TRUE);
        InvalidateRect(videoPanel_, nullptr, TRUE);

        UpdateTitle();
        UpdateLoopText();
        UpdateTimeText(0);
        UpdateControls();
        ReadNativeVideoSize();
        ResizeWindowToVideoDefault();
        ResizeVideoWindow();

        SetStatus(L"正在加载");
        ApplyPlaybackRate(false);
        Play();

        UpdateControls();
        Logf(L"LoadFile end: hr=0x%08X, autoPlayWhenMediaReady=%d", static_cast<unsigned int>(hr), autoPlayWhenMediaReady_ ? 1 : 0);
        LogPlaybackSnapshot(L"load-file-end", 0);
        return true;
    }

    LRESULT CALLBACK PlayerWindow::StaticProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        PlayerWindow* self = nullptr;

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
        PlayerWindow* self = nullptr;

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
        PlayerWindow* self = nullptr;

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
        PlayerWindow* self = nullptr;

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
        PlayerWindow* self = nullptr;

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
        PlayerWindow* self = nullptr;

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

    DWORD WINAPI PlayerWindow::StaticSeekWorkerProc(LPVOID param)
    {
        PlayerWindow* self = reinterpret_cast<PlayerWindow*>(param);
        if (!self)
        {
            return 1;
        }

        return self->SeekWorkerProc();
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
        case WM_PLAYBACK_ENGINE_EVENT:
            OnPlaybackEngineEvent(static_cast<PlaybackEngineEvent>(wparam), static_cast<HRESULT>(lparam));
            return 0;
        case WM_ASYNC_SEEK_DONE:
            OnAsyncSeekDone(lparam);
            return 0;
        case WM_CLOSE:
            Logf(L"WM_CLOSE received");
            DestroyWindow(hwnd_);
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
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = kProgressClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticOverlayWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kOverlayClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticTopOverlayWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kTopOverlayClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PlayerWindow::StaticZoomTipWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = kZoomTipClass;
        RegisterClassExW(&wc);

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = PlayerWindow::StaticVideoWndProc;
        wc.hInstance = hinst_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
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
                                     0, 0, 90, 24, hwnd_, reinterpret_cast<HMENU>(IDC_LOOP_AB), hinst_, nullptr);
        SendMessageW(loopCheck_, BM_SETCHECK, BST_CHECKED, 0);
        fullScreenButton_ = CreateButton(L"全屏", IDC_FULLSCREEN);

        seekSlider_ = CreateWindowExW(0, kProgressClass, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      0, 0, 100, NORMAL_PROGRESS_HEIGHT, hwnd_, reinterpret_cast<HMENU>(IDC_SEEK), hinst_, this);

        timeText_ = CreateWindowExW(0, L"STATIC", L"--:--.--- / --:--.---", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                    0, 0, 220, 20, hwnd_, reinterpret_cast<HMENU>(IDC_TIME_TEXT), hinst_, nullptr);
        abText_ = CreateWindowExW(0, L"STATIC", L"A: --:--.---   B: --:--.---", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  0, 0, 320, 20, hwnd_, reinterpret_cast<HMENU>(IDC_AB_TEXT), hinst_, nullptr);
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
                                           nullptr,
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

        uiFont_ = CreateUiFont(-13, FW_NORMAL);
        ApplyControlFont();
        CreateTopLoadToolTip();

        DragAcceptFiles(hwnd_, TRUE);
        SetTimer(hwnd_, TIMER_POSITION, 10, nullptr);

        LayoutControls();
        UpdateControls();
        SetStatus(L"加载或拖入视频文件");
    }

    HWND PlayerWindow::CreateButton(const wchar_t* text, int id)
    {
        return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                               0, 0, 70, 26, hwnd_, reinterpret_cast<HMENU>(id), hinst_, nullptr);
    }

    void PlayerWindow::ApplyControlFont()
    {
        if (!uiFont_)
        {
            return;
        }

        HWND controls[] =
        {
            openButton_,
            playButton_,
            pauseButton_,
            stopButton_,
            setAButton_,
            setBButton_,
            aPrevFrameButton_,
            aNextFrameButton_,
            bPrevFrameButton_,
            bNextFrameButton_,
            clearButton_,
            loopCheck_,
            fullScreenButton_,
            timeText_,
            abText_
        };

        for (size_t i = 0; i < ARRAYSIZE(controls); ++i)
        {
            if (controls[i])
            {
                SendMessageW(controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(uiFont_), TRUE);
            }
        }
    }

    HFONT PlayerWindow::CreateUiFont(int height, int weight) const
    {
        return CreateFontW(height,
                           0,
                           0,
                           0,
                           weight,
                           FALSE,
                           FALSE,
                           FALSE,
                           DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE,
                           L"楷体");
    }

    void PlayerWindow::CreateTopLoadToolTip()
    {
        if (!topOverlayPanel_ || topLoadToolTip_)
        {
            return;
        }

        topLoadToolTip_ = CreateWindowExW(WS_EX_TOPMOST,
                                          TOOLTIPS_CLASS,
                                          nullptr,
                                          WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          topOverlayPanel_,
                                          nullptr,
                                          hinst_,
                                          nullptr);
        if (!topLoadToolTip_)
        {
            Logf(L"Create top load tooltip failed: lastError=%lu", GetLastError());
            return;
        }

        TOOLINFOW toolInfo = { 0 };
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.uFlags = TTF_SUBCLASS;
        toolInfo.hwnd = topOverlayPanel_;
        toolInfo.uId = TOP_BUTTON_LOAD;
        toolInfo.lpszText = const_cast<LPWSTR>(L"加载视频");
        toolInfo.rect = GetTopButtonRect(topOverlayPanel_, TOP_BUTTON_LOAD);
        SendMessageW(topLoadToolTip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&toolInfo));
        SetWindowPos(topLoadToolTip_, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        Logf(L"Top load tooltip created");
    }

    void PlayerWindow::UpdateTopLoadToolTipRect()
    {
        if (!topLoadToolTip_ || !topOverlayPanel_)
        {
            return;
        }

        TOOLINFOW toolInfo = { 0 };
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.hwnd = topOverlayPanel_;
        toolInfo.uId = TOP_BUTTON_LOAD;
        toolInfo.rect = GetTopButtonRect(topOverlayPanel_, TOP_BUTTON_LOAD);
        SendMessageW(topLoadToolTip_, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&toolInfo));
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
        UpdateTopLoadToolTipRect();
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
            rect.right = 28;
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

        if (button == TOP_BUTTON_LOAD)
        {
            RECT icon = rect;
            InflateRect(&icon, -4, -4);
            HPEN folderPen = CreatePen(PS_SOLID, 1, RGB(238, 238, 238));
            HBRUSH folderBrush = CreateSolidBrush(hovered ? RGB(88, 88, 94) : RGB(70, 70, 76));
            oldPen = SelectObject(hdc, folderPen);
            oldBrush = SelectObject(hdc, folderBrush);
            POINT points[6] =
            {
                { icon.left, icon.top + 4 },
                { icon.left + 5, icon.top + 4 },
                { icon.left + 7, icon.top + 2 },
                { icon.right, icon.top + 2 },
                { icon.right, icon.bottom },
                { icon.left, icon.bottom }
            };
            Polygon(hdc, points, ARRAYSIZE(points));

            HBRUSH playBrush = CreateSolidBrush(RGB(245, 245, 245));
            SelectObject(hdc, playBrush);
            POINT playPoints[3] =
            {
                { icon.left + 6, icon.top + 6 },
                { icon.left + 6, icon.bottom - 3 },
                { icon.right - 4, icon.top + (icon.bottom - icon.top) / 2 }
            };
            Polygon(hdc, playPoints, ARRAYSIZE(playPoints));
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(playBrush);
            DeleteObject(folderBrush);
            DeleteObject(folderPen);
            return;
        }

        SetTextColor(hdc, RGB(245, 245, 245));
        HPEN iconPen = CreatePen(PS_SOLID, 1, RGB(245, 245, 245));
        oldPen = SelectObject(hdc, iconPen);
        HGDIOBJ oldIconBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
        const int cx = (rect.left + rect.right) / 2;
        const int cy = (rect.top + rect.bottom) / 2;
        if (button == TOP_BUTTON_MINIMIZE)
        {
            MoveToEx(hdc, cx - 4, cy + 3, nullptr);
            LineTo(hdc, cx + 5, cy + 3);
        }
        else if (button == TOP_BUTTON_MAXIMIZE)
        {
            if (IsZoomed(hwnd_) || isFullScreen_)
            {
                POINT backRect[5] =
                {
                    { cx - 5, cy - 2 },
                    { cx + 3, cy - 2 },
                    { cx + 3, cy + 6 },
                    { cx - 5, cy + 6 },
                    { cx - 5, cy - 2 }
                };
                POINT frontRect[5] =
                {
                    { cx - 2, cy - 5 },
                    { cx + 6, cy - 5 },
                    { cx + 6, cy + 3 },
                    { cx - 2, cy + 3 },
                    { cx - 2, cy - 5 }
                };
                Polyline(hdc, backRect, ARRAYSIZE(backRect));
                Polyline(hdc, frontRect, ARRAYSIZE(frontRect));
            }
            else
            {
                POINT maxRect[5] =
                {
                    { cx - 4, cy - 4 },
                    { cx + 5, cy - 4 },
                    { cx + 5, cy + 5 },
                    { cx - 4, cy + 5 },
                    { cx - 4, cy - 4 }
                };
                Polyline(hdc, maxRect, ARRAYSIZE(maxRect));
            }
        }
        else if (button == TOP_BUTTON_CLOSE)
        {
            MoveToEx(hdc, cx - 4, cy - 4, nullptr);
            LineTo(hdc, cx + 5, cy + 5);
            MoveToEx(hdc, cx + 4, cy - 4, nullptr);
            LineTo(hdc, cx - 5, cy + 5);
        }
        SelectObject(hdc, oldIconBrush);
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
        SetTimer(hwnd_, TIMER_TITLE_DRAG, 15, nullptr);
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
                     nullptr,
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
        MoveToEx(hdc, cx - half, cy, nullptr);
        LineTo(hdc, cx + half + 1, cy);
        MoveToEx(hdc, cx, cy - half, nullptr);
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
        while ((owner = GetWindow(owner, GW_OWNER)) != nullptr)
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

    void PlayerWindow::PaintPauseIndicator(HDC hdc, HWND hwnd, int alpha)
    {
        if (!hdc || !hwnd || alpha <= 0)
        {
            return;
        }

        RECT panelRect = { 0 };
        GetClientRect(hwnd, &panelRect);
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

        void* bits = nullptr;
        HDC screenDc = GetDC(nullptr);
        HBITMAP bitmap = CreateDIBSection(screenDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HDC memoryDc = CreateCompatibleDC(screenDc);
        ReleaseDC(nullptr, screenDc);
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

        BLENDFUNCTION blend = { 0 };
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = static_cast<BYTE>(alpha);
        blend.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(hdc, left, top, size, size, memoryDc, 0, 0, size, size, blend);

        SelectObject(memoryDc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDc);
        pauseIndicatorWasDrawn_ = true;
    }

    void PlayerWindow::UpdatePauseIndicatorState()
    {
        const DWORD now = GetTickCount();
        const bool targetVisible = !openFileDialogActive_ && hasMedia_ && player_ && player_->isOpen() && !isPlaying_ && IsMouseUiActive(now);
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

        if (!player_ || !player_->isOpen())
        {
            pauseIndicatorAlpha_ = 0;
            pauseIndicatorWasDrawn_ = false;
            return;
        }

        if (pauseIndicatorAlpha_ > 0)
        {
            InvalidateRect(videoPanel_, nullptr, FALSE);
            pauseIndicatorWasDrawn_ = true;
        }
        else if (pauseIndicatorWasDrawn_)
        {
            InvalidateRect(videoPanel_, nullptr, FALSE);
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
        InvalidateRect(zoomTipPanel_, nullptr, TRUE);
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
                                 L"楷体");
        HGDIOBJ oldFont = nullptr;
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
        const DWORD now = GetTickCount();
        const HWND cursorWindow = WindowFromPoint(screenPt);
        const HWND foregroundWindow = GetForegroundWindow();
        const bool cursorOverThisInstance = IsWindowFromThisInstance(cursorWindow);
        const bool foregroundThisInstance = IsWindowFromThisInstance(foregroundWindow);
        const bool keepVisible = progressOverlayKeepVisibleUntil_ != 0 &&
                                 static_cast<DWORD>(progressOverlayKeepVisibleUntil_ - now) < 0x80000000;
        if (!keepVisible)
        {
            progressOverlayKeepVisibleUntil_ = 0;
        }

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
                                    keepVisible ||
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
        InvalidateRect(videoHost_, nullptr, TRUE);

        if (player_ && player_->isOpen())
        {
            InvalidateRect(videoPanel_, nullptr, FALSE);
        }
    }

    bool PlayerWindow::ReadNativeVideoSize()
    {
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        if (!player_ || !player_->isOpen())
        {
            return false;
        }

        int videoWidth = 0;
        int videoHeight = 0;
        const bool ok = player_->getNativeVideoSize(videoWidth, videoHeight);
        if (ok)
        {
            nativeVideoWidth_ = videoWidth;
            nativeVideoHeight_ = videoHeight;
        }

        Logf(L"GetNativeVideoSize from custom engine: ok=%d, video=%dx%d, stored=%dx%d",
             ok ? 1 : 0,
             videoWidth,
             videoHeight,
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

        MONITORINFO mi = { 0 };
        GetWindowMonitorLayout(mi);
        const int monitorW = max(320, mi.rcMonitor.right - mi.rcMonitor.left);
        const int monitorH = max(240, mi.rcMonitor.bottom - mi.rcMonitor.top);
        const int workW = max(320, mi.rcWork.right - mi.rcWork.left);
        const int workH = max(240, mi.rcWork.bottom - mi.rcWork.top);
        const int halfMonitorW = max(320, monitorW / 2);
        const int halfMonitorH = max(240, monitorH / 2);

        double scale = 1.0;
        if (nativeVideoWidth_ > halfMonitorW || nativeVideoHeight_ > halfMonitorH)
        {
            scale = 0.5;
        }

        int targetW = max(1, static_cast<int>(nativeVideoWidth_ * scale + 0.5));
        int targetH = max(1, static_cast<int>(nativeVideoHeight_ * scale + 0.5));
        if (targetW > monitorW || targetH > monitorH)
        {
            const double sx = static_cast<double>(monitorW) / targetW;
            const double sy = static_cast<double>(monitorH) / targetH;
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
                     nullptr,
                     x,
                     y,
                     targetW,
                     targetH,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
        ResetVideoTransform();
        LayoutControls();
        Logf(L"Window resized for video: native=%dx%d, monitor=%dx%d, work=%dx%d, halfMonitor=%dx%d, scale=%0.3f, window=(%d,%d,%d,%d)",
             nativeVideoWidth_,
             nativeVideoHeight_,
             monitorW,
             monitorH,
             workW,
             workH,
             halfMonitorW,
             halfMonitorH,
             scale,
             x,
             y,
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
        if (GetParent(hwnd_) != nullptr || (GetWindowLongPtrW(hwnd_, GWL_STYLE) & WS_CHILD))
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

        MONITORINFO mi = { 0 };
        GetWindowMonitorLayout(mi);

        isFullScreen_ = true;
        overlayVisiblePixels_ = 0;
        overlayTargetVisible_ = false;
        progressMenuActive_ = false;
        progressOverlayKeepVisibleUntil_ = 0;
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
        progressOverlayKeepVisibleUntil_ = 0;
        ShowWindow(overlayPanel_, SW_HIDE);
        SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
        SetWindowPlacement(hwnd_, &savedPlacement_);
        SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
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
            SetTimer(hwnd_, TIMER_VIDEO_CLICK, GetDoubleClickTime() + 30, nullptr);
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
        if (!player_ || !player_->isOpen())
        {
            return;
        }

        if (!mediaItemReady_)
        {
            autoPlayWhenMediaReady_ = true;
            Logf(L"Play requested before MEDIAITEM_SET; pending autoplay enabled");
            SetStatus(L"正在加载");
            UpdateControls();
            return;
        }

        if (UpdateAsyncSeekPlaybackIntent(true, L"播放请求"))
        {
            SetStatus(L"定位后播放");
            UpdateControls();
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

        HRESULT hr = player_->play();
        if (SUCCEEDED(hr))
        {
            isPlaying_ = true;
            manualPauseRequest_ = false;
            SetStatus(L"正在播放");
            Logf(L"Play succeeded");
        }
        else
        {
            Logf(L"Play failed: 0x%08X", static_cast<unsigned int>(hr));
            SetStatus((L"播放失败：" + HResultText(hr)).c_str());
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
        if (!player_ || !player_->isOpen())
        {
            return false;
        }

        const float rate = PlaybackRate();
        HRESULT hr = player_->setRate(rate);
        wchar_t rateText[32] = { 0 };
        FormatPlaybackRate(rateText, ARRAYSIZE(rateText));
        Logf(L"Set playback rate %s returned 0x%08X", rateText, static_cast<unsigned int>(hr));
        if (SUCCEEDED(hr))
        {
            if (showStatus)
            {
                std::wstring status = L"倍速 ";
                status += rateText;
                SetStatus(status.c_str());
            }
            return true;
        }

        if (showStatus)
        {
            std::wstring status = L"倍速调整失败：";
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
            std::wstring status = L"倍速 ";
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

        if (!mediaItemReady_)
        {
            autoPlayWhenMediaReady_ = false;
            isPlaying_ = false;
            Logf(L"Pause requested before MEDIAITEM_SET; pending autoplay canceled");
            SetStatus(L"已暂停");
            UpdateControls();
            return;
        }

        if (UpdateAsyncSeekPlaybackIntent(false, L"暂停请求"))
        {
            manualPauseRequest_ = true;
            isPlaying_ = false;
            if (player_->state() == PlaybackEngineStatePlaying)
            {
                player_->pause();
            }
            SetStatus(L"已暂停");
            UpdateControls();
            return;
        }

        manualPauseRequest_ = true;
        HRESULT hr = player_->pause();
        if (SUCCEEDED(hr))
        {
            isPlaying_ = false;
            loopReplayPending_ = false;
            loopReplayFastAttempt_ = false;
            markerSeekPending_ = false;
            SetStatus(L"已暂停");
            Logf(L"Pause succeeded");
        }
        else
        {
            manualPauseRequest_ = false;
            Logf(L"Pause failed: 0x%08X", static_cast<unsigned int>(hr));
            SetStatus((L"暂停失败：" + HResultText(hr)).c_str());
        }

        UpdateControls();
    }

    void PlayerWindow::Stop()
    {
        if (!player_ || !player_->isOpen())
        {
            return;
        }

        if (!mediaItemReady_)
        {
            autoPlayWhenMediaReady_ = false;
            suppressReplay_ = true;
            loopReplayPending_ = false;
            loopReplayFastAttempt_ = false;
            markerSeekPending_ = false;
            manualPauseRequest_ = false;
            isPlaying_ = false;
            Logf(L"Stop requested before MEDIAITEM_SET; pending autoplay canceled");
            UpdatePositionUi(0);
            SetStatus(L"已停止");
            UpdateControls();
            return;
        }

        suppressReplay_ = true;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        manualPauseRequest_ = false;
        HRESULT hr = player_->stop();
        Logf(L"Stop returned 0x%08X", static_cast<unsigned int>(hr));
        isPlaying_ = false;
        UpdatePositionUi(0);
        SetStatus(L"已停止");
        UpdateControls();
    }

    bool PlayerWindow::ReadDuration()
    {
        duration_ = 0;
        if (!player_ || !player_->isOpen())
        {
            return false;
        }

        duration_ = player_->duration();
        const bool ok = duration_ > 0;
        Logf(L"ReadDuration from custom engine: ok=%d, duration=%s (%I64d)",
             ok ? 1 : 0,
             FormatTime(duration_).c_str(),
             duration_);
        return ok;
    }

    void PlayerWindow::DetectFrameDurationFromEngine()
    {
        frameDuration_ = DEFAULT_FRAME_DURATION;
        if (player_ && player_->isOpen() && player_->frameDuration() > 0)
        {
            frameDuration_ = player_->frameDuration();
        }
        else if (videoTimeline_.isValid && videoTimeline_.secondTime > videoTimeline_.firstTime)
        {
            frameDuration_ = videoTimeline_.secondTime - videoTimeline_.firstTime;
        }
        else if (videoTimeline_.isValid && videoTimeline_.firstDuration > 0)
        {
            frameDuration_ = videoTimeline_.firstDuration;
        }

        Logf(L"Frame duration selected from custom engine/timeline: frameDuration=%s (%I64d), timelineValid=%d, first=%s (%I64d), second=%s (%I64d), firstDuration=%s (%I64d)",
             FormatTime(frameDuration_).c_str(),
             frameDuration_,
             videoTimeline_.isValid ? 1 : 0,
             FormatTime(videoTimeline_.firstTime).c_str(),
             videoTimeline_.firstTime,
             FormatTime(videoTimeline_.secondTime).c_str(),
             videoTimeline_.secondTime,
             FormatTime(videoTimeline_.firstDuration).c_str(),
             videoTimeline_.firstDuration);
    }

    void PlayerWindow::NormalizeVideoTimelineForPlayback(VideoTimelineInfo& info, REFERENCE_TIME& videoOffset) const
    {
        videoOffset = 0;
        if (!info.isValid || info.firstTime <= ONE_SECOND)
        {
            Logf(L"Video timeline offset not needed: valid=%d, first=%s (%I64d)",
                 info.isValid ? 1 : 0,
                 FormatTime(info.firstTime).c_str(),
                 info.firstTime);
            return;
        }

        videoOffset = info.firstTime;
        const REFERENCE_TIME oldFirst = info.firstTime;
        const REFERENCE_TIME oldSecond = info.secondTime;
        const REFERENCE_TIME oldLast = info.lastTime;
        const REFERENCE_TIME oldLastEnd = info.lastEndTime;

        // SourceReader自定义管线直接在播放时映射视频时间戳，所以这里仅修正UI和AB逻辑看到的时间轴。
        info.firstTime = 0;
        if (info.secondTime >= videoOffset)
        {
            info.secondTime -= videoOffset;
        }
        if (info.lastTime >= videoOffset)
        {
            info.lastTime -= videoOffset;
        }
        if (info.lastEndTime >= videoOffset)
        {
            info.lastEndTime -= videoOffset;
        }
        if (info.maxGapStart >= videoOffset)
        {
            info.maxGapStart -= videoOffset;
        }
        if (info.maxGapEnd >= videoOffset)
        {
            info.maxGapEnd -= videoOffset;
        }

        Logf(L"Video timeline normalized for playback: offset=%s (%I64d), first %s->%s, second %s->%s, last %s->%s, lastEnd %s->%s",
             FormatTime(videoOffset).c_str(),
             videoOffset,
             FormatTime(oldFirst).c_str(),
             FormatTime(info.firstTime).c_str(),
             FormatTime(oldSecond).c_str(),
             FormatTime(info.secondTime).c_str(),
             FormatTime(oldLast).c_str(),
             FormatTime(info.lastTime).c_str(),
             FormatTime(oldLastEnd).c_str(),
             FormatTime(info.lastEndTime).c_str());
    }

    void PlayerWindow::ApplyVideoTimelineDuration(const wchar_t* reason)
    {
        if (!videoTimeline_.isValid)
        {
            Logf(L"Video timeline duration not applied: reason=%s, valid=0, currentDuration=%s (%I64d)",
                 reason ? reason : L"",
                 FormatTime(duration_).c_str(),
                 duration_);
            return;
        }

        const REFERENCE_TIME oldDuration = duration_;
        const REFERENCE_TIME videoEnd = videoTimeline_.lastEndTime;
        const REFERENCE_TIME tolerance = LoopMinLength();
        const REFERENCE_TIME diff = videoEnd - oldDuration;
        Logf(L"Video timeline duration compare: reason=%s, currentDuration=%s (%I64d), videoEnd=%s (%I64d), diff=%s (%I64d), tolerance=%s (%I64d), samples=%lu, first=%s (%I64d), second=%s (%I64d), firstDuration=%s (%I64d), maxGap=%s (%I64d)",
             reason ? reason : L"",
             FormatTime(oldDuration).c_str(),
             oldDuration,
             FormatTime(videoEnd).c_str(),
             videoEnd,
             FormatTime(diff).c_str(),
             diff,
             FormatTime(tolerance).c_str(),
             tolerance,
             videoTimeline_.sampleCount,
             FormatTime(videoTimeline_.firstTime).c_str(),
             videoTimeline_.firstTime,
             FormatTime(videoTimeline_.secondTime).c_str(),
             videoTimeline_.secondTime,
             FormatTime(videoTimeline_.firstDuration).c_str(),
             videoTimeline_.firstDuration,
             FormatTime(videoTimeline_.maxGap).c_str(),
             videoTimeline_.maxGap);

        if (videoEnd > oldDuration + tolerance)
        {
            duration_ = videoEnd;
            Logf(L"Duration expanded from source duration to video timeline end: old=%s (%I64d), new=%s (%I64d)",
                 FormatTime(oldDuration).c_str(),
                 oldDuration,
                 FormatTime(duration_).c_str(),
                 duration_);
        }
    }

    bool PlayerWindow::GetPosition(REFERENCE_TIME& pos)
    {
        pos = 0;
        if (!player_ || !player_->isOpen())
        {
            return false;
        }

        const bool ok = player_->getPosition(pos);
        if (ok)
        {
            pos = ClampMediaPosition(pos);
        }
        else
        {
            Logf(L"GetPosition failed from custom engine");
        }
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
        if (!player_ || !player_->isOpen())
        {
            return false;
        }

        const bool activeAb = IsActiveAbLoop();
        if (!activeAb && !hadSegment)
        {
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
            return false;
        }

        Logf(L"Playback segment updated for custom engine: mode=%s, start=%s (%I64d), stop=%s (%I64d), manualLoop=%d",
             activeAb ? L"AB" : L"full",
             FormatTime(start).c_str(),
             start,
             FormatTime(stop).c_str(),
             stop,
             activeAb ? 1 : 0);
        return activeAb;
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
        SetStatus(L"A点已设置");
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
            SetStatus(L"B点必须在A点之后");
            return;
        }

        loopB_ = pos;
        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        markerSeekPending_ = false;
        loopReplayCount_ = 0;
        UpdateLoopText();
        ApplyPlaybackSegment();
        SetStatus(L"B点已设置");
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
        SetStatus(L"AB循环已清除");
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
            SetStatus(isA ? L"请先设置A点" : L"请先设置B点");
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
                SetStatus(L"B点没有可移动空间");
                return false;
            }
            loopB_ = next;
        }

        loopReplayPending_ = false;
        loopReplayFastAttempt_ = false;
        UpdateLoopText();
        ApplyPlaybackSegment();

        if (previewFrame && isPlaying_ && player_ && player_->isOpen())
        {
            manualPauseRequest_ = true;
            HRESULT pauseHr = player_->pause();
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

        SeekTo(next, !previewFrame && isPlaying_);
        UpdatePositionUi(next);
        SetStatus(isA ? L"A点移动一帧" : L"B点移动一帧");
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

        const int buttonW = 38;
        const int buttonH = 24;
        RECT button = { 0 };
        button.left = (rc.right - buttonW) / 2;
        button.top = rc.bottom - buttonH - 4;
        if (button.top < 17)
        {
            button.top = 17;
        }
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

    REFERENCE_TIME PlayerWindow::ProgressPointToTime(HWND hwnd, bool overlay, int x) const
    {
        RECT track = GetProgressTrackRect(hwnd, overlay);
        return SnapToNearestFrame(ProgressXToTime(x, track));
    }

    bool PlayerWindow::IsPointInProgressArea(HWND hwnd, bool overlay, POINT pt) const
    {
        RECT track = GetProgressTrackRect(hwnd, overlay);
        RECT area = track;
        InflateRect(&area, 8, overlay ? 13 : 15);
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
                                 L"楷体");
        HGDIOBJ oldFont = nullptr;
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
        POINT cursor = { 0 };
        GetCursorPos(&cursor);
        ScreenToClient(hwnd, &cursor);
        const bool hovered = PtInRect(&button, cursor) != FALSE;
        HBRUSH buttonBrush = CreateSolidBrush(hovered ? RGB(66, 66, 70) : RGB(48, 48, 52));
        HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 125));
        HGDIOBJ oldPen = SelectObject(hdc, buttonPen);
        HGDIOBJ oldBrush = SelectObject(hdc, buttonBrush);
        RoundRect(hdc, button.left, button.top, button.right, button.bottom, 6, 6);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(buttonBrush);
        DeleteObject(buttonPen);

        HPEN iconPen = CreatePen(PS_SOLID, 1, RGB(245, 245, 245));
        HBRUSH iconBrush = CreateSolidBrush(RGB(245, 245, 245));
        oldPen = SelectObject(hdc, iconPen);
        oldBrush = SelectObject(hdc, iconBrush);
        const int cx = (button.left + button.right) / 2;
        const int cy = (button.top + button.bottom) / 2;
        if (isPlaying_)
        {
            RECT leftBar = { cx - 7, cy - 7, cx - 3, cy + 8 };
            RECT rightBar = { cx + 3, cy - 7, cx + 7, cy + 8 };
            FillRect(hdc, &leftBar, iconBrush);
            FillRect(hdc, &rightBar, iconBrush);
        }
        else
        {
            POINT points[3] =
            {
                { cx - 5, cy - 8 },
                { cx - 5, cy + 8 },
                { cx + 8, cy }
            };
            Polygon(hdc, points, ARRAYSIZE(points));
        }
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(iconBrush);
        DeleteObject(iconPen);
    }

    void PlayerWindow::InvalidateProgressViews()
    {
        if (seekSlider_)
        {
            InvalidateRect(seekSlider_, nullptr, TRUE);
        }
        if (overlayPanel_)
        {
            InvalidateRect(overlayPanel_, nullptr, TRUE);
        }
    }

    void PlayerWindow::UpdateProgressHover(HWND hwnd, bool overlay, POINT pt)
    {
        const int marker = HitTestProgressMarker(hwnd, overlay, pt);
        if (hoveredMarker_ != marker || hoverMarkerWindow_ != hwnd)
        {
            hoveredMarker_ = marker;
            hoverMarkerWindow_ = marker == PROGRESS_MARKER_NONE ? nullptr : hwnd;
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
            hoverMarkerWindow_ = nullptr;
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
        if (!segmentStopApplied_)
        {
            return;
        }

        Logf(L"Clear playback segment flag for custom engine seek drag: duration=%s (%I64d)",
             FormatTime(duration_).c_str(),
             duration_);
        segmentStopApplied_ = false;
    }

    void PlayerWindow::SeekFromProgressPoint(HWND hwnd, bool overlay, int x, bool logSeek)
    {
        if (!hasMedia_ || duration_ <= 0)
        {
            return;
        }

        RECT track = GetProgressTrackRect(hwnd, overlay);
        const REFERENCE_TIME target = ProgressPointToTime(hwnd, overlay, x);
        if (logSeek)
        {
            Logf(L"SeekFromProgressPoint overlay=%d x=%d track=(%ld,%ld)-(%ld,%ld) target=%s (%I64d) duration=%s (%I64d)",
                 overlay ? 1 : 0,
                 x,
                 track.left,
                 track.top,
                 track.right,
                 track.bottom,
                 FormatTime(target).c_str(),
                 target,
                 FormatTime(duration_).c_str(),
                 duration_);
        }
        QueueAsyncSeek(target, isPlaying_, logSeek, SEEK_FINAL_PREVIEW_MAX_READ_COUNT, L"进度条单击定位");
        UpdatePositionUi(target);
    }

    void PlayerWindow::PreviewSeekDragTarget(REFERENCE_TIME target, bool force)
    {
        if (!player_ || !player_->isOpen() || !mediaItemReady_)
        {
            return;
        }

        target = SnapToNearestFrame(target);
        seekDragPendingTarget_ = target;
        UpdatePositionUi(target);

        const DWORD now = GetTickCount();
        if (!force)
        {
            if (seekDragLastPreviewTick_ != 0 && (now - seekDragLastPreviewTick_) < SEEK_DRAG_PREVIEW_INTERVAL_MS)
            {
                return;
            }
            if (seekDragLastPreviewTarget_ == target)
            {
                return;
            }
        }

        QueueAsyncSeek(target, false, false, SEEK_DRAG_PREVIEW_MAX_READ_COUNT, L"进度条拖动预览");
        seekDragLastPreviewTick_ = GetTickCount();
        seekDragLastPreviewTarget_ = target;
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
        lastSeekDragLogTick_ = GetTickCount();
        seekDragLastPreviewTick_ = 0;
        seekDragLastPreviewTarget_ = -1;
        seekDragPendingTarget_ = ProgressPointToTime(hwnd, overlay, pt.x);
        progressOverlayKeepVisibleUntil_ = 0;
        overlayVisiblePixels_ = FULLSCREEN_OVERLAY_HEIGHT;
        overlayTargetVisible_ = true;
        LayoutOverlay();
        Logf(L"Seek drag begin overlay=%d pt=(%ld,%ld) wasPlaying=%d restoreSegment=%d duration=%s (%I64d)",
             overlay ? 1 : 0,
             pt.x,
             pt.y,
             wasPlayingBeforeDrag_ ? 1 : 0,
             restoreSegmentAfterDrag_ ? 1 : 0,
             FormatTime(duration_).c_str(),
             duration_);
        LogPlaybackSnapshot(L"seek-drag-begin");

        if (isPlaying_ && player_ && player_->isOpen())
        {
            HRESULT hr = player_->pause();
            Logf(L"Seek drag pause returned 0x%08X", static_cast<unsigned int>(hr));
            if (SUCCEEDED(hr))
            {
                isPlaying_ = false;
            }
        }

        ClearNativePlaybackSegmentForSeek();
        isPlaying_ = false;
        PreviewSeekDragTarget(seekDragPendingTarget_, true);
        UpdateControls();
    }

    void PlayerWindow::ContinueSeekDrag(HWND hwnd, bool overlay, POINT pt)
    {
        if (!draggingSeek_ || GetCapture() != hwnd)
        {
            return;
        }

        const REFERENCE_TIME target = ProgressPointToTime(hwnd, overlay, pt.x);
        seekDragPendingTarget_ = target;
        PreviewSeekDragTarget(target, false);
        const DWORD now = GetTickCount();
        if (IsLoggingEnabled() && (now - lastSeekDragLogTick_) >= 200)
        {
            lastSeekDragLogTick_ = now;
            Logf(L"Seek drag continue overlay=%d pt=(%ld,%ld) target=%s (%I64d)",
                 overlay ? 1 : 0,
                 pt.x,
                 pt.y,
                 FormatTime(target).c_str(),
                 target);
            LogPlaybackSnapshot(L"seek-drag-continue", target);
        }
    }

    void PlayerWindow::EndSeekDrag(HWND hwnd, bool overlay, POINT pt)
    {
        if (!draggingSeek_)
        {
            return;
        }

        draggingSeek_ = false;
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        const REFERENCE_TIME target = ProgressPointToTime(hwnd, overlay, pt.x);
        seekDragPendingTarget_ = target;
        progressOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
        Logf(L"Seek drag end overlay=%d pt=(%ld,%ld) target=%s (%I64d) restoreSegment=%d wasPlaying=%d",
             overlay ? 1 : 0,
             pt.x,
             pt.y,
             FormatTime(target).c_str(),
             target,
             restoreSegmentAfterDrag_ ? 1 : 0,
             wasPlayingBeforeDrag_ ? 1 : 0);
        QueueAsyncSeek(target, wasPlayingBeforeDrag_, true, SEEK_FINAL_PREVIEW_MAX_READ_COUNT, L"进度条拖动结束");
        UpdatePositionUi(target);

        if (restoreSegmentAfterDrag_)
        {
            ApplyPlaybackSegment();
        }
        restoreSegmentAfterDrag_ = false;
        wasPlayingBeforeDrag_ = false;
        seekDragLastPreviewTick_ = 0;
        seekDragLastPreviewTarget_ = -1;
        seekDragPendingTarget_ = -1;
        UpdateControls();
        UpdateOverlayState();
        LogPlaybackSnapshot(L"seek-drag-end", target);
    }

    void PlayerWindow::ShowProgressContextMenu(HWND hwnd, bool overlay, POINT pt)
    {
        if (!hasMedia_ || duration_ <= 0 || !IsPointInProgressArea(hwnd, overlay, pt))
        {
            return;
        }

        contextMenuPosition_ = ProgressPointToTime(hwnd, overlay, pt.x);
        const bool resumeAfterMenuSeek = isPlaying_;

        HMENU menu = CreatePopupMenu();
        if (!menu)
        {
            return;
        }

        AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_A, L"设置A点");
        AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_B, L"设置B点");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, IDM_PROGRESS_CLEAR_AB, L"取消AB点");

        POINT screen = pt;
        ClientToScreen(hwnd, &screen);
        HWND menuOwner = hwnd_;
        progressMenuActive_ = true;
        progressOverlayKeepVisibleUntil_ = 0;
        overlayVisiblePixels_ = FULLSCREEN_OVERLAY_HEIGHT;
        overlayTargetVisible_ = true;
        LayoutOverlay();
        if (overlayPanel_)
        {
            SetWindowPos(overlayPanel_, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        Logf(L"Progress context menu opened with overlay locked: overlay=%d, screen=(%ld,%ld), owner=%p, target=%s (%I64d), resumeAfterSeek=%d",
             overlay ? 1 : 0,
             screen.x,
             screen.y,
             menuOwner,
             FormatTime(contextMenuPosition_).c_str(),
             contextMenuPosition_,
             resumeAfterMenuSeek ? 1 : 0);

        SetForegroundWindow(hwnd_);
        const int command = TrackPopupMenu(menu,
                                           TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                                           screen.x,
                                           screen.y,
                                           0,
                                           menuOwner,
                                           nullptr);
        PostMessageW(menuOwner, WM_NULL, 0, 0);
        DestroyMenu(menu);

        progressMenuActive_ = false;
        progressOverlayKeepVisibleUntil_ = GetTickCount() + MOUSE_UI_IDLE_HIDE_MS;
        Logf(L"Progress context menu closed: command=%d", command);

        if (command == IDM_PROGRESS_SET_A)
        {
            SetLoopAAt(contextMenuPosition_, contextMenuPosition_);
            QueueAsyncSeek(contextMenuPosition_, resumeAfterMenuSeek, true, SEEK_FINAL_PREVIEW_MAX_READ_COUNT, L"进度条右键设置A点");
            UpdatePositionUi(contextMenuPosition_);
        }
        else if (command == IDM_PROGRESS_SET_B)
        {
            SetLoopBAt(contextMenuPosition_, contextMenuPosition_);
            QueueAsyncSeek(contextMenuPosition_, resumeAfterMenuSeek, true, SEEK_FINAL_PREVIEW_MAX_READ_COUNT, L"进度条右键设置B点");
            UpdatePositionUi(contextMenuPosition_);
        }
        else if (command == IDM_PROGRESS_CLEAR_AB)
        {
            ClearLoop();
        }
        UpdateOverlayState();
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
            SetTimer(hwnd_, TIMER_VIDEO_CLICK, GetDoubleClickTime() + 30, nullptr);
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
            if (hasMedia_ && hwnd == videoPanel_)
            {
                return 1;
            }

            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            FillRect(reinterpret_cast<HDC>(wparam), &rc, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            return 1;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);
            if (hasMedia_ && hwnd == videoPanel_ && player_)
            {
                player_->paintVideo(hdc, rc);
                if (pauseIndicatorAlpha_ > 0)
                {
                    PaintPauseIndicator(hdc, hwnd, pauseIndicatorAlpha_);
                }
            }
            else
            {
                FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
                if (!hasMedia_)
                {
                    PaintEmptyPlusButton(hdc, hwnd);
                }
            }
            EndPaint(hwnd, &ps);
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
                    InvalidateRect(videoHost_, nullptr, TRUE);
                    InvalidateRect(videoPanel_, nullptr, TRUE);
                }

                TRACKMOUSEEVENT tme = { 0 };
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                if (hovered)
                {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
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
                InvalidateRect(videoHost_, nullptr, TRUE);
                InvalidateRect(videoPanel_, nullptr, TRUE);
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
                InvalidateRect(hwnd, nullptr, TRUE);
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
            InvalidateRect(hwnd, nullptr, TRUE);
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
                Logf(L"Top overlay close button clicked");
                DestroyWindow(hwnd_);
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
        case WM_CAPTURECHANGED:
        {
            if (draggingSeek_ && reinterpret_cast<HWND>(lparam) != hwnd)
            {
                POINT pt = { 0 };
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                Logf(L"Seek drag capture changed, finishing drag: newCapture=%p, pt=(%ld,%ld)",
                     reinterpret_cast<HWND>(lparam),
                     pt.x,
                     pt.y);
                EndSeekDrag(hwnd, overlay, pt);
            }
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

    bool PlayerWindow::EnsureSeekWorker()
    {
        if (seekWorkerThread_)
        {
            return true;
        }

        seekRequestEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        seekExitEvent_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!seekRequestEvent_ || !seekExitEvent_)
        {
            Logf(L"Create async seek events failed: request=%p, exit=%p, lastError=%lu",
                 seekRequestEvent_,
                 seekExitEvent_,
                 GetLastError());
            StopSeekWorker();
            return false;
        }

        DWORD threadId = 0;
        seekWorkerThread_ = CreateThread(nullptr, 0, PlayerWindow::StaticSeekWorkerProc, this, 0, &threadId);
        if (!seekWorkerThread_)
        {
            Logf(L"Create async seek thread failed: lastError=%lu", GetLastError());
            StopSeekWorker();
            return false;
        }

        Logf(L"Async seek thread created: threadId=%lu", threadId);
        return true;
    }

    void PlayerWindow::StopSeekWorker()
    {
        HANDLE thread = nullptr;
        EnterCriticalSection(&seekWorkerLock_);
        thread = seekWorkerThread_;
        asyncSeekPending_ = false;
        asyncSeekBusy_ = false;
        ++asyncSeekSerial_;
        if (seekExitEvent_)
        {
            SetEvent(seekExitEvent_);
        }
        if (seekRequestEvent_)
        {
            SetEvent(seekRequestEvent_);
        }
        LeaveCriticalSection(&seekWorkerLock_);

        if (thread)
        {
            Logf(L"Stopping async seek thread");
            WaitForSingleObject(thread, INFINITE);
        }

        EnterCriticalSection(&seekWorkerLock_);
        if (seekWorkerThread_)
        {
            CloseHandle(seekWorkerThread_);
            seekWorkerThread_ = nullptr;
        }
        if (seekRequestEvent_)
        {
            CloseHandle(seekRequestEvent_);
            seekRequestEvent_ = nullptr;
        }
        if (seekExitEvent_)
        {
            CloseHandle(seekExitEvent_);
            seekExitEvent_ = nullptr;
        }
        asyncSeekPending_ = false;
        asyncSeekBusy_ = false;
        asyncSeekTarget_ = 0;
        asyncSeekPreviewMaxReadCount_ = 0;
        asyncSeekKeepPlaying_ = false;
        asyncSeekLog_ = false;
        asyncSeekActiveSerial_ = 0;
        LeaveCriticalSection(&seekWorkerLock_);
    }

    void PlayerWindow::QueueAsyncSeek(REFERENCE_TIME pos, bool keepPlaying, bool logSeek, size_t previewMaxReadCount, const wchar_t* reason)
    {
        if (!player_ || !player_->isOpen() || !mediaItemReady_)
        {
            return;
        }

        pos = ClampMediaPosition(pos);
        UpdatePositionUi(pos);
        if (!EnsureSeekWorker())
        {
            Logf(L"Async seek unavailable, fallback to sync seek: reason=%s, target=%s (%I64d)",
                 reason ? reason : L"",
                 FormatTime(pos).c_str(),
                 pos);
            SeekTo(pos, keepPlaying, logSeek, previewMaxReadCount);
            return;
        }

        DWORD serial = 0;
        EnterCriticalSection(&seekWorkerLock_);
        serial = ++asyncSeekSerial_;
        asyncSeekPending_ = true;
        asyncSeekTarget_ = pos;
        asyncSeekKeepPlaying_ = keepPlaying;
        asyncSeekLog_ = logSeek;
        asyncSeekPreviewMaxReadCount_ = previewMaxReadCount;
        LeaveCriticalSection(&seekWorkerLock_);

        SetEvent(seekRequestEvent_);
        Logf(L"Async seek queued: serial=%lu, reason=%s, target=%s (%I64d), keepPlaying=%d, log=%d, previewMax=%u",
             serial,
             reason ? reason : L"",
             FormatTime(pos).c_str(),
             pos,
             keepPlaying ? 1 : 0,
             logSeek ? 1 : 0,
             static_cast<unsigned int>(previewMaxReadCount));
    }

    bool PlayerWindow::UpdateAsyncSeekPlaybackIntent(bool keepPlaying, const wchar_t* reason)
    {
        bool updated = false;
        DWORD serial = 0;
        EnterCriticalSection(&seekWorkerLock_);
        if (asyncSeekPending_ || asyncSeekBusy_)
        {
            asyncSeekKeepPlaying_ = keepPlaying;
            updated = true;
            serial = asyncSeekSerial_;
        }
        LeaveCriticalSection(&seekWorkerLock_);

        if (updated)
        {
            Logf(L"Async seek playback intent updated: serial=%lu, keepPlaying=%d, reason=%s",
                 serial,
                 keepPlaying ? 1 : 0,
                 reason ? reason : L"");
        }
        return updated;
    }

    DWORD PlayerWindow::SeekWorkerProc()
    {
        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        Logf(L"Async seek thread enter: coHr=0x%08X", static_cast<unsigned int>(coHr));

        HANDLE waitHandles[2] = { seekExitEvent_, seekRequestEvent_ };
        for (;;)
        {
            const DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
            if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_FAILED)
            {
                break;
            }

            DWORD serial = 0;
            REFERENCE_TIME target = 0;
            bool keepPlaying = false;
            bool logSeek = false;
            size_t previewMaxReadCount = 0;
            bool haveRequest = false;
            EnterCriticalSection(&seekWorkerLock_);
            if (asyncSeekPending_)
            {
                serial = asyncSeekSerial_;
                target = asyncSeekTarget_;
                keepPlaying = asyncSeekKeepPlaying_;
                logSeek = asyncSeekLog_;
                previewMaxReadCount = asyncSeekPreviewMaxReadCount_;
                asyncSeekPending_ = false;
                asyncSeekBusy_ = true;
                asyncSeekActiveSerial_ = serial;
                haveRequest = true;
            }
            ResetEvent(seekRequestEvent_);
            LeaveCriticalSection(&seekWorkerLock_);

            if (!haveRequest)
            {
                continue;
            }

            const DWORD beginTick = GetTickCount();
            Logf(L"Async seek begin: serial=%lu, target=%s (%I64d), keepPlaying=%d, previewMax=%u",
                 serial,
                 FormatTime(target).c_str(),
                 target,
                 keepPlaying ? 1 : 0,
                 static_cast<unsigned int>(previewMaxReadCount));

            HRESULT seekHr = E_POINTER;
            HRESULT playHr = S_FALSE;
            if (player_)
            {
                seekHr = player_->seek(target, false, previewMaxReadCount);
                if (SUCCEEDED(seekHr))
                {
                    EnterCriticalSection(&seekWorkerLock_);
                    if (serial == asyncSeekSerial_)
                    {
                        keepPlaying = asyncSeekKeepPlaying_;
                    }
                    LeaveCriticalSection(&seekWorkerLock_);

                    if (keepPlaying && WaitForSingleObject(seekExitEvent_, 0) != WAIT_OBJECT_0)
                    {
                        playHr = player_->play();
                    }
                }
            }

            const DWORD elapsedMs = GetTickCount() - beginTick;
            bool shouldPost = WaitForSingleObject(seekExitEvent_, 0) != WAIT_OBJECT_0;
            EnterCriticalSection(&seekWorkerLock_);
            asyncSeekBusy_ = false;
            if (asyncSeekPending_ && asyncSeekSerial_ != serial)
            {
                shouldPost = false;
            }
            LeaveCriticalSection(&seekWorkerLock_);

            Logf(L"Async seek end: serial=%lu, target=%s (%I64d), seekHr=0x%08X, keepPlaying=%d, playHr=0x%08X, elapsed=%lu, post=%d",
                 serial,
                 FormatTime(target).c_str(),
                 target,
                 static_cast<unsigned int>(seekHr),
                 keepPlaying ? 1 : 0,
                 static_cast<unsigned int>(playHr),
                 elapsedMs,
                 shouldPost ? 1 : 0);

            if (shouldPost)
            {
                AsyncSeekResult* result = new AsyncSeekResult;
                result->serial = serial;
                result->target = target;
                result->keepPlaying = keepPlaying;
                result->logSeek = logSeek;
                result->previewMaxReadCount = previewMaxReadCount;
                result->seekHr = seekHr;
                result->playHr = playHr;
                result->elapsedMs = elapsedMs;
                if (!PostMessageW(hwnd_, WM_ASYNC_SEEK_DONE, 0, reinterpret_cast<LPARAM>(result)))
                {
                    delete result;
                }
            }
        }

        Logf(L"Async seek thread leave");
        if (SUCCEEDED(coHr))
        {
            CoUninitialize();
        }
        return 0;
    }

    void PlayerWindow::OnAsyncSeekDone(LPARAM resultParam)
    {
        AsyncSeekResult* result = reinterpret_cast<AsyncSeekResult*>(resultParam);
        if (!result)
        {
            return;
        }

        bool stale = false;
        DWORD latestSerial = 0;
        EnterCriticalSection(&seekWorkerLock_);
        latestSerial = asyncSeekSerial_;
        stale = result->serial != latestSerial;
        LeaveCriticalSection(&seekWorkerLock_);
        if (stale)
        {
            Logf(L"Async seek complete ignored as stale: serial=%lu, latest=%lu, target=%s (%I64d)",
                 result->serial,
                 latestSerial,
                 FormatTime(result->target).c_str(),
                 result->target);
            delete result;
            return;
        }

        Logf(L"Async seek complete on UI: serial=%lu, target=%s (%I64d), seekHr=0x%08X, keepPlaying=%d, playHr=0x%08X, elapsed=%lu, previewMax=%u",
             result->serial,
             FormatTime(result->target).c_str(),
             result->target,
             static_cast<unsigned int>(result->seekHr),
             result->keepPlaying ? 1 : 0,
             static_cast<unsigned int>(result->playHr),
             result->elapsedMs,
             static_cast<unsigned int>(result->previewMaxReadCount));

        if (FAILED(result->seekHr))
        {
            isPlaying_ = false;
            SetStatus((std::wstring(L"定位失败：") + HResultText(result->seekHr)).c_str());
        }
        else if (result->keepPlaying)
        {
            isPlaying_ = SUCCEEDED(result->playHr);
            if (isPlaying_)
            {
                manualPauseRequest_ = false;
                SetStatus(L"正在播放");
            }
        }
        else
        {
            isPlaying_ = false;
            SetStatus(L"已暂停");
        }

        const REFERENCE_TIME uiTarget = draggingSeek_ && seekDragPendingTarget_ >= 0 ? seekDragPendingTarget_ : result->target;
        UpdatePositionUi(uiTarget);
        UpdateControls();
        LogPlaybackSnapshot(L"async-seek-done", uiTarget);
        delete result;
    }

    void PlayerWindow::SeekTo(REFERENCE_TIME pos, bool keepPlaying, bool logSeek, size_t previewMaxReadCount)
    {
        if (!player_)
        {
            return;
        }

        if (!mediaItemReady_)
        {
            if (logSeek)
            {
                Logf(L"SeekTo ignored before MEDIAITEM_SET: target=%s (%I64d)", FormatTime(pos).c_str(), pos);
            }
            return;
        }

        pos = ClampMediaPosition(pos);

        HRESULT hr = player_->seek(pos, false, previewMaxReadCount);

        if (logSeek)
        {
            Logf(L"SeekTo target=%s (%I64d), keepPlaying=%d, previewMax=%u, hr=0x%08X",
                 FormatTime(pos).c_str(),
                 pos,
                 keepPlaying ? 1 : 0,
                 static_cast<unsigned int>(previewMaxReadCount),
                 static_cast<unsigned int>(hr));
        }

        if (keepPlaying && SUCCEEDED(hr))
        {
            HRESULT playHr = player_->play();
            if (logSeek)
            {
                Logf(L"SeekTo resume Play returned 0x%08X", static_cast<unsigned int>(playHr));
            }
            isPlaying_ = SUCCEEDED(playHr);
            if (isPlaying_)
            {
                manualPauseRequest_ = false;
                SetStatus(L"正在播放");
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
        if (!player_ || !player_->isOpen())
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
            HRESULT pauseHr = player_->pause();
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
        HRESULT hr = player_->play();
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
        else
        {
            manualPauseRequest_ = false;
        }
        UpdatePositionUi(pos);
    }

    void PlayerWindow::LogPlaybackSnapshot(const wchar_t* reason, REFERENCE_TIME knownPos)
    {
        if (!IsLoggingEnabled())
        {
            return;
        }

        REFERENCE_TIME pos = knownPos;
        bool gotPosition = knownPos >= 0;
        if (!gotPosition)
        {
            gotPosition = GetPosition(pos);
        }

        PlaybackEngineSnapshot engineSnapshot;
        if (player_)
        {
            player_->snapshot(engineSnapshot);
        }

        Logf(L"Playback snapshot reason=%s stateHr=0x%08X state=%s gotPos=%d pos=%s (%I64d) enginePos=%s (%I64d) ui=%s (%I64d) duration=%s (%I64d) hasMedia=%d mediaReady=%d autoPlayReady=%d isPlaying=%d suppressReplay=%d segmentStop=%d loopPending=%d loopFast=%d markerPending=%d draggingSeek=%d wasPlayingBeforeDrag=%d restoreSegmentAfterDrag=%d manualPause=%d loopEnabled=%d loopA=%s (%I64d) loopB=%s (%I64d) frameDuration=%I64d engineFrame=%I64d rate=%.2f engineRate=%.2f videoAvailable=%d audioAvailable=%d videoEnded=%d audioEnded=%d native=%dx%d zoom=%d pan=(%d,%d) videoTimeline=%d videoEnd=%s (%I64d)",
             reason ? reason : L"",
             static_cast<unsigned int>(engineSnapshot.m_lastResult),
             PlaybackEngineStateName(engineSnapshot.m_state),
             gotPosition ? 1 : 0,
             gotPosition ? FormatTime(pos).c_str() : L"--:--.---",
             gotPosition ? pos : -1,
             FormatTime(engineSnapshot.m_position).c_str(),
             engineSnapshot.m_position,
             FormatTime(uiPosition_).c_str(),
             uiPosition_,
             FormatTime(duration_).c_str(),
             duration_,
             hasMedia_ ? 1 : 0,
             mediaItemReady_ ? 1 : 0,
             autoPlayWhenMediaReady_ ? 1 : 0,
             isPlaying_ ? 1 : 0,
             suppressReplay_ ? 1 : 0,
             segmentStopApplied_ ? 1 : 0,
             loopReplayPending_ ? 1 : 0,
             loopReplayFastAttempt_ ? 1 : 0,
             markerSeekPending_ ? 1 : 0,
             draggingSeek_ ? 1 : 0,
             wasPlayingBeforeDrag_ ? 1 : 0,
             restoreSegmentAfterDrag_ ? 1 : 0,
             manualPauseRequest_ ? 1 : 0,
             IsLoopEnabled() ? 1 : 0,
             FormatTime(loopA_).c_str(),
             loopA_,
             FormatTime(loopB_).c_str(),
             loopB_,
             frameDuration_,
             engineSnapshot.m_frameDuration,
             PlaybackRate(),
             engineSnapshot.m_playbackRate,
             engineSnapshot.m_videoAvailable ? 1 : 0,
             engineSnapshot.m_audioAvailable ? 1 : 0,
             engineSnapshot.m_videoEnded ? 1 : 0,
             engineSnapshot.m_audioEnded ? 1 : 0,
             nativeVideoWidth_,
             nativeVideoHeight_,
             videoZoomPercent_,
             videoPanX_,
             videoPanY_,
             videoTimeline_.isValid ? 1 : 0,
             FormatTime(videoTimeline_.lastEndTime).c_str(),
             videoTimeline_.lastEndTime);
    }

    void PlayerWindow::OnTimer(bool updateUi)
    {
        if (draggingSeek_)
        {
            return;
        }

        if (!hasMedia_ || !player_ || !player_->isOpen())
        {
            return;
        }

        REFERENCE_TIME pos = 0;
        if (!GetPosition(pos))
        {
            return;
        }

        if (player_->state() == PlaybackEngineStateEnded)
        {
            OnPlaybackEngineEvent(PlaybackEngineEventEnded, S_OK);
            return;
        }

        const DWORD now = GetTickCount();
        if (IsLoggingEnabled() && (now - lastPositionLogTick_) >= 500)
        {
            lastPositionLogTick_ = now;
            LogPlaybackSnapshot(L"timer", pos);
        }

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

    void PlayerWindow::OnPlaybackEngineEvent(PlaybackEngineEvent eventType, HRESULT eventHr)
    {
        Logf(L"Playback engine window event: event=%s(%d), hr=0x%08X",
             PlaybackEngineEventName(eventType),
             static_cast<int>(eventType),
             static_cast<unsigned int>(eventHr));
        LogPlaybackSnapshot(PlaybackEngineEventName(eventType));

        if (FAILED(eventHr) || eventType == PlaybackEngineEventError)
        {
            isPlaying_ = false;
            manualPauseRequest_ = false;
            SetStatus((std::wstring(L"播放错误：") + HResultText(eventHr)).c_str());
            UpdateControls();
            return;
        }

        if (eventType == PlaybackEngineEventPositionSet)
        {
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
                        Logf(L"AB replay rearmed after engine POSITION_SET: pos=%s (%I64d), trigger=%s (%I64d)",
                             gotPosition ? FormatTime(current).c_str() : L"--:--.---",
                             gotPosition ? current : -1,
                             FormatTime(trigger).c_str(),
                             trigger);
                    }
                }
            }
            if (markerSeekPending_)
            {
                markerSeekPending_ = false;
                if (!IsShortActiveAbLoop() || ShouldLogLoopReplay())
                {
                    Logf(L"Marker nudge seek confirmed by engine POSITION_SET");
                }
            }
            UpdateControls();
            return;
        }

        if (eventType == PlaybackEngineEventEnded)
        {
            if (suppressReplay_)
            {
                suppressReplay_ = false;
                isPlaying_ = false;
                UpdateControls();
                return;
            }

            const DWORD now = GetTickCount();
            if (IsActiveAbLoop())
            {
                if ((now - lastLoopReplayTick_) < LOOP_ENDED_EVENT_SUPPRESS_MS)
                {
                    Logf(L"Ignore engine ended event after recent AB replay: elapsed=%lu", now - lastLoopReplayTick_);
                    UpdateControls();
                    return;
                }
                ReplayFrom(loopA_);
                SetStatus(L"AB循环播放");
            }
            else
            {
                ReplayFrom(0);
                SetStatus(L"从头循环播放");
            }
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
        if (draggingSeek_ && (GetCapture() == overlayPanel_ || GetCapture() == seekSlider_))
        {
            ReleaseCapture();
        }
        draggingSeek_ = false;
        progressMenuActive_ = false;
        StopSeekWorker();
        if (player_)
        {
            player_->uninit();
        }

        hasMedia_ = false;
        mediaItemReady_ = false;
        autoPlayWhenMediaReady_ = false;
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
        draggingSeek_ = false;
        progressMenuActive_ = false;
        wasPlayingBeforeDrag_ = false;
        restoreSegmentAfterDrag_ = false;
        markerSeekPendingTick_ = 0;
        progressOverlayKeepVisibleUntil_ = 0;
        loopReplayCount_ = 0;
        lastPositionLogTick_ = 0;
        lastSeekDragLogTick_ = 0;
        seekDragLastPreviewTick_ = 0;
        seekDragLastPreviewTarget_ = -1;
        seekDragPendingTarget_ = -1;
        nativeVideoWidth_ = 0;
        nativeVideoHeight_ = 0;
        ResetVideoTransform();
        duration_ = 0;
        loopA_ = -1;
        loopB_ = -1;
        videoTimeline_ = VideoTimelineInfo();
        filePath_.clear();
        playbackPath_.clear();

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
            InvalidateRect(videoHost_, nullptr, TRUE);
        }
        if (videoPanel_)
        {
            InvalidateRect(videoPanel_, nullptr, TRUE);
        }
        Logf(L"ClosePlayer end");
    }

}