#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0600
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <evr.h>
#include <mfapi.h>
#include <mfplay.h>
#include <propidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <stdarg.h>
#include <string>

namespace
{
    const wchar_t kWindowClass[] = L"LoopPlayerWindow";
    const wchar_t kProgressClass[] = L"LoopPlayerProgress";
    const wchar_t kOverlayClass[] = L"LoopPlayerOverlay";
    const wchar_t kAppTitle[] = L"Loop Player";

    const UINT WM_PLAYER_EVENT = WM_APP + 1;
    const UINT_PTR TIMER_POSITION = 1;
    const UINT_PTR TIMER_VIDEO_CLICK = 2;
    const REFERENCE_TIME ONE_SECOND = 10000000;
    const REFERENCE_TIME DEFAULT_FRAME_DURATION = ONE_SECOND / 30;
    const DWORD LOOP_REARM_DELAY_MS = 20;
    const DWORD LOOP_FAST_REPLAY_VERIFY_MS = 60;
    const DWORD LOOP_REPLAY_PENDING_TIMEOUT_MS = 750;
    const DWORD LOOP_ENDED_EVENT_SUPPRESS_MS = 500;
    const DWORD MARKER_SEEK_PENDING_TIMEOUT_MS = 500;
    const int LOOP_TRIGGER_EARLY_FRAMES = 3;
    const int NORMAL_PROGRESS_HEIGHT = 58;
    const int FULLSCREEN_OVERLAY_HEIGHT = 90;
    const int FULLSCREEN_OVERLAY_HOT_ZONE = 24;
    const int OVERLAY_ANIMATION_STEP = 16;
    const int PROGRESS_TIME_LABEL_WIDTH = 78;
    const int PROGRESS_TIME_LABEL_GAP = 10;
    const int PROGRESS_MARKER_NONE = 0;
    const int PROGRESS_MARKER_A = 1;
    const int PROGRESS_MARKER_B = 2;
    const int IDM_PROGRESS_SET_A = 40001;
    const int IDM_PROGRESS_SET_B = 40002;

    enum ControlId
    {
        IDC_OPEN = 1001,
        IDC_PLAY,
        IDC_PAUSE,
        IDC_STOP,
        IDC_SET_A,
        IDC_SET_B,
        IDC_A_PREV_FRAME,
        IDC_A_NEXT_FRAME,
        IDC_B_PREV_FRAME,
        IDC_B_NEXT_FRAME,
        IDC_CLEAR_AB,
        IDC_LOOP_AB,
        IDC_FULLSCREEN,
        IDC_SEEK,
        IDC_TIME_TEXT,
        IDC_AB_TEXT,
        IDC_VIDEO_PANEL,
        IDC_FULLSCREEN_OVERLAY
    };

    template <class T>
    void SafeRelease(T*& ptr)
    {
        if (ptr)
        {
            ptr->Release();
            ptr = NULL;
        }
    }

