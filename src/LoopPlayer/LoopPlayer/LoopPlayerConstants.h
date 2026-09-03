#pragma once

#include "LoopPlayerPlatform.h"

namespace LoopPlayer
{
    static const wchar_t kWindowClass[] = L"LoopPlayerWindow";
    static const wchar_t kProgressClass[] = L"LoopPlayerProgress";
    static const wchar_t kOverlayClass[] = L"LoopPlayerOverlay";
    static const wchar_t kTopOverlayClass[] = L"LoopPlayerTopOverlay";
    static const wchar_t kZoomTipClass[] = L"LoopPlayerZoomTip";
    static const wchar_t kVideoClass[] = L"LoopPlayerVideoSurface";
    static const wchar_t kAppTitle[] = L"循环播放器";

    static const UINT WM_PLAYBACK_ENGINE_EVENT = WM_APP + 1;
    static const UINT WM_ASYNC_SEEK_DONE = WM_APP + 2;
    static const UINT_PTR TIMER_POSITION = 1;
    static const UINT_PTR TIMER_VIDEO_CLICK = 2;
    static const UINT_PTR TIMER_TITLE_DRAG = 3;
    static const REFERENCE_TIME ONE_SECOND = 10000000;
    static const REFERENCE_TIME DEFAULT_FRAME_DURATION = ONE_SECOND / 30;
    static const DWORD LOOP_REARM_DELAY_MS = 20;
    static const DWORD LOOP_FAST_REPLAY_VERIFY_MS = 60;
    static const DWORD LOOP_REPLAY_PENDING_TIMEOUT_MS = 750;
    static const DWORD LOOP_ENDED_EVENT_SUPPRESS_MS = 500;
    static const DWORD MARKER_SEEK_PENDING_TIMEOUT_MS = 500;
    static const int LOOP_TRIGGER_EARLY_FRAMES = 3;
    static const int NORMAL_PROGRESS_HEIGHT = 29;
    static const int FULLSCREEN_OVERLAY_HEIGHT = 45;
    static const int FULLSCREEN_OVERLAY_HOT_ZONE = 14;
    static const int OVERLAY_ANIMATION_STEP = 8;
    static const int TOP_OVERLAY_HEIGHT = 27;
    static const int TOP_OVERLAY_HOT_ZONE = 12;
    static const int TOP_OVERLAY_ANIMATION_STEP = 6;
    static const DWORD MOUSE_UI_IDLE_HIDE_MS = 1000;
    static const int PAUSE_INDICATOR_SIZE = 86;
    static const int PAUSE_INDICATOR_MAX_ALPHA = 155;
    static const int PAUSE_INDICATOR_ALPHA_STEP = 12;
    static const DWORD ZOOM_TIP_VISIBLE_MS = 1000;
    static const int ZOOM_TIP_WIDTH = 104;
    static const int ZOOM_TIP_HEIGHT = 42;
    static const int PROGRESS_TIME_LABEL_WIDTH = 60;
    static const int PROGRESS_TIME_LABEL_GAP = 5;
    static const DWORD SEEK_DRAG_PREVIEW_INTERVAL_MS = 120;
    static const size_t SEEK_DRAG_PREVIEW_MAX_READ_COUNT = 120;
    static const size_t SEEK_FINAL_PREVIEW_MAX_READ_COUNT = 180;
    static const int EMPTY_PLUS_BUTTON_SIZE = 72;
    static const int EMPTY_PLUS_LINE_LENGTH = 18;
    static const int PROGRESS_MARKER_NONE = 0;
    static const int PROGRESS_MARKER_A = 1;
    static const int PROGRESS_MARKER_B = 2;
    static const int TOP_BUTTON_NONE = 0;
    static const int TOP_BUTTON_LOAD = 1;
    static const int TOP_BUTTON_MINIMIZE = 2;
    static const int TOP_BUTTON_MAXIMIZE = 3;
    static const int TOP_BUTTON_CLOSE = 4;
    static const int IDM_PROGRESS_SET_A = 40001;
    static const int IDM_PROGRESS_SET_B = 40002;
    static const int IDM_PROGRESS_CLEAR_AB = 40003;

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
        IDC_VIDEO_HOST,
        IDC_VIDEO_PANEL,
        IDC_FULLSCREEN_OVERLAY,
        IDC_ZOOM_TIP
    };
}