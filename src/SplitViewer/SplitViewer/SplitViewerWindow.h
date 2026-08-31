#pragma once

#include "Layer.h"
#include "LeafHit.h"
#include "SplitterHit.h"

namespace SplitViewer
{
    class SplitViewerWindow
    {
    public:
        SplitViewerWindow();

        ~SplitViewerWindow();

        bool Create(HINSTANCE hinst, int cmdShow);

        HWND hwnd() const;

        bool LoadStartupPath(const wchar_t* path);

    private:
        static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK StaticDragFrameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        void RegisterDragFrameClass();

        LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK LowLevelMouseProc(int code, WPARAM wparam, LPARAM lparam);

        void InstallMouseHook();

        void UninstallMouseHook();

        void UpdateMouseHookState();

        bool NeedsMouseHook() const;

        bool NodeNeedsMouseHook(const Node* node) const;

        HWND EnsureDragFrameWindow(HWND& frameWindow, COLORREF color);

        void ShowDragFrameWindow(HWND& frameWindow, const RECT& screenRect, COLORREF color);

        void HideDragFrameWindow(HWND frameWindow);

        void HideDragOutFrames();

        void DestroyDragFrameWindow(HWND& frameWindow);

        void DestroyDragOutFrames();

        void HideEmbeddedResizePreviewFrames();

        void DestroyEmbeddedResizePreviewFrames();

        void ShowEmbeddedResizePreviews(Node* root, const RECT& rootRect);

        void CollectEmbeddedResizePreviewRects(Node* node, const RECT& rc, std::vector<RECT>& rects) const;

        void OnGlobalMouseEvent(WPARAM mouseMessage, POINT screenPt);

        bool BeginEmbeddedChildInteraction(POINT screenPt);

        void TrackEmbeddedDoubleClick(POINT screenPt);

        bool FindEmbeddedLeafAtScreen(POINT screenPt, LeafHit& hit) const;

        bool EmbeddedDragMovedEnough(POINT screenPt) const;

        bool IsOutsideStageAtScreen(POINT screenPt) const;

        RECT MakeDetachedWindowRect(HWND window, POINT screenPt) const;

        bool GetLeafScreenRect(Node* leaf, int layerIndex, RECT& screenRect) const;

        void UpdateEmbeddedDragOutPreview(POINT screenPt);

        void CompleteEmbeddedDragOut(POINT screenPt);

        void ClearEmbeddedDragOutState();

        void BeginExternalDragProbe(POINT screenPt);

        void UpdateExternalDropTarget(POINT screenPt);

        void CompleteExternalDragProbe(POINT screenPt);

        void ClearExternalDragState(bool invalidate);

        HWND ExternalTopLevelWindowFromPoint(POINT screenPt) const;

        bool IsUsableExternalWindow(HWND window) const;

        bool ExternalDragMovedEnough(POINT screenPt) const;

        bool FindExternalDropLeafAtScreen(POINT screenPt, LeafHit& hit) const;

        void OnEmbedExternalWindow(HWND window);

        POINT MakePoint(LPARAM lparam) const;

        POINT MakeScreenPoint(LPARAM lparam) const;

        void CreateToolbar();

        HWND CreateButton(const wchar_t* text, int id);

        void LayoutToolbar();

        void OnCommand(WORD id);

        void PrimaryConfigAction();

        void UpdatePrimaryButtonText();

        bool HasAnyContent() const;

        bool NodeHasContent(const Node* node) const;

        void OnPaint();

        bool EnsureBackBuffer(HDC referenceDc, int width, int height);

        void ReleaseBackBuffer();

        bool PaintIntersects(const RECT& rc) const;

        bool ShouldUseFastImageRendering() const;

        void DrawWindow(HDC hdc, const RECT& client, bool includeTip);

        void DrawStage(HDC hdc, const RECT& stage);

        void DrawProjectedStage(HDC hdc, const RECT& sourceStage, const RECT& targetStage, bool includeEmbeddedWindows);

        void DrawNode(HDC hdc, Node* node, const RECT& rc, int layerIndex);

        void DrawProjectedNode(HDC hdc, Node* node, const RECT& sourceRect, const RECT& targetRect, int layerIndex, bool includeEmbeddedWindows);

        void DrawLeaf(HDC hdc, Node* node, const RECT& rc);

        void DrawProjectedLeaf(HDC hdc, Node* node, const RECT& sourceRect, const RECT& targetRect, bool includeEmbeddedWindows);

