#pragma once

#include "LoopPlayerPlatform.h"
#include "PlayerCallback.h"

#include <string>

namespace LoopPlayer
{
    class PlayerWindow
    {
    public:
        PlayerWindow();

        bool Create(HINSTANCE hinst, int cmdShow);

        HWND hwnd() const;

        bool LoadFile(const wchar_t* path);

    private:
        static LRESULT CALLBACK StaticProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticTopOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticZoomTipWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticVideoWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

        void RegisterChildClasses();

        void OnCreate();

        HWND CreateButton(const wchar_t* text, int id);

        bool ReportFailure(const wchar_t* action, HRESULT hr);

        void LayoutControls();

        bool GetOverlayAnchorRect(RECT& screenRect) const;

        void LayoutOverlay();

        void LayoutTopOverlay();

        void LayoutZoomTip();

        RECT GetTopButtonRect(HWND hwnd, int button) const;

        int HitTestTopButton(HWND hwnd, POINT pt) const;

        void PaintTopButton(HDC hdc, HWND hwnd, int button);

        void PaintTopOverlay(HDC hdc, HWND hwnd);

        void BeginTopOverlayDrag(HWND hwnd, POINT pt);

        void UpdateTopOverlayDrag(bool force);

        void EndTopOverlayDrag(bool applyFinal);

        RECT GetEmptyPlusButtonRect(HWND hwnd) const;

        bool HitTestEmptyPlusButton(HWND hwnd, POINT pt) const;

        void PaintEmptyPlusButton(HDC hdc, HWND hwnd);

        bool IsWindowFromThisInstance(HWND window) const;

        void TouchMouseActivity(bool force = false);

        bool IsMouseUiActive(DWORD now) const;

        bool BeginResizeIfNeeded(HWND hwnd, POINT pt);

        void UpdatePauseIndicatorState();

        void PaintPauseIndicator(int alpha);

        void ShowZoomTip();

        void HideZoomTip();

        void UpdateZoomTipState();

        void PaintZoomTip(HDC hdc, HWND hwnd);

        void UpdateOverlayState();

        void UpdateTopOverlayState();

        void ResetVideoTransform();

        void GetBaseVideoSize(int hostW, int hostH, int& baseW, int& baseH) const;

        void ClampVideoPan(int videoW, int videoH, int hostW, int hostH);

        RECT GetVideoRenderRect(int hostW, int hostH);

        void ApplyVideoTransform();

        bool ReadNativeVideoSize();

        void ResizeWindowToVideoDefault();

        void ResizeVideoWindow();

        void ShowControls(int cmdShow);

        void ToggleFullScreen();

        void EnterFullScreen();

        void ExitFullScreen();

        void OnGetMinMaxInfo(MINMAXINFO* info);

        LRESULT OnNcHitTest(int screenX, int screenY) const;

        void OnCommand(WORD id, WORD notifyCode);

        void OpenFileDialog();

        void OnDropFiles(HDROP drop);

        void Play();

        float PlaybackRate() const;

        void FormatPlaybackRate(wchar_t* buffer, size_t count) const;

        bool ApplyPlaybackRate(bool showStatus);

        void AdjustPlaybackRate(int steps);

        void TogglePlayback();

        void Pause();

        void Stop();

        bool ReadDuration();

        void DetectFrameDuration(IMFPMediaItem* item);

        bool GetPosition(REFERENCE_TIME& pos) const;

        REFERENCE_TIME LoopMinLength() const;

        REFERENCE_TIME ClampMediaPosition(REFERENCE_TIME pos) const;

        REFERENCE_TIME SnapToNearestFrame(REFERENCE_TIME pos) const;

        REFERENCE_TIME LoopBFrameEnd() const;

        REFERENCE_TIME LoopTriggerPosition() const;

        bool HasValidAbLoop() const;

        bool IsActiveAbLoop() const;

        REFERENCE_TIME ActiveLoopLength() const;

        bool IsShortActiveAbLoop() const;

        bool ShouldLogLoopReplay() const;

        bool ShouldLogPlayerEvent(MFP_EVENT_TYPE eventType) const;

        bool IsMarkerSeekStillPending();

        bool ApplyPlaybackSegment();

        void SetLoopA();

        void SetLoopAAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos);

        void SetLoopB();

        void SetLoopBAt(REFERENCE_TIME pos, REFERENCE_TIME rawPos);

        void ClearLoop();

        bool NudgeLoopPoint(bool isA, int frames, bool previewFrame = true);

        RECT GetProgressTrackRect(HWND hwnd, bool overlay) const;

        RECT GetOverlayPlayRect(HWND hwnd) const;

