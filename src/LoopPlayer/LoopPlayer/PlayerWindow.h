#pragma once

#include "LoopPlayerPlatform.h"
#include "MfSourcePlaybackEngine.h"
#include "VideoTimelineProbe.h"

#include <string>

namespace LoopPlayer
{
    /** 播放器主窗口，负责窗口交互、悬浮控制条、AB点、缩放拖动和播放引擎生命周期
    */
    class PlayerWindow
    {
    public:
        /** 构造函数，只做成员默认值初始化
        */
        PlayerWindow();

        /** 析构函数兜底释放播放引擎和字体资源
        */
        ~PlayerWindow();

        /** 创建播放器窗口
        @param [in] hinst 应用实例句柄
        @param [in] cmdShow 初始显示状态
        @return 创建成功返回true，否则返回false
        */
        bool Create(HINSTANCE hinst, int cmdShow);

        /** 获取主窗口句柄
        @return 返回主窗口句柄
        */
        HWND hwnd() const;

        /** 加载媒体文件并自动开始播放
        @param [in] path 媒体文件完整路径
        @return 加载成功返回true，否则返回false
        */
        bool LoadFile(const wchar_t* path);

    private:
        static LRESULT CALLBACK StaticProgressWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticTopOverlayWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticZoomTipWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticVideoWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static DWORD WINAPI StaticSeekWorkerProc(LPVOID param);

        LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

        void RegisterChildClasses();

        void OnCreate();

        HWND CreateButton(const wchar_t* text, int id);

        /** 把楷体字体应用到所有系统子控件
        */
        void ApplyControlFont();

        /** 创建楷体字体对象，调用方负责DeleteObject
        @param [in] height 字体高度
        @param [in] weight 字体粗细
        @return 创建成功返回字体句柄，否则返回NULL
        */
        HFONT CreateUiFont(int height, int weight) const;

        /** 创建顶部加载视频图片按钮的悬浮提示
        */
        void CreateTopLoadToolTip();

        /** 根据顶部浮层当前布局刷新加载按钮悬浮提示区域
        */
        void UpdateTopLoadToolTipRect();

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

        void PaintPauseIndicator(HDC hdc, HWND hwnd, int alpha);

        void ShowZoomTip();

        void HideZoomTip();

        void UpdateZoomTipState();

        void PaintZoomTip(HDC hdc, HWND hwnd);

        /** 读取主显示器布局，rcMonitor保留完整桌面区域，rcWork排除任务栏
        @param [out] info 显示器布局信息
        */
        void GetPrimaryMonitorLayout(MONITORINFO& info) const;

        /** 读取当前窗口所在显示器布局，失败时回退到主显示器布局
        @param [out] info 显示器布局信息
        */
        void GetWindowMonitorLayout(MONITORINFO& info) const;

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

        void DetectFrameDurationFromEngine();

        /** 把异常MP4的原始视频时间轴修正为播放器内部的逻辑时间轴
        @param [in,out] info 需要修正的视频时间轴信息
        @param [out] videoOffset 原始视频时间戳偏移量
        */
        void NormalizeVideoTimelineForPlayback(VideoTimelineInfo& info, REFERENCE_TIME& videoOffset) const;

        void ApplyVideoTimelineDuration(const wchar_t* reason);

        bool GetPosition(REFERENCE_TIME& pos);

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

        /** 把进度条横坐标转换为吸附到视频帧附近的媒体时间
        @param [in] hwnd 进度条窗口
        @param [in] overlay 是否为底部悬浮进度条
        @param [in] x 鼠标在进度条窗口内的横坐标
        @return 媒体时间，单位100ns
        */
        REFERENCE_TIME ProgressPointToTime(HWND hwnd, bool overlay, int x) const;

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

        /** 拖动进度条时节流预览目标帧，避免每个鼠标消息都触发重解码
        @param [in] target 目标媒体时间，单位100ns
        @param [in] force 是否忽略节流立即预览
        */
        void PreviewSeekDragTarget(REFERENCE_TIME target, bool force);

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

        /** 确保后台seek线程已经创建
        @return 创建或已存在返回true，失败返回false
        */
        bool EnsureSeekWorker();

        /** 停止后台seek线程，换视频或退出前调用
        */
        void StopSeekWorker();

        /** 投递一个后台seek请求，线程只处理最新目标
        @param [in] pos 目标媒体时间，单位100ns
        @param [in] keepPlaying seek完成后是否恢复播放
        @param [in] logSeek 是否写详细seek日志
        @param [in] previewMaxReadCount seek后预览最多读取的视频帧数
        @param [in] reason 请求来源说明，供日志排查
        */
        void QueueAsyncSeek(REFERENCE_TIME pos, bool keepPlaying, bool logSeek, size_t previewMaxReadCount, const wchar_t* reason);