        void DrawPlusButton(HDC hdc, const RECT& rc);

        bool DrawEmbeddedWindowSnapshot(HDC hdc, HWND window, const RECT& targetRect);

        void DrawDropFrame(HDC hdc, const RECT& rc);

        void DrawFrame(HDC hdc, const RECT& rc, COLORREF color);

        void DrawTip(HDC hdc, const RECT& client);

        RECT GetTipRect(const RECT& client) const;

        void FillSolidRect(HDC hdc, const RECT& rc, COLORREF color);

        void InvalidateStage();

        void InvalidateStageRect(const RECT& rc);

        void InvalidateStageRects(const RECT& first, const RECT& second);

        Gdiplus::InterpolationMode CurrentImageInterpolationMode() const;

        RECT GetStageRect() const;

        RECT FitRectToAspect(const RECT& bounds, double aspect) const;

        SIZE GetFullScreenStageSize() const;

        SIZE GetThumbnailStageSize(const RECT& sourceStage) const;

        bool RenderProjectedStageBitmap(const RECT& sourceStage, int width, int height, HBITMAP& bitmapHandle, bool includeEmbeddedWindows);

        bool SaveBitmapToFile(HBITMAP bitmapHandle, const std::wstring& path);

        bool SaveBitmapToPngBytes(HBITMAP bitmapHandle, std::vector<BYTE>& pngBytes);

        RECT ContentRect(const RECT& rc) const;

        RECT PlusButtonRect(const RECT& rc) const;

        RECT GetLayerRect(int index, const RECT& stage) const;

        void SetLayerRectFromStage(int index, const RECT& rc, const RECT& stage);

        void SplitNodeRects(const RECT& rc, Node* node, RECT& firstRect, RECT& splitterRect, RECT& secondRect) const;

        int SplitterSize() const;

        RECT SplitterDrawRect(const RECT& splitterRect, SplitDirection direction, int thickness) const;

        bool GetSplitterVisualRect(Node* root, int layerIndex, Node* splitNode, bool hover, RECT& visualRect) const;

        void UpdateForCurrentStage(bool scaleManualViews);

        void ScaleViewsForStageChange(const RECT& oldStage, const RECT& newStage);

        void ScaleViewsForRectChange(Node* node, const RECT& oldRect, const RECT& newRect);

        void ScaleLeafViewForRectChange(ImageView& view, const RECT& oldRect, const RECT& newRect);

        void SyncAutoFitAll();

        void SyncAutoFitOwner(Node* root, const RECT& ownerRect);

        void SyncAutoFitNode(Node* node, const RECT& rc);

        void SyncEmbeddedWindows();

        void SyncEmbeddedOwner(Node* root, const RECT& ownerRect);

        void SyncEmbeddedNode(Node* node, const RECT& rc);

        double FitImageScale(const ImageView& view, const RECT& rc) const;

        Node* FirstLeaf(Node* node) const;

        bool FindLeafAt(Node* node, const RECT& rc, POINT pt, Node*& leaf, RECT& leafRect) const;

        bool FindSplitterAt(Node* node, const RECT& rc, POINT pt, Node*& splitNode, RECT& nodeRect) const;

        bool FindNodeRect(Node* current, const RECT& rc, Node* target, RECT& targetRect) const;

        bool CanDeleteLeafFromSplit(Node* root, Node* target) const;

        bool DeleteLeafFromSplit(Node* root, Node* target);

        void PromoteSiblingIntoNode(Node* parent, Node* sibling, Node* removed);

        void ClearTransientNodeReferences();

        bool FindTopLayer(POINT pt, int& layerIndex, RECT& layerRect) const;

        bool FindTopLeaf(POINT pt, LeafHit& hit) const;

        bool FindTopSplitter(POINT pt, SplitterHit& hit) const;

        int HitLayerResize(const RECT& layerRect, POINT pt) const;

        void OnLeftButtonDown(POINT pt, WPARAM keyState);

        void OnLeftButtonUp(POINT pt);

        void OnMouseMove(POINT pt, WPARAM keyState);

        void OnRightButtonUp(POINT pt);

        void OnDoubleClick(POINT pt);

        void OnMouseWheel(int delta, WPARAM keyState, POINT screenPt);

        void OnKeyDown(WPARAM key);

        void OnGetMinMaxInfo(MINMAXINFO* info);