    std::wstring FormatTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--.---";
        }

        const LONGLONG totalMs = value / 10000;
        const int ms = static_cast<int>(totalMs % 1000);
        const LONGLONG totalSeconds = totalMs / 1000;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        if (hours > 0)
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%I64d:%02d:%02d.%03d", hours, minutes, seconds, ms);
        }
        else
        {
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02d:%02d.%03d", minutes, seconds, ms);
        }

        return buffer;
    }

    std::wstring FormatClockTime(REFERENCE_TIME value)
    {
        if (value < 0)
        {
            return L"--:--:--";
        }

        const LONGLONG totalSeconds = value / ONE_SECOND;
        const int seconds = static_cast<int>(totalSeconds % 60);
        const int minutes = static_cast<int>((totalSeconds / 60) % 60);
        const LONGLONG hours = totalSeconds / 3600;

        wchar_t buffer[64] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%02I64d:%02d:%02d", hours, minutes, seconds);
        return buffer;
    }

    std::wstring HResultText(HRESULT hr)
    {
        wchar_t buffer[256] = { 0 };
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"Error 0x%08X", static_cast<unsigned int>(hr));
        return buffer;
    }

    std::wstring GetLogFilePath()
    {
        wchar_t path[MAX_PATH] = { 0 };
        DWORD length = GetModuleFileNameW(NULL, path, ARRAYSIZE(path));
        if (length == 0 || length >= ARRAYSIZE(path))
        {
            return L"LoopPlayer.log";
        }

        PathRemoveFileSpecW(path);
        PathAppendW(path, L"LoopPlayer.log");
        return path;
    }

    void ResetLogFile()
    {
        const std::wstring logPath = GetLogFilePath();
        HANDLE file = CreateFileW(logPath.c_str(),
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
        }
    }

    void WriteUtf8Text(HANDLE file, const wchar_t* text)
    {
        if (!text)
        {
            return;
        }

        const int byteCount = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
        if (byteCount <= 1)
        {
            return;
        }

        std::string bytes;
        bytes.resize(byteCount - 1);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, &bytes[0], byteCount, NULL, NULL);

        DWORD written = 0;
        WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, NULL);
    }

    void Logf(const wchar_t* format, ...)
    {
        wchar_t message[2048] = { 0 };

        va_list args;
        va_start(args, format);
        StringCchVPrintfW(message, ARRAYSIZE(message), format, args);
        va_end(args);

        SYSTEMTIME now = { 0 };
        GetLocalTime(&now);

        wchar_t line[2300] = { 0 };
        StringCchPrintfW(line,
                         ARRAYSIZE(line),
                         L"[%04u-%02u-%02u %02u:%02u:%02u.%03u] %s\r\n",
                         now.wYear,
                         now.wMonth,
                         now.wDay,
                         now.wHour,
                         now.wMinute,
                         now.wSecond,
                         now.wMilliseconds,
                         message);

        const std::wstring logPath = GetLogFilePath();
        HANDLE file = CreateFileW(logPath.c_str(),
                                  FILE_APPEND_DATA,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            return;
        }

        WriteUtf8Text(file, line);
        CloseHandle(file);
    }

    const wchar_t* MediaPlayerStateName(MFP_MEDIAPLAYER_STATE state)
    {
        switch (state)
        {
        case MFP_MEDIAPLAYER_STATE_EMPTY:
            return L"EMPTY";
        case MFP_MEDIAPLAYER_STATE_STOPPED:
            return L"STOPPED";
        case MFP_MEDIAPLAYER_STATE_PLAYING:
            return L"PLAYING";
        case MFP_MEDIAPLAYER_STATE_PAUSED:
            return L"PAUSED";
        case MFP_MEDIAPLAYER_STATE_SHUTDOWN:
            return L"SHUTDOWN";
        default:
            return L"UNKNOWN";
        }
    }

    const wchar_t* MediaPlayerEventName(MFP_EVENT_TYPE type)
    {
        switch (type)
        {
        case MFP_EVENT_TYPE_PLAY:
            return L"PLAY";
        case MFP_EVENT_TYPE_PAUSE:
            return L"PAUSE";
        case MFP_EVENT_TYPE_STOP:
            return L"STOP";
        case MFP_EVENT_TYPE_POSITION_SET:
            return L"POSITION_SET";
        case MFP_EVENT_TYPE_RATE_SET:
            return L"RATE_SET";
        case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
            return L"MEDIAITEM_CREATED";
        case MFP_EVENT_TYPE_MEDIAITEM_SET:
            return L"MEDIAITEM_SET";
        case MFP_EVENT_TYPE_FRAME_STEP:
            return L"FRAME_STEP";
        case MFP_EVENT_TYPE_MEDIAITEM_CLEARED:
            return L"MEDIAITEM_CLEARED";
        case MFP_EVENT_TYPE_MF:
            return L"MF";
        case MFP_EVENT_TYPE_ERROR:
            return L"ERROR";
        case MFP_EVENT_TYPE_PLAYBACK_ENDED:
            return L"PLAYBACK_ENDED";
        case MFP_EVENT_TYPE_ACQUIRE_USER_CREDENTIAL:
            return L"ACQUIRE_USER_CREDENTIAL";
        default:
            return L"UNKNOWN";
        }
    }

    void InitInt64PropVariant(PROPVARIANT& value, LONGLONG number)
    {
        PropVariantInit(&value);
        value.vt = VT_I8;
        value.hVal.QuadPart = number;
    }

    bool ReadInt64PropVariant(const PROPVARIANT& value, LONGLONG& number)
    {
        if (value.vt == VT_I8)
        {
            number = value.hVal.QuadPart;
            return true;
        }
        if (value.vt == VT_UI8)
        {
            number = static_cast<LONGLONG>(value.uhVal.QuadPart);
            return true;
        }
        return false;
    }

    class PlayerCallback : public IMFPMediaPlayerCallback
    {
    public:
        PlayerCallback()
            : refCount_(1),
              hwnd_(NULL)
        {
        }

        void SetWindow(HWND hwnd)
        {
            hwnd_ = hwnd;
        }

        STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
        {
            if (!ppv)
            {
                return E_POINTER;
            }

            if (riid == __uuidof(IUnknown) || riid == __uuidof(IMFPMediaPlayerCallback))
            {
                *ppv = static_cast<IMFPMediaPlayerCallback*>(this);
                AddRef();
                return S_OK;
            }

            *ppv = NULL;
            return E_NOINTERFACE;
        }

        STDMETHODIMP_(ULONG) AddRef()
        {
            return static_cast<ULONG>(InterlockedIncrement(&refCount_));
        }

        STDMETHODIMP_(ULONG) Release()
        {
            LONG value = InterlockedDecrement(&refCount_);
            if (value < 0)
            {
                InterlockedIncrement(&refCount_);
                value = 0;
            }
            return static_cast<ULONG>(value);
        }

        void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* eventHeader)
        {
            if (!eventHeader)
            {
                return;
            }

            if (FAILED(eventHeader->hrEvent))
            {
                Logf(L"MFPlay callback error: event=%s(%d), hr=0x%08X, state=%s",
                     MediaPlayerEventName(eventHeader->eEventType),
                     static_cast<int>(eventHeader->eEventType),
                     static_cast<unsigned int>(eventHeader->hrEvent),
                     MediaPlayerStateName(eventHeader->eState));
            }

            HWND hwnd = hwnd_;
            if (hwnd)
            {
                PostMessageW(hwnd,
                             WM_PLAYER_EVENT,
                             static_cast<WPARAM>(eventHeader->eEventType),
                             static_cast<LPARAM>(eventHeader->hrEvent));
            }
        }

    private:
        volatile LONG refCount_;
        HWND hwnd_;
    };

    class PlayerWindow
    {
    public:
        PlayerWindow()
            : hinst_(NULL),
              hwnd_(NULL),
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
              savedStyle_(0),
              savedExStyle_(0),
              lastLoopReplayTick_(0),
              markerSeekPendingTick_(0),
              loopReplayCount_(0),
              playbackRateTenths_(10),
              uiPosition_(0),
              overlayVisiblePixels_(0),
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
        }

        bool Create(HINSTANCE hinst, int cmdShow)
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

            hwnd_ = CreateWindowExW(
                0,
                kWindowClass,
                kAppTitle,
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                900,
                600,
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

        HWND hwnd() const
        {
            return hwnd_;
        }

        bool LoadFile(const wchar_t* path)
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
            ReadDuration();
            DetectFrameDuration(item);
            SafeRelease(item);

            loopA_ = -1;
            loopB_ = -1;
            hasMedia_ = true;
            isPlaying_ = false;
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

            UpdateTitle();
            UpdateLoopText();
            UpdateTimeText(0);
            UpdateControls();
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

    private:
        static LRESULT CALLBACK StaticProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

        static LRESULT CALLBACK StaticOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

        static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

        LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
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
            case WM_LBUTTONDBLCLK:
                ToggleFullScreen();
                return 0;
            case WM_GETMINMAXINFO:
                OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lparam));
                return 0;
            case WM_DROPFILES:
                OnDropFiles(reinterpret_cast<HDROP>(wparam));
                return 0;
            case WM_TIMER:
                if (wparam == TIMER_POSITION)
                {
                    OnTimer();
                    UpdateOverlayState();
                    return 0;
                }
                if (wparam == TIMER_VIDEO_CLICK)
                {
                    KillTimer(hwnd_, TIMER_VIDEO_CLICK);
                    TogglePlayback();
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
                callback_.SetWindow(NULL);
                ClosePlayer();
                PostQuitMessage(0);
                return 0;
            default:
                break;
            }

            return DefWindowProcW(hwnd_, msg, wparam, lparam);
        }

        void RegisterChildClasses()
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
        }

        void OnCreate()
        {
            InitCommonControls();
            RegisterChildClasses();
            Logf(L"Window created. Log path: %s", GetLogFilePath().c_str());

            openButton_ = CreateButton(L"Open", IDC_OPEN);
            playButton_ = CreateButton(L"Play", IDC_PLAY);
            pauseButton_ = CreateButton(L"Pause", IDC_PAUSE);
            stopButton_ = CreateButton(L"Stop", IDC_STOP);
            setAButton_ = CreateButton(L"Set A", IDC_SET_A);
            setBButton_ = CreateButton(L"Set B", IDC_SET_B);
            aPrevFrameButton_ = CreateButton(L"A -1F", IDC_A_PREV_FRAME);
            aNextFrameButton_ = CreateButton(L"A +1F", IDC_A_NEXT_FRAME);
            bPrevFrameButton_ = CreateButton(L"B -1F", IDC_B_PREV_FRAME);
            bNextFrameButton_ = CreateButton(L"B +1F", IDC_B_NEXT_FRAME);
            clearButton_ = CreateButton(L"Clear AB", IDC_CLEAR_AB);
            loopCheck_ = CreateWindowExW(0, L"BUTTON", L"Loop AB", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                         0, 0, 90, 24, hwnd_, reinterpret_cast<HMENU>(IDC_LOOP_AB), hinst_, NULL);
            SendMessageW(loopCheck_, BM_SETCHECK, BST_CHECKED, 0);
            fullScreenButton_ = CreateButton(L"Full", IDC_FULLSCREEN);

            seekSlider_ = CreateWindowExW(0, kProgressClass, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          0, 0, 100, NORMAL_PROGRESS_HEIGHT, hwnd_, reinterpret_cast<HMENU>(IDC_SEEK), hinst_, this);

            timeText_ = CreateWindowExW(0, L"STATIC", L"--:--.--- / --:--.---", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                        0, 0, 220, 20, hwnd_, reinterpret_cast<HMENU>(IDC_TIME_TEXT), hinst_, NULL);
            abText_ = CreateWindowExW(0, L"STATIC", L"A: --:--.---   B: --:--.---", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      0, 0, 320, 20, hwnd_, reinterpret_cast<HMENU>(IDC_AB_TEXT), hinst_, NULL);
            videoPanel_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BLACKRECT | SS_NOTIFY | WS_CLIPSIBLINGS,
                                          0, 0, 100, 100, hwnd_, reinterpret_cast<HMENU>(IDC_VIDEO_PANEL), hinst_, NULL);
            overlayPanel_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                            kOverlayClass,
                                            L"",
                                            WS_POPUP | WS_CLIPSIBLINGS,
                                            0, 0, 100, FULLSCREEN_OVERLAY_HEIGHT, hwnd_,
                                            NULL,
                                            hinst_,
                                            this);
            ShowWindow(overlayPanel_, SW_HIDE);

            DragAcceptFiles(hwnd_, TRUE);
            SetTimer(hwnd_, TIMER_POSITION, 10, NULL);

            LayoutControls();
            UpdateControls();
            SetStatus(L"Open or drop an MP4/AVI file");
        }

        HWND CreateButton(const wchar_t* text, int id)
        {
            return CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   0, 0, 70, 26, hwnd_, reinterpret_cast<HMENU>(id), hinst_, NULL);
        }

        bool ReportFailure(const wchar_t* action, HRESULT hr)
        {
            std::wstring message = action ? action : L"Operation failed";
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

        void LayoutControls()
        {
            RECT rc = { 0 };
            GetClientRect(hwnd_, &rc);

            if (isFullScreen_)
            {
                ShowControls(SW_HIDE);
                MoveWindow(videoPanel_, 0, 0, max(1, rc.right), max(1, rc.bottom), TRUE);
                LayoutOverlay();
                return;
            }

            overlayVisiblePixels_ = 0;
            overlayTargetVisible_ = false;
            progressMenuActive_ = false;
            ShowWindow(overlayPanel_, SW_HIDE);
            ShowControls(SW_SHOW);

            const int margin = 8;
            const int rowY = 8;
            const int buttonH = 26;
            int x = margin;

            MoveWindow(openButton_, x, rowY, 78, buttonH, TRUE);
            x += 84;
            MoveWindow(playButton_, x, rowY, 58, buttonH, TRUE);
            x += 64;
            MoveWindow(pauseButton_, x, rowY, 64, buttonH, TRUE);
            x += 70;
            MoveWindow(stopButton_, x, rowY, 58, buttonH, TRUE);
            x += 70;
            MoveWindow(setAButton_, x, rowY, 68, buttonH, TRUE);
            x += 74;
            MoveWindow(setBButton_, x, rowY, 68, buttonH, TRUE);
            x += 74;
            MoveWindow(clearButton_, x, rowY, 78, buttonH, TRUE);
            x += 88;
            MoveWindow(loopCheck_, x, rowY + 3, 90, 22, TRUE);
            x += 96;
            MoveWindow(fullScreenButton_, x, rowY, 58, buttonH, TRUE);

            const int timeWidth = 230;
            MoveWindow(timeText_, max(margin, rc.right - timeWidth - margin), rowY + 4, timeWidth, 20, TRUE);

            const int secondY = 42;
            const int adjustButtonW = 62;
            x = margin;
            MoveWindow(abText_, x, secondY + 6, 300, 20, TRUE);
            x += 308;
            MoveWindow(aPrevFrameButton_, x, secondY, adjustButtonW, buttonH, TRUE);
            x += adjustButtonW + 6;
            MoveWindow(aNextFrameButton_, x, secondY, adjustButtonW, buttonH, TRUE);
            x += adjustButtonW + 12;
            MoveWindow(bPrevFrameButton_, x, secondY, adjustButtonW, buttonH, TRUE);
            x += adjustButtonW + 6;
            MoveWindow(bNextFrameButton_, x, secondY, adjustButtonW, buttonH, TRUE);

            const int sliderY = 76;
            MoveWindow(seekSlider_, margin, sliderY, max(120, rc.right - margin * 2), NORMAL_PROGRESS_HEIGHT, TRUE);

            const int videoTop = 142;
            MoveWindow(videoPanel_, margin, videoTop, max(1, rc.right - margin * 2), max(1, rc.bottom - videoTop - margin), TRUE);
        }

        bool GetOverlayAnchorRect(RECT& screenRect) const
        {
            ZeroMemory(&screenRect, sizeof(screenRect));

            if (isFullScreen_)
            {
                return GetWindowRect(hwnd_, &screenRect) != FALSE;
            }

            if (!hasMedia_ || !videoPanel_)
            {
                return false;
            }

            return GetWindowRect(videoPanel_, &screenRect) != FALSE;
        }

        void LayoutOverlay()
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

            const int width = max(1, screenRect.right - screenRect.left);
            const int y = screenRect.bottom - visible;
            const HWND insertAfter = isFullScreen_ ? HWND_TOPMOST : HWND_TOP;
            SetWindowPos(overlayPanel_,
                         insertAfter,
                         screenRect.left,
                         y,
                         width,
                         FULLSCREEN_OVERLAY_HEIGHT,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }

        bool IsWindowFromThisInstance(HWND window) const
        {
            if (!window)
            {
                return false;
            }

            if (window == hwnd_ || window == overlayPanel_)
            {
                return true;
            }

            if ((hwnd_ && IsChild(hwnd_, window)) || (overlayPanel_ && IsChild(overlayPanel_, window)))
            {
                return true;
            }

            HWND root = GetAncestor(window, GA_ROOT);
            if (root == hwnd_ || root == overlayPanel_)
            {
                return true;
            }

            HWND rootOwner = GetAncestor(window, GA_ROOTOWNER);
            if (rootOwner == hwnd_ || rootOwner == overlayPanel_)
            {
                return true;
            }

            HWND owner = window;
            while ((owner = GetWindow(owner, GW_OWNER)) != NULL)
            {
                if (owner == hwnd_ || owner == overlayPanel_)
                {
                    return true;
                }
            }

            return false;
        }

        void UpdateOverlayState()
        {
            if (!overlayPanel_)
            {
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
            const bool targetVisible = draggingSeek_ ||
                                       progressMenuActive_ ||
                                       (foregroundThisInstance && (mouseInHotZone || mouseInOverlay));
            const int target = targetVisible ? FULLSCREEN_OVERLAY_HEIGHT : 0;

            if (overlayTargetVisible_ != targetVisible)
            {
                overlayTargetVisible_ = targetVisible;
                Logf(L"Fullscreen overlay target %s: cursorScreen=(%ld,%ld), cursorClient=(%ld,%ld), cursorWindow=%p, foreground=%p, overThis=%d, foregroundThis=%d, visible=%d",
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

        void ResizeVideoWindow()
        {
            if (player_)
            {
                HRESULT hr = player_->UpdateVideo();
                if (FAILED(hr))
                {
                    Logf(L"UpdateVideo failed: 0x%08X", static_cast<unsigned int>(hr));
                }
            }
        }

        void ShowControls(int cmdShow)
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

        void ToggleFullScreen()
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

        void EnterFullScreen()
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

        void ExitFullScreen()
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
            SetWindowPos(overlayPanel_, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
            SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
            SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
            SetWindowPlacement(hwnd_, &savedPlacement_);
            SetWindowPos(hwnd_, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            LayoutControls();
            ResizeVideoWindow();
            Logf(L"ExitFullScreen: hwnd=%p, overlay=%p", hwnd_, overlayPanel_);
        }

        void OnGetMinMaxInfo(MINMAXINFO* info)
        {
            if (!info)
            {
                return;
            }

            info->ptMinTrackSize.x = 700;
            info->ptMinTrackSize.y = 420;
        }

        void OnCommand(WORD id, WORD notifyCode)
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

        void OpenFileDialog()
        {
            wchar_t path[MAX_PATH] = { 0 };

            OPENFILENAMEW ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd_;
            ofn.lpstrFilter = L"Video files (*.mp4;*.avi)\0*.mp4;*.avi\0MP4 files (*.mp4)\0*.mp4\0AVI files (*.avi)\0*.avi\0All files (*.*)\0*.*\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = ARRAYSIZE(path);
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
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
        }

        void OnDropFiles(HDROP drop)
        {
            wchar_t path[MAX_PATH] = { 0 };
            if (DragQueryFileW(drop, 0, path, ARRAYSIZE(path)))
            {
                Logf(L"Drop file selected: %s", path);
                LoadFile(path);
            }
            DragFinish(drop);
        }

        void Play()
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

        float PlaybackRate() const
        {
            return static_cast<float>(playbackRateTenths_) / 10.0f;
        }

        void FormatPlaybackRate(wchar_t* buffer, size_t count) const
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

        bool ApplyPlaybackRate(bool showStatus)
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

        void AdjustPlaybackRate(int steps)
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

        void TogglePlayback()
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

        void Pause()
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

        void Stop()
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

        bool ReadDuration()
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

        void DetectFrameDuration(IMFPMediaItem* item)
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

        bool GetPosition(REFERENCE_TIME& pos) const
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

        REFERENCE_TIME LoopMinLength() const
        {
            return frameDuration_ > 0 ? frameDuration_ : DEFAULT_FRAME_DURATION;
        }

        REFERENCE_TIME ClampMediaPosition(REFERENCE_TIME pos) const
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

        REFERENCE_TIME SnapToNearestFrame(REFERENCE_TIME pos) const
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

        REFERENCE_TIME LoopBFrameEnd() const
        {
            if (loopB_ < 0)
            {
                return -1;
            }

            return ClampMediaPosition(loopB_ + LoopMinLength());
        }

        REFERENCE_TIME LoopTriggerPosition() const
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

        bool HasValidAbLoop() const
        {
            return loopA_ >= 0 && loopB_ > loopA_;
        }

        bool IsActiveAbLoop() const
        {
            return IsLoopEnabled() && HasValidAbLoop();
        }

        REFERENCE_TIME ActiveLoopLength() const
        {
            return HasValidAbLoop() ? loopB_ - loopA_ : 0;
        }

        bool IsShortActiveAbLoop() const
        {
            return IsActiveAbLoop() && ActiveLoopLength() <= ONE_SECOND;
        }

        bool ShouldLogLoopReplay() const
        {
            return loopReplayCount_ <= 5 || (loopReplayCount_ % 20) == 0;
        }

        bool ShouldLogPlayerEvent(MFP_EVENT_TYPE eventType) const
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

        bool IsMarkerSeekStillPending()
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

        bool ApplyPlaybackSegment()
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

        void SetLoopA()
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

        void SetLoopAAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos)
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

        void SetLoopB()
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

        void SetLoopBAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos)
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

        void ClearLoop()
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

        bool NudgeLoopPoint(bool isA, int frames, bool previewFrame = true)
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

        RECT GetProgressTrackRect(HWND hwnd, bool overlay) const
        {
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);

            RECT track = rc;
            const int edge = overlay ? 20 : 12;
            track.left += edge + PROGRESS_TIME_LABEL_WIDTH + PROGRESS_TIME_LABEL_GAP;
            track.right -= edge + PROGRESS_TIME_LABEL_WIDTH + PROGRESS_TIME_LABEL_GAP;
            const int centerY = overlay ? 26 : 22;
            track.top = centerY - 3;
            track.bottom = centerY + 3;
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

        RECT GetOverlayPlayRect(HWND hwnd) const
        {
            RECT rc = { 0 };
            GetClientRect(hwnd, &rc);

            const int buttonW = 82;
            const int buttonH = 30;
            RECT button = { 0 };
            button.left = (rc.right - buttonW) / 2;
            button.top = 48;
            button.right = button.left + buttonW;
            button.bottom = button.top + buttonH;
            return button;
        }

        int TimeToProgressX(REFERENCE_TIME pos, const RECT& track) const
        {
            if (duration_ <= 0)
            {
                return track.left;
            }

            pos = ClampMediaPosition(pos);
            const int width = max(1, track.right - track.left);
            return track.left + static_cast<int>((pos * width) / duration_);
        }

        REFERENCE_TIME ProgressXToTime(int x, const RECT& track) const
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

        bool IsPointInProgressArea(HWND hwnd, bool overlay, POINT pt) const
        {
            RECT track = GetProgressTrackRect(hwnd, overlay);
            RECT area = track;
            InflateRect(&area, 12, overlay ? 22 : 26);
            return PtInRect(&area, pt) != FALSE;
        }

        int HitTestProgressMarker(HWND hwnd, bool overlay, POINT pt) const
        {
            if (!hasMedia_ || duration_ <= 0)
            {
                return PROGRESS_MARKER_NONE;
            }

            RECT track = GetProgressTrackRect(hwnd, overlay);
            const int cy = (track.top + track.bottom) / 2;
            const int hitRadius = 12;
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

        void PaintProgressMarker(HDC hdc, HWND hwnd, bool overlay, const RECT& track, int marker, REFERENCE_TIME pos)
        {
            if (pos < 0)
            {
                return;
            }

            const bool hovered = hoveredMarker_ == marker && hoverMarkerWindow_ == hwnd;
            const int x = TimeToProgressX(pos, track);
            const int cy = (track.top + track.bottom) / 2;
            const int radius = hovered ? 7 : 4;

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
            RECT labelRect = { x - 12, track.bottom + 7, x + 12, track.bottom + 25 };
            SetTextColor(hdc, overlay ? RGB(235, 235, 235) : RGB(60, 60, 60));
            DrawTextW(hdc, label, -1, &labelRect, DT_CENTER | DT_TOP | DT_SINGLELINE);

            if (hovered)
            {
                std::wstring time = FormatTime(pos);
                RECT timeRect = { x - 48, max(0, track.top - 23), x + 48, track.top - 5 };
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

        void PaintProgressTrack(HDC hdc, HWND hwnd, bool overlay)
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
            RECT currentTimeRect = { max(rc.left + 2, track.left - PROGRESS_TIME_LABEL_GAP - PROGRESS_TIME_LABEL_WIDTH),
                                     cy - 10,
                                     track.left - PROGRESS_TIME_LABEL_GAP,
                                     cy + 11 };
            RECT durationRect = { track.right + PROGRESS_TIME_LABEL_GAP,
                                  cy - 10,
                                  min(rc.right - 2, track.right + PROGRESS_TIME_LABEL_GAP + PROGRESS_TIME_LABEL_WIDTH),
                                  cy + 11 };
            SetTextColor(hdc, textColor);
            DrawTextW(hdc, FormatClockTime(uiPosition_).c_str(), -1, &currentTimeRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            DrawTextW(hdc, FormatClockTime(duration_).c_str(), -1, &durationRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            RECT bar = { track.left, cy - 2, track.right, cy + 2 };
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
            Ellipse(hdc, thumbX - 6, cy - 6, thumbX + 7, cy + 7);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(thumbBrush);
            DeleteObject(thumbPen);

            PaintProgressMarker(hdc, hwnd, overlay, track, PROGRESS_MARKER_A, loopA_);
            PaintProgressMarker(hdc, hwnd, overlay, track, PROGRESS_MARKER_B, loopB_);
        }

        void PaintOverlay(HDC hdc, HWND hwnd)
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
            RoundRect(hdc, button.left, button.top, button.right, button.bottom, 8, 8);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(buttonBrush);
            DeleteObject(buttonPen);

            SetTextColor(hdc, RGB(245, 245, 245));
            DrawTextW(hdc, isPlaying_ ? L"Pause" : L"Play", -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        void InvalidateProgressViews()
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

        void UpdateProgressHover(HWND hwnd, bool overlay, POINT pt)
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

        void ClearProgressHover(HWND hwnd)
        {
            if (hoverMarkerWindow_ == hwnd)
            {
                hoveredMarker_ = PROGRESS_MARKER_NONE;
                hoverMarkerWindow_ = NULL;
                InvalidateProgressViews();
            }
        }

        void MoveCursorToHoveredMarker()
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

        bool NudgeHoveredMarker(int frames)
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

        void ClearNativePlaybackSegmentForSeek()
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

        void SeekFromProgressPoint(HWND hwnd, bool overlay, int x, bool logSeek)
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

        void BeginSeekDrag(HWND hwnd, bool overlay, POINT pt)
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

        void ContinueSeekDrag(HWND hwnd, bool overlay, POINT pt)
        {
            if (!draggingSeek_ || GetCapture() != hwnd)
            {
                return;
            }

            SeekFromProgressPoint(hwnd, overlay, pt.x, false);
        }

        void EndSeekDrag(HWND hwnd, bool overlay, POINT pt)
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

        void ShowProgressContextMenu(HWND hwnd, bool overlay, POINT pt)
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

            AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_A, L"Set A");
            AppendMenuW(menu, MF_STRING, IDM_PROGRESS_SET_B, L"Set B");

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

        LRESULT ProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool overlay)
        {
            switch (msg)
            {
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
                POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
                EndSeekDrag(hwnd, overlay, pt);
                return 0;
            }
            case WM_RBUTTONUP:
            {
                POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
                ShowProgressContextMenu(hwnd, overlay, pt);
                return 0;
            }
            case WM_CONTEXTMENU:
            {
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

        void SeekTo(REFERENCE_TIME pos, bool keepPlaying, bool logSeek = true)
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

        void LogLoopTrigger() const
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

        void ReplayFrom(REFERENCE_TIME pos, bool forcePauseBeforeSeek = false)
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

        void OnTimer()
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
                        UpdatePositionUi(loopA_);
                        return;
                    }
                    else if ((now - lastLoopReplayTick_) < LOOP_REPLAY_PENDING_TIMEOUT_MS)
                    {
                        UpdatePositionUi(loopA_);
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

            UpdatePositionUi(pos);
        }

        void UpdatePositionUi(REFERENCE_TIME pos)
        {
            uiPosition_ = ClampMediaPosition(pos);
            InvalidateProgressViews();
            UpdateTimeText(pos);
        }

        void UpdateTimeText(REFERENCE_TIME pos)
        {
            if (hasMedia_)
            {
                pos = SnapToNearestFrame(pos);
            }

            const std::wstring text = FormatTime(pos) + L" / " + FormatTime(duration_);
            SetWindowTextW(timeText_, text.c_str());
        }

        bool IsLoopEnabled() const
        {
            return SendMessageW(loopCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
        }

        void UpdateLoopText()
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

        void UpdateControls()
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

        void SetStatus(const wchar_t* status)
        {
            std::wstring title = kAppTitle;
            if (status && status[0])
            {
                title += L" - ";
                title += status;
            }
            SetWindowTextW(hwnd_, title.c_str());
        }

        void UpdateTitle()
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

        void OnPlayerEvent(MFP_EVENT_TYPE eventType, HRESULT eventHr)
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

        void OnKeyDown(WPARAM key)
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

        void ClosePlayer()
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
            suppressReplay_ = false;
            segmentStopApplied_ = false;
            loopReplayPending_ = false;
            loopReplayFastAttempt_ = false;
            markerSeekPending_ = false;
            manualPauseRequest_ = false;
            markerSeekPendingTick_ = 0;
            loopReplayCount_ = 0;
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
            if (videoPanel_)
            {
                InvalidateRect(videoPanel_, NULL, TRUE);
            }
            Logf(L"ClosePlayer end");
        }

    private:
        HINSTANCE hinst_;
        HWND hwnd_;
        HWND videoPanel_;
        HWND openButton_;
        HWND playButton_;
        HWND pauseButton_;
        HWND stopButton_;
        HWND setAButton_;
        HWND setBButton_;
        HWND aPrevFrameButton_;
        HWND aNextFrameButton_;
        HWND bPrevFrameButton_;
        HWND bNextFrameButton_;
        HWND clearButton_;
        HWND loopCheck_;
        HWND fullScreenButton_;
        HWND seekSlider_;
        HWND timeText_;
        HWND abText_;
        HWND overlayPanel_;

        PlayerCallback callback_;
        IMFPMediaPlayer* player_;

        bool hasMedia_;
        bool isPlaying_;
        bool suppressReplay_;
        bool segmentStopApplied_;
        bool loopReplayPending_;
        bool loopReplayFastAttempt_;
        bool markerSeekPending_;
        bool manualPauseRequest_;
        bool draggingSeek_;
        bool progressMenuActive_;
        bool wasPlayingBeforeDrag_;
        bool restoreSegmentAfterDrag_;
        bool isFullScreen_;
        bool overlayTargetVisible_;
        LONG savedStyle_;
        LONG savedExStyle_;
        DWORD lastLoopReplayTick_;
        DWORD markerSeekPendingTick_;
        DWORD loopReplayCount_;
        int playbackRateTenths_;
        WINDOWPLACEMENT savedPlacement_;
        REFERENCE_TIME uiPosition_;
        int overlayVisiblePixels_;
        int hoveredMarker_;
        HWND hoverMarkerWindow_;
        REFERENCE_TIME contextMenuPosition_;
        REFERENCE_TIME duration_;
        REFERENCE_TIME loopA_;
        REFERENCE_TIME loopB_;
        REFERENCE_TIME frameDuration_;
        std::wstring filePath_;
    };
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int cmdShow)
{
    ResetLogFile();
    Logf(L"================ LoopPlayer process start ================");
    Logf(L"Command line: %s", GetCommandLineW());

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        Logf(L"CoInitializeEx failed: 0x%08X", static_cast<unsigned int>(hr));
        MessageBoxW(NULL, L"COM initialization failed.", kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }
    Logf(L"CoInitializeEx succeeded");

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
        Logf(L"MFStartup failed: 0x%08X", static_cast<unsigned int>(hr));
        CoUninitialize();
        MessageBoxW(NULL, L"Media Foundation initialization failed.", kAppTitle, MB_ICONERROR | MB_OK);
        return 1;
    }
    Logf(L"MFStartup succeeded");

    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    PlayerWindow window;
    if (!window.Create(hInstance, cmdShow))
    {
        MFShutdown();
        CoUninitialize();
        return 1;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc > 1)
    {
        window.LoadFile(argv[1]);
    }
    if (argv)
    {
        LocalFree(argv);
    }

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    MFShutdown();
    CoUninitialize();
    Logf(L"Process exit: code=%d", static_cast<int>(msg.wParam));
    return static_cast<int>(msg.wParam);
}