        /** 修改正在执行或等待执行的后台seek完成后的播放意图
        @param [in] keepPlaying seek完成后是否播放
        @param [in] reason 请求来源说明，供日志排查
        @return 当前存在后台seek请求时返回true，否则返回false
        */
        bool UpdateAsyncSeekPlaybackIntent(bool keepPlaying, const wchar_t* reason);

        /** 后台seek线程主循环
        @return 线程退出码
        */
        DWORD SeekWorkerProc();

        /** 处理后台seek完成消息
        @param [in] resultParam 后台线程投递的完成结果指针
        */
        void OnAsyncSeekDone(LPARAM resultParam);

        /** 跳转到指定位置，播放恢复由窗口层统一控制
        @param [in] pos 目标媒体时间，单位100ns
        @param [in] keepPlaying seek前窗口处于播放状态时是否恢复播放
        @param [in] logSeek 是否写详细seek日志
        @param [in] previewMaxReadCount seek后预览最多读取的视频帧数，0表示使用引擎默认值
        */
        void SeekTo(REFERENCE_TIME pos, bool keepPlaying, bool logSeek = true, size_t previewMaxReadCount = 0);

        void LogLoopTrigger() const;

        void ReplayFrom(REFERENCE_TIME pos, bool forcePauseBeforeSeek = false);

        void LogPlaybackSnapshot(const wchar_t* reason, REFERENCE_TIME knownPos = -1);

        void OnTimer(bool updateUi = true);

        void UpdatePositionUi(REFERENCE_TIME pos);

        void UpdateTimeText(REFERENCE_TIME pos);

        bool IsLoopEnabled() const;

        void UpdateLoopText();

        void UpdateControls();

        void SetStatus(const wchar_t* status);

        void UpdateTitle();

        void OnPlaybackEngineEvent(PlaybackEngineEvent eventType, HRESULT eventHr);

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
        // 顶部加载视频图片按钮的悬浮提示
        HWND topLoadToolTip_;
        // 所有系统控件统一使用的楷体字体
        HFONT uiFont_;

        // 自定义SourceReader播放引擎
        MfSourcePlaybackEngine* player_;
        // 后台seek线程，避免UI线程被SourceReader补解码阻塞
        HANDLE seekWorkerThread_;
        // 后台seek请求事件
        HANDLE seekRequestEvent_;
        // 后台seek退出事件
        HANDLE seekExitEvent_;
        // 保护后台seek请求状态
        CRITICAL_SECTION seekWorkerLock_;

        bool hasMedia_;
        bool mediaItemReady_;
        bool autoPlayWhenMediaReady_;
        bool isPlaying_;
        bool suppressReplay_;
        bool segmentStopApplied_;
        bool loopReplayPending_;
        bool loopReplayFastAttempt_;
        bool markerSeekPending_;
        bool manualPauseRequest_;
        bool draggingSeek_;
        bool progressMenuActive_;
        bool asyncSeekPending_;
        bool asyncSeekBusy_;
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
        // 底部进度条右键菜单或拖动结束后保持可见到这个tick
        DWORD progressOverlayKeepVisibleUntil_;
        DWORD topOverlayKeepVisibleUntil_;
        DWORD lastMouseActivityTick_;
        DWORD zoomTipHideTick_;
        DWORD loopReplayCount_;
        DWORD topOverlayDragLastApplyTick_;
        DWORD lastPositionLogTick_;
        DWORD lastSeekDragLogTick_;
        // 拖动进度条时上一次真正触发视频预览seek的tick
        DWORD seekDragLastPreviewTick_;
        DWORD asyncSeekSerial_;
        DWORD asyncSeekActiveSerial_;
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
        REFERENCE_TIME asyncSeekTarget_;
        // 拖动进度条时上一次真正触发视频预览seek的位置
        REFERENCE_TIME seekDragLastPreviewTarget_;
        // 拖动进度条时鼠标最新指向的位置
        REFERENCE_TIME seekDragPendingTarget_;
        REFERENCE_TIME duration_;
        REFERENCE_TIME loopA_;
        REFERENCE_TIME loopB_;
        REFERENCE_TIME frameDuration_;
        size_t asyncSeekPreviewMaxReadCount_;
        bool asyncSeekKeepPlaying_;
        bool asyncSeekLog_;
        VideoTimelineInfo videoTimeline_;
        std::wstring filePath_;
        std::wstring playbackPath_;
        std::wstring normalizedMediaPath_;
        std::wstring zoomTipText_;
    };
}