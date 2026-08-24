#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0600
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
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
    const wchar_t kAppTitle[] = L"Loop Player";

    const UINT WM_PLAYER_EVENT = WM_APP + 1;
    const UINT_PTR TIMER_POSITION = 1;
    const int SLIDER_RANGE = 10000;
    const REFERENCE_TIME ONE_SECOND = 10000000;
    const REFERENCE_TIME DEFAULT_FRAME_DURATION = ONE_SECOND / 30;
    const DWORD LOOP_REARM_DELAY_MS = 90;
    const int LOOP_TRIGGER_EARLY_FRAMES = 3;

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
        IDC_VIDEO_PANEL
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

            Logf(L"MFPlay callback: event=%s(%d), hr=0x%08X, state=%s",
                 MediaPlayerEventName(eventHeader->eEventType),
                 static_cast<int>(eventHeader->eEventType),
                 static_cast<unsigned int>(eventHeader->hrEvent),
                 MediaPlayerStateName(eventHeader->eState));

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
              player_(NULL),
              hasMedia_(false),
              isPlaying_(false),
              updatingSlider_(false),
              suppressReplay_(false),
              segmentStopApplied_(false),
              isFullScreen_(false),
              savedStyle_(0),
              savedExStyle_(0),
              lastLoopReplayTick_(0),
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
            filePath_ = path;

            SendMessageW(seekSlider_, TBM_SETRANGE, TRUE, MAKELPARAM(0, SLIDER_RANGE));
            SendMessageW(seekSlider_, TBM_SETPOS, TRUE, 0);

            UpdateTitle();
            UpdateLoopText();
            UpdateTimeText(0);
            UpdateControls();
            ResizeVideoWindow();

            hr = player_->Play();
            if (SUCCEEDED(hr))
            {
                isPlaying_ = true;
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
            case WM_HSCROLL:
                if (reinterpret_cast<HWND>(lparam) == seekSlider_)
                {
                    OnSeekSlider(LOWORD(wparam));
                    return 0;
                }
                break;
            case WM_DROPFILES:
                OnDropFiles(reinterpret_cast<HDROP>(wparam));
                return 0;
            case WM_TIMER:
                if (wparam == TIMER_POSITION)
                {
                    OnTimer();
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
                callback_.SetWindow(NULL);
                ClosePlayer();
                PostQuitMessage(0);
                return 0;
            default:
                break;
            }

            return DefWindowProcW(hwnd_, msg, wparam, lparam);
        }

        void OnCreate()
        {
            InitCommonControls();
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

            seekSlider_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
                                          0, 0, 100, 28, hwnd_, reinterpret_cast<HMENU>(IDC_SEEK), hinst_, NULL);
            SendMessageW(seekSlider_, TBM_SETRANGE, TRUE, MAKELPARAM(0, SLIDER_RANGE));
            SendMessageW(seekSlider_, TBM_SETPAGESIZE, 0, 250);
            SendMessageW(seekSlider_, TBM_SETLINESIZE, 0, 50);

            timeText_ = CreateWindowExW(0, L"STATIC", L"--:--.--- / --:--.---", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                        0, 0, 220, 20, hwnd_, reinterpret_cast<HMENU>(IDC_TIME_TEXT), hinst_, NULL);
            abText_ = CreateWindowExW(0, L"STATIC", L"A: --:--.---   B: --:--.---", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      0, 0, 320, 20, hwnd_, reinterpret_cast<HMENU>(IDC_AB_TEXT), hinst_, NULL);
            videoPanel_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_BLACKRECT | SS_NOTIFY | WS_CLIPSIBLINGS,
                                          0, 0, 100, 100, hwnd_, reinterpret_cast<HMENU>(IDC_VIDEO_PANEL), hinst_, NULL);

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
                return;
            }

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
            MoveWindow(seekSlider_, margin, sliderY, max(120, rc.right - margin * 2), 34, TRUE);

            const int videoTop = 118;
            MoveWindow(videoPanel_, margin, videoTop, max(1, rc.right - margin * 2), max(1, rc.bottom - videoTop - margin), TRUE);
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
        }

        void ExitFullScreen()
        {
            if (!isFullScreen_)
            {
                return;
            }

            isFullScreen_ = false;
            SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
            SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
            SetWindowPlacement(hwnd_, &savedPlacement_);
            SetWindowPos(hwnd_, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            LayoutControls();
            ResizeVideoWindow();
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
                ToggleFullScreen();
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

        void Pause()
        {
            if (!player_)
            {
                return;
            }

            HRESULT hr = player_->Pause();
            if (SUCCEEDED(hr))
            {
                isPlaying_ = false;
                SetStatus(L"Paused");
                Logf(L"Pause succeeded");
            }
            else
            {
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

        bool ApplyPlaybackSegment()
        {
            segmentStopApplied_ = false;
            if (!player_)
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

            if (IsActiveAbLoop())
            {
                const REFERENCE_TIME stop = LoopBFrameEnd();
                if (stop <= loopA_)
                {
                    Logf(L"Skip AB segment: invalid start/stop A=%s (%I64d), stop=%s (%I64d)",
                         FormatTime(loopA_).c_str(),
                         loopA_,
                         FormatTime(stop).c_str(),
                         stop);
                    SafeRelease(item);
                    return false;
                }

                PROPVARIANT startValue;
                PROPVARIANT stopValue;
                InitInt64PropVariant(startValue, loopA_);
                InitInt64PropVariant(stopValue, stop);
                hr = item->SetStartStopPosition(&MFP_POSITIONTYPE_100NS,
                                                &startValue,
                                                &MFP_POSITIONTYPE_100NS,
                                                &stopValue);
                PropVariantClear(&startValue);
                PropVariantClear(&stopValue);

                segmentStopApplied_ = SUCCEEDED(hr);
                Logf(L"SetStartStopPosition AB returned 0x%08X, start=%s (%I64d), stop=%s (%I64d), nativeStop=%d",
                     static_cast<unsigned int>(hr),
                     FormatTime(loopA_).c_str(),
                     loopA_,
                     FormatTime(stop).c_str(),
                     stop,
                     segmentStopApplied_ ? 1 : 0);
            }
            else
            {
                hr = item->SetStartStopPosition(NULL, NULL, NULL, NULL);
                Logf(L"ClearStartStopPosition returned 0x%08X", static_cast<unsigned int>(hr));
            }

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

            const REFERENCE_TIME rawPos = pos;
            pos = SnapToNearestFrame(pos);
            loopA_ = pos;
            if (loopB_ >= 0 && loopB_ <= loopA_)
            {
                loopB_ = -1;
            }

            UpdateLoopText();
            ApplyPlaybackSegment();
            SetStatus(L"A point set");
            Logf(L"A point set: %s (%I64d), raw=%s (%I64d), frameDuration=%I64d",
                 FormatTime(loopA_).c_str(),
                 loopA_,
                 FormatTime(rawPos).c_str(),
                 rawPos,
                 LoopMinLength());
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

            const REFERENCE_TIME rawPos = pos;
            pos = SnapToNearestFrame(pos);

            if (loopA_ < 0)
            {
                SetStatus(L"Set A before setting B");
                return;
            }

            if (pos <= loopA_)
            {
                SetStatus(L"B must be after A");
                return;
            }

            loopB_ = pos;
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
            UpdateLoopText();
            ApplyPlaybackSegment();
            SetStatus(L"AB loop cleared");
            Logf(L"AB loop cleared");
        }

        void NudgeLoopPoint(bool isA, int frames)
        {
            if (!hasMedia_)
            {
                return;
            }

            REFERENCE_TIME& point = isA ? loopA_ : loopB_;
            if (point < 0)
            {
                SetStatus(isA ? L"Set A first" : L"Set B first");
                return;
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
                    return;
                }
                loopB_ = next;
            }

            UpdateLoopText();
            ApplyPlaybackSegment();
            SeekTo(next, true);
            UpdatePositionUi(next);
            SetStatus(isA ? L"A moved one frame" : L"B moved one frame");
            Logf(L"%s moved by %d frame(s): %s (%I64d)", isA ? L"A" : L"B", frames, FormatTime(next).c_str(), next);
            LogLoopTrigger();
        }

        void OnSeekSlider(WORD code)
        {
            if (!hasMedia_ || duration_ <= 0 || updatingSlider_)
            {
                return;
            }

            if (code == TB_ENDTRACK || code == TB_THUMBPOSITION || code == TB_THUMBTRACK ||
                code == TB_PAGEUP || code == TB_PAGEDOWN || code == TB_LINEUP || code == TB_LINEDOWN)
            {
                const int sliderPos = static_cast<int>(SendMessageW(seekSlider_, TBM_GETPOS, 0, 0));
                REFERENCE_TIME target = duration_ * sliderPos / SLIDER_RANGE;
                SeekTo(target, true);
                UpdatePositionUi(target);
            }
        }

        void SeekTo(REFERENCE_TIME pos, bool keepPlaying)
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

            Logf(L"SeekTo target=%s (%I64d), hr=0x%08X",
                 FormatTime(pos).c_str(),
                 pos,
                 static_cast<unsigned int>(hr));

            if (keepPlaying && isPlaying_)
            {
                HRESULT playHr = player_->Play();
                Logf(L"SeekTo resume Play returned 0x%08X", static_cast<unsigned int>(playHr));
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

        void ReplayFrom(REFERENCE_TIME pos)
        {
            if (!player_)
            {
                return;
            }

            lastLoopReplayTick_ = GetTickCount();
            Logf(L"ReplayFrom: %s (%I64d)", FormatTime(pos).c_str(), pos);
            suppressReplay_ = false;

            if (isPlaying_)
            {
                HRESULT pauseHr = player_->Pause();
                Logf(L"Replay pause-before-seek returned 0x%08X", static_cast<unsigned int>(pauseHr));
            }

            SeekTo(pos, false);
            HRESULT hr = player_->Play();
            Logf(L"Replay Play returned 0x%08X", static_cast<unsigned int>(hr));
            isPlaying_ = SUCCEEDED(hr);
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
            if (isPlaying_ && IsActiveAbLoop() && !segmentStopApplied_ && (now - lastLoopReplayTick_) >= LOOP_REARM_DELAY_MS)
            {
                const REFERENCE_TIME trigger = LoopTriggerPosition();
                if (trigger >= 0 && pos >= trigger)
                {
                    const REFERENCE_TIME bFrameEnd = LoopBFrameEnd();
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
                    ReplayFrom(loopA_);
                    pos = loopA_;
                }
            }

            UpdatePositionUi(pos);
        }

        void UpdatePositionUi(REFERENCE_TIME pos)
        {
            if (duration_ > 0)
            {
                const int sliderPos = static_cast<int>((pos * SLIDER_RANGE) / duration_);
                updatingSlider_ = true;
                SendMessageW(seekSlider_, TBM_SETPOS, TRUE, sliderPos);
                updatingSlider_ = false;
            }

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
            Logf(L"MFPlay window event: event=%s(%d), hr=0x%08X",
                 MediaPlayerEventName(eventType),
                 static_cast<int>(eventType),
                 static_cast<unsigned int>(eventHr));

            if (FAILED(eventHr))
            {
                isPlaying_ = false;
                SetStatus((std::wstring(L"Playback error: ") + HResultText(eventHr)).c_str());
                UpdateControls();
                return;
            }

            switch (eventType)
            {
            case MFP_EVENT_TYPE_PLAY:
                isPlaying_ = true;
                break;
            case MFP_EVENT_TYPE_PAUSE:
            case MFP_EVENT_TYPE_STOP:
                isPlaying_ = false;
                break;
            case MFP_EVENT_TYPE_MEDIAITEM_SET:
                ReadDuration();
                ResizeVideoWindow();
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
                if (hasMedia_)
                {
                    if (isPlaying_)
                    {
                        Pause();
                    }
                    else
                    {
                        Play();
                    }
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
            duration_ = 0;
            loopA_ = -1;
            loopB_ = -1;
            filePath_.clear();

            if (seekSlider_)
            {
                SendMessageW(seekSlider_, TBM_SETPOS, TRUE, 0);
            }
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

        PlayerCallback callback_;
        IMFPMediaPlayer* player_;

        bool hasMedia_;
        bool isPlaying_;
        bool updatingSlider_;
        bool suppressReplay_;
        bool segmentStopApplied_;
        bool isFullScreen_;
        LONG savedStyle_;
        LONG savedExStyle_;
        DWORD lastLoopReplayTick_;
        WINDOWPLACEMENT savedPlacement_;
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