        int TimeToProgressX(REFERENCE_TIME pos, const RECT& track) const;

        REFERENCE_TIME ProgressXToTime(int x, const RECT& track) const;

        bool IsPointInProgressArea(HWND hwnd, bool overlay, POINT pt) const;

        int HitTestProgressMarker(HWND hwnd, bool overlay, POINT pt) const;

        void PaintProgressMarker(HDC hdc, HWND hwnd, bool overlay, const RECT& track, int marker, REFERENCE_TIME pos);

        void PaintProgressTrack(HDC hdc, HWND hwnd, bool overlay);

        void PaintOverlay(HDC hdc, HWND hwnd);

        void InvalidateProgressViews();

        void UpdateProgressHover(HWND hwnd, bool overlay, POINT pt);

        void ClearProgressHover(HWND hwnd);

        void MoveCursorToHoveredMarker();

        bool NudgeHoveredMarker(int frames);

        void ClearNativePlaybackSegmentForSeek();

        void SeekFromProgressPoint(HWND hwnd, bool overlay, int x, bool logSeek);

        void BeginSeekDrag(HWND hwnd, bool overlay, POINT pt);

        void ContinueSeekDrag(HWND hwnd, bool overlay, POINT pt);

        void EndSeekDrag(HWND hwnd, bool overlay, POINT pt);

        void ShowProgressContextMenu(HWND hwnd, bool overlay, POINT pt);

        POINT VideoPointToHost(HWND hwnd, POINT pt) const;

        void ZoomVideoAt(POINT hostPt, int wheelSteps, bool fine);

        void BeginVideoDrag(HWND hwnd, POINT pt);

        void ContinueVideoDrag(HWND hwnd, POINT pt);

        void EndVideoDrag(HWND hwnd, POINT pt);

        LRESULT VideoWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT TopOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT ZoomTipWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT ProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool overlay);

        void SeekTo(REFERENCE_TIME pos, bool keepPlaying, bool logSeek = true);

        void LogLoopTrigger() const;

        void ReplayFrom(REFERENCE_TIME pos, bool forcePauseBeforeSeek = false);

        void OnTimer(bool updateUi = true);

        void UpdatePositionUi(REFERENCE_TIME pos);

        void UpdateTimeText(REFERENCE_TIME pos);

        bool IsLoopEnabled() const;

        void UpdateLoopText();

        void UpdateControls();

        void SetStatus(const wchar_t* status);

        void UpdateTitle();

        void OnPlayerEvent(MFP_EVENT_TYPE eventType, HRESULT eventHr);

        void OnKeyDown(WPARAM key);

        void ClosePlayer();

    private:
        HINSTANCE hinst_;
        HWND hwnd_;
        HWND videoHost_;
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
        HWND topOverlayPanel_;
        HWND zoomTipPanel_;

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
        bool topOverlayTargetVisible_;
        bool openFileDialogActive_;
        bool emptyPlusHovered_;
        bool topOverlayDragActive_;
        bool movingOrSizing_;
        bool mouseInsideWindow_;
        bool haveLastMouseActivityScreen_;
        bool pauseIndicatorWasDrawn_;
        LONG savedStyle_;
        LONG savedExStyle_;
        DWORD lastLoopReplayTick_;
        DWORD markerSeekPendingTick_;
        DWORD topOverlayKeepVisibleUntil_;
        DWORD lastMouseActivityTick_;
        DWORD zoomTipHideTick_;
        DWORD loopReplayCount_;
        DWORD topOverlayDragLastApplyTick_;
        int playbackRateTenths_;
        int nativeVideoWidth_;
        int nativeVideoHeight_;
        int videoZoomPercent_;
        int videoPanX_;
        int videoPanY_;
        int pauseIndicatorAlpha_;
        bool videoMouseDown_;
        bool draggingVideo_;
        POINT lastMouseActivityScreen_;
        POINT topOverlayDragStartCursor_;
        POINT videoDragStart_;
        POINT videoDragLast_;
        WINDOWPLACEMENT savedPlacement_;
        RECT topOverlayDragStartWindow_;
        REFERENCE_TIME uiPosition_;
        int overlayVisiblePixels_;
        int topOverlayVisiblePixels_;
        int topHoveredButton_;
        int hoveredMarker_;
        HWND hoverMarkerWindow_;
        REFERENCE_TIME contextMenuPosition_;
        REFERENCE_TIME duration_;
        REFERENCE_TIME loopA_;
        REFERENCE_TIME loopB_;
        REFERENCE_TIME frameDuration_;
        std::wstring filePath_;
        std::wstring zoomTipText_;
    };
}