        void BeginImageDrag(const LeafHit& hit, POINT pt);

        void BeginSplitterDrag(const SplitterHit& hit, POINT pt);

        void BeginLayerMove(int layerIndex, const RECT& layerRect, POINT pt);

        void BeginLayerResize(int layerIndex, const RECT& layerRect, int resizeHit, POINT pt);

        void UpdateSplitterDrag(POINT pt);

        void UpdateLayerMove(POINT pt);

        void UpdateLayerResize(POINT pt);

        void NormalizeLayerResizeRect(RECT& rc, const RECT& bounds) const;

        void ConstrainRectToBounds(RECT& rc, const RECT& bounds) const;

        void UpdateHover(POINT pt);

        void ClearHover();

        bool ApplyHoverCursor();

        void SetCursorForResizeHit(int hit);

        void TrackMouse();

        void LoadImageForLeaf(const LeafHit& hit);

        bool BrowseOpenImage(std::wstring& path);

        bool BrowseSaveImage(std::wstring& path);

        bool BrowseSaveConfig(std::wstring& path);

        bool BrowseOpenConfig(std::wstring& path);

        void SaveStageImage();

        void SaveConfigDialog();

        void LoadConfigDialog();

        bool SaveConfigToFile(const std::wstring& path);

        int SaveNode(const std::wstring& file, const std::wstring& prefix, Node* node, int& nextId);

        bool LoadConfigFromFile(const std::wstring& path);

        Node* LoadNode(const std::wstring& file, const std::wstring& prefix, int id);

        void ClearLayers();

        void AddLayer();

        void ToggleFullScreen();

        void EnterFullScreen();

        void ExitFullScreen();

        void ShowZoomTip(int layerIndex, const RECT& ownerRect, double scale, double fitScale);

        void OnTipTimer();

    private:
        HINSTANCE hinst_;
        HWND hwnd_;
        HWND saveButton_;
        HWND saveConfigButton_;
        HWND newLayerButton_;
        HWND fullScreenButton_;
        HWND dragOutSourceFrame_;
        HWND dragOutPreviewFrame_;
        std::vector<HWND> embeddedResizePreviewFrames_;
        HHOOK mouseHook_;
        HDC backBufferDc_;
        HBITMAP backBufferBitmap_;
        HGDIOBJ backBufferOldBitmap_;
        int backBufferWidth_;
        int backBufferHeight_;
        bool backBufferValid_;
        Node* baseRoot_;
        std::vector<Layer*> layers_;
        int selectedLayer_;
        bool borderVisible_;
        bool isFullScreen_;
        double stageAspect_;
        int defaultStageWidth_;
        int defaultStageHeight_;
        LONG savedStyle_;
        LONG savedExStyle_;
        WINDOWPLACEMENT savedPlacement_;
        RECT lastStageRect_;
        DragMode dragMode_;
        Node* dragNode_;
        Node* dragRoot_;
        int dragLayerIndex_;
        int dragResizeHit_;
        POINT dragStartPoint_;
        RECT dragLayerStartRect_;
        RECT dragLastInvalidRect_;
        double dragImageOffsetX_;
        double dragImageOffsetY_;
        Node* hoveredSplitter_;
        int hoveredSplitterLayer_;
        int hoverResizeLayer_;
        int hoverResizeHit_;
        bool trackingMouse_;
        HWND externalDragWindow_;
        POINT externalDragStartPoint_;
        RECT externalDragStartRect_;
        Node* externalDropLeaf_;
        HWND pendingEmbedWindow_;
        Node* pendingEmbedLeaf_;
        int pendingEmbedLayer_;
        HWND embeddedDragWindow_;
        Node* embeddedDragLeaf_;
        int embeddedDragLayer_;
        POINT embeddedDragStartPoint_;
        RECT embeddedDragStartRect_;
        DWORD embeddedClickTick_;
        POINT embeddedClickPoint_;
        std::wstring tipText_;
        int tipLayerIndex_;
        RECT tipOwnerRect_;
        RECT paintClipRect_;
        RECT zoomFinishInvalidRect_;
        DWORD tipTick_;
        std::wstring currentConfigPath_;
        std::wstring lastImageSavePath_;
        bool hasPaintClip_;

    private:
        static SplitViewerWindow* hookOwner_;

        SplitViewerWindow(const SplitViewerWindow& other);
        SplitViewerWindow& operator=(const SplitViewerWindow& other);
    };
}
