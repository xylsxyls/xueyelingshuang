#include "SplitViewerWindow.h"

#include "TempFileGuard.h"



namespace SplitViewer

{

            SplitViewerWindow::SplitViewerWindow()
                : hinst_(NULL),
                  hwnd_(NULL),
                  saveButton_(NULL),
                  saveConfigButton_(NULL),
                  newLayerButton_(NULL),
                  fullScreenButton_(NULL),
                  dragOutSourceFrame_(NULL),
                  dragOutPreviewFrame_(NULL),
                  mouseHook_(NULL),
                  backBufferDc_(NULL),
                  backBufferBitmap_(NULL),
                  backBufferOldBitmap_(NULL),
                  backBufferWidth_(0),
                  backBufferHeight_(0),
                  backBufferValid_(false),
                  baseRoot_(new Node()),
                  selectedLayer_(-1),
                  borderVisible_(true),
                  isFullScreen_(false),
                  stageAspect_(4.0 / 3.0),
                  defaultStageWidth_(640),
                  defaultStageHeight_(360),
                  savedStyle_(0),
                  savedExStyle_(0),
                  dragMode_(DRAG_NONE),
                  dragNode_(NULL),
                  dragRoot_(NULL),
                  dragLayerIndex_(-1),
                  dragResizeHit_(RESIZE_NONE),
                  dragImageOffsetX_(0.0),
                  dragImageOffsetY_(0.0),
                  hoveredSplitter_(NULL),
                  hoveredSplitterLayer_(-2),
                  hoverResizeLayer_(-1),
                  hoverResizeHit_(RESIZE_NONE),
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
        }

    SplitViewerWindow::~SplitViewerWindow()
    {
            UninstallMouseHook();
            ReleaseBackBuffer();
            DestroyEmbeddedResizePreviewFrames();
            delete baseRoot_;
            ClearLayers();
        }

    bool SplitViewerWindow::Create(HINSTANCE hinst, int cmdShow)
    {
            hinst_ = hinst;

            const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            defaultStageWidth_ = std::max(320, screenWidth / 2);
            defaultStageHeight_ = std::max(240, screenHeight / 2);
            stageAspect_ = static_cast<double>(defaultStageWidth_) / static_cast<double>(defaultStageHeight_);

            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
            wc.lpfnWndProc = SplitViewerWindow::StaticWndProc;
            wc.hInstance = hinst_;
            wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
            wc.lpszClassName = kWindowClass;
            wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

            if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }

            RegisterDragFrameClass();

            RECT clientRect = MakeRect(0,
                                       0,
                                       defaultStageWidth_ + kMargin * 2,
                                       kToolbarHeight + defaultStageHeight_ + kMargin * 2);
            DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
            AdjustWindowRectEx(&clientRect, style, FALSE, 0);

            const int windowWidth = RectWidth(clientRect);
            const int windowHeight = RectHeight(clientRect);
            const int x = std::max(0, (screenWidth - windowWidth) / 2);
            const int y = std::max(0, (screenHeight - windowHeight) / 2);

            hwnd_ = CreateWindowExW(0,
                                    kWindowClass,
                                    kAppTitle,
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
                return false;
            }

            ShowWindow(hwnd_, cmdShow);
            UpdateWindow(hwnd_);
            return true;
        }

            bool SplitViewerWindow::LoadStartupPath(const wchar_t* path)
    {
            if (!path || !path[0])
            {
                return false;
            }

            if (IsConfigPath(path))
            {
                return LoadConfigFromFile(path);
            }

            currentConfigPath_.clear();
            Node* leaf = FirstLeaf(baseRoot_);
            if (!leaf || !leaf->view.Load(path))
            {
                return false;
            }

            UpdateForCurrentStage(false);
            UpdatePrimaryButtonText();
            UpdateMouseHookState();
            InvalidateRect(hwnd_, NULL, FALSE);
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

                HBRUSH transparentBrush = CreateSolidBrush(kFrameTransparentColor);
                FillRect(hdc, &rc, transparentBrush);
                DeleteObject(transparentBrush);

                COLORREF color = static_cast<COLORREF>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (color == 0)
                {
                    color = kExternalDragOutFrameColor;
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

    void SplitViewerWindow::RegisterDragFrameClass()
    {
            WNDCLASSEXW wc = { 0 };
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = SplitViewerWindow::StaticDragFrameWndProc;
            wc.hInstance = hinst_;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
            wc.lpszClassName = kDragFrameClass;
            RegisterClassExW(&wc);
        }

    LRESULT SplitViewerWindow::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
    {
            switch (msg)
            {
            case WM_CREATE:
                CreateToolbar();
                UpdateForCurrentStage(false);
                UpdateMouseHookState();
                return 0;
            case WM_SIZE:
                LayoutToolbar();
                UpdateForCurrentStage(true);
                InvalidateRect(hwnd_, NULL, FALSE);
                return 0;
            case WM_COMMAND:
                OnCommand(LOWORD(wparam));
                return 0;
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
                if (wparam == TIMER_TIP)
                {
                    OnTipTimer();
                    return 0;
                }
                break;
            case WM_APP_EMBED_EXTERNAL_WINDOW:
                OnEmbedExternalWindow(reinterpret_cast<HWND>(wparam));
                return 0;
            case WM_APP_TOGGLE_FULLSCREEN:
                ToggleFullScreen();
                return 0;
            case WM_GETMINMAXINFO:
                OnGetMinMaxInfo(reinterpret_cast<MINMAXINFO*>(lparam));
                return 0;
            case WM_DESTROY:
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
                hookOwner_ = NULL;
            }
        }

    void SplitViewerWindow::UninstallMouseHook()
    {
            if (mouseHook_)
            {
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
                InstallMouseHook();
                return;
            }

            if (!embeddedDragWindow_ && !externalDragWindow_)
            {
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

    bool SplitViewerWindow::NodeNeedsMouseHook(const Node* node) const
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
                                          kDragFrameClass,
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
                SetLayeredWindowAttributes(frameWindow, kFrameTransparentColor, 0, LWA_COLORKEY);
            }
            return frameWindow;
        }

    void SplitViewerWindow::ShowDragFrameWindow(HWND& frameWindow, const RECT& screenRect, COLORREF color)
    {
            if (!IsRectUsable(screenRect))
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
                         std::max(1, RectWidth(screenRect)),
                         std::max(1, RectHeight(screenRect)),
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

    void SplitViewerWindow::ShowEmbeddedResizePreviews(Node* root, const RECT& rootRect)
    {
            if (!hwnd_ || !root || !IsRectUsable(rootRect))
            {
                HideEmbeddedResizePreviewFrames();
                return;
            }

            std::vector<RECT> clientRects;
            CollectEmbeddedResizePreviewRects(root, rootRect, clientRects);
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

                RECT screenRect = MakeRect(points[0].x, points[0].y, points[1].x, points[1].y);
                ShowDragFrameWindow(embeddedResizePreviewFrames_[i], screenRect, kExternalDropFrameColor);
            }

            for (size_t i = clientRects.size(); i < embeddedResizePreviewFrames_.size(); ++i)
            {
                HideDragFrameWindow(embeddedResizePreviewFrames_[i]);
            }
        }

    void SplitViewerWindow::CollectEmbeddedResizePreviewRects(Node* node, const RECT& rc, std::vector<RECT>& rects) const
    {
            if (!node || !IsRectUsable(rc))
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
            LeafHit hit;
            if (!FindEmbeddedLeafAtScreen(screenPt, hit))
            {
                return false;
            }

            embeddedDragWindow_ = hit.leaf->view.embeddedWindow;
            embeddedDragLeaf_ = hit.leaf;
            embeddedDragLayer_ = hit.layerIndex;
            embeddedDragStartPoint_ = screenPt;
            GetWindowRect(embeddedDragWindow_, &embeddedDragStartRect_);
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
                embeddedClickTick_ = 0;
                ZeroMemory(&embeddedClickPoint_, sizeof(embeddedClickPoint_));
                PostMessageW(hwnd_, WM_APP_TOGGLE_FULLSCREEN, 0, 0);
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
                ShowDragFrameWindow(dragOutSourceFrame_, sourceRect, kExternalDragOutFrameColor);
            }
            else
            {
                HideDragFrameWindow(dragOutSourceFrame_);
            }

            RECT previewRect = MakeDetachedWindowRect(embeddedDragWindow_, screenPt);
            ShowDragFrameWindow(dragOutPreviewFrame_, previewRect, kExternalDragOutFrameColor);
        }

    void SplitViewerWindow::CompleteEmbeddedDragOut(POINT screenPt)
    {
            HWND window = embeddedDragWindow_;
            Node* leaf = embeddedDragLeaf_;
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
                return;
            }

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
        }

    void SplitViewerWindow::UpdateExternalDropTarget(POINT screenPt)
    {
            Node* nextLeaf = NULL;
            LeafHit hit;
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
            LeafHit hit;
            const bool shouldEmbed = IsWindow(droppedWindow) &&
                                     ExternalDragMovedEnough(screenPt) &&
                                     IsUsableExternalWindow(droppedWindow) &&
                                     FindExternalDropLeafAtScreen(screenPt, hit);

            ClearExternalDragState(true);

            if (shouldEmbed)
            {
                pendingEmbedWindow_ = droppedWindow;
                pendingEmbedLeaf_ = hit.leaf;
                pendingEmbedLayer_ = hit.layerIndex;
                PostMessageW(hwnd_, WM_APP_EMBED_EXTERNAL_WINDOW, reinterpret_cast<WPARAM>(droppedWindow), 0);
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
            Node* targetLeaf = pendingEmbedLeaf_;
            const int targetLayer = pendingEmbedLayer_;
            pendingEmbedWindow_ = NULL;
            pendingEmbedLeaf_ = NULL;
            pendingEmbedLayer_ = -1;

            if (!window || !targetLeaf || !targetLeaf->IsLeaf() || targetLeaf->view.HasContent())
            {
                return;
            }
            if (!IsUsableExternalWindow(window))
            {
                return;
            }

            if (!targetLeaf->view.AttachEmbeddedWindow(window, hwnd_))
            {
                MessageBoxW(hwnd_, L"\u5D4C\u5165\u7A97\u53E3\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return;
            }

            selectedLayer_ = targetLayer;
            SyncEmbeddedWindows();
            UpdatePrimaryButtonText();
            UpdateMouseHookState();
            InvalidateRect(hwnd_, NULL, FALSE);
        }

            void SplitViewerWindow::CreateToolbar()
    {
            saveConfigButton_ = CreateButton(L"\u52A0\u8F7D\u914D\u7F6E", IDC_SAVE_CONFIG);
            saveButton_ = CreateButton(L"\u53E6\u5B58\u56FE\u7247", IDC_SAVE_IMAGE);
            newLayerButton_ = CreateButton(L"\u65B0\u5EFA\u56FE\u5C42", IDC_NEW_LAYER);
            fullScreenButton_ = CreateButton(L"\u5168\u5C4F", IDC_FULLSCREEN);
            LayoutToolbar();
        }

    HWND SplitViewerWindow::CreateButton(const wchar_t* text, int id)
    {
            return CreateWindowExW(0,
                                   L"BUTTON",
                                   text,
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                   0,
                                   0,
                                   10,
                                   10,
                                   hwnd_,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   hinst_,
                                   NULL);
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
            int widths[] = { 86, 86, 86, 62 };
            int x = kMargin;
            const int y = (kToolbarHeight - kButtonHeight) / 2;
            for (int i = 0; i < ARRAYSIZE(buttons); ++i)
            {
                ShowWindow(buttons[i], visible ? SW_SHOW : SW_HIDE);
                SetWindowPos(buttons[i], NULL, x, y, widths[i], kButtonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
                x += widths[i] + kButtonGap;
            }
        }

    void SplitViewerWindow::OnCommand(WORD id)
    {
            switch (id)
            {
            case IDC_SAVE_IMAGE:
                SaveStageImage();
                break;
            case IDC_SAVE_CONFIG:
                PrimaryConfigAction();
                break;
            case IDC_NEW_LAYER:
                AddLayer();
                break;
            case IDC_FULLSCREEN:
                ToggleFullScreen();
                break;
            default:
                break;
            }
        }

    void SplitViewerWindow::PrimaryConfigAction()
    {
            if (HasAnyContent())
            {
                SaveConfigDialog();
            }
            else
            {
                LoadConfigDialog();
            }
        }

    void SplitViewerWindow::UpdatePrimaryButtonText()
    {
            if (!saveConfigButton_)
            {
                return;
            }

            SetWindowTextW(saveConfigButton_, HasAnyContent() ? L"\u4FDD\u5B58" : L"\u52A0\u8F7D\u914D\u7F6E");
        }

            void SplitViewerWindow::OnPaint()
    {
            PAINTSTRUCT ps = { 0 };
            HDC hdc = BeginPaint(hwnd_, &ps);
            RECT client = { 0 };
            GetClientRect(hwnd_, &client);

            const int clientWidth = RectWidth(client);
            const int clientHeight = RectHeight(client);
            if (clientWidth <= 0 || clientHeight <= 0)
            {
                EndPaint(hwnd_, &ps);
                return;
            }

            if (!EnsureBackBuffer(hdc, clientWidth, clientHeight))
            {
                DrawWindow(hdc, client, true);
                EndPaint(hwnd_, &ps);
                return;
            }

            RECT paintRect = ps.rcPaint;
            if (!backBufferValid_ || !IsRectUsable(paintRect))
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
                   RectWidth(paintRect),
                   RectHeight(paintRect),
                   backBufferDc_,
                   paintRect.left,
                   paintRect.top,
                   SRCCOPY);
            backBufferValid_ = true;
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

            backBufferDc_ = CreateCompatibleDC(referenceDc);
            if (!backBufferDc_)
            {
                return false;
            }

            backBufferBitmap_ = CreateCompatibleBitmap(referenceDc, width, height);
            if (!backBufferBitmap_)
            {
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
            if (!IsRectUsable(rc))
            {
                return false;
            }
            if (!hasPaintClip_)
            {
                return true;
            }

            RECT intersectRect = { 0 };
            return IntersectRect(&intersectRect, &paintClipRect_, &rc) && IsRectUsable(intersectRect);
        }

    bool SplitViewerWindow::ShouldUseFastImageRendering() const
    {
            if (dragMode_ != DRAG_NONE)
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
            FillSolidRect(hdc, client, isFullScreen_ ? RGB(18, 18, 18) : kWindowBackColor);

            if (!isFullScreen_)
            {
                RECT toolbar = MakeRect(0, 0, client.right, kToolbarHeight);
                FillSolidRect(hdc, toolbar, kToolbarColor);

                RECT titleRect = toolbar;
                titleRect.left = 420;
                titleRect.right -= kMargin;
                DrawTextW(hdc, kAppTitle, -1, &titleRect, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);
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
            if (!IsRectUsable(stage) || !PaintIntersects(stage))
            {
                return;
            }

            FillSolidRect(hdc, stage, kGrayColor);
            RECT baseContent = ContentRect(stage);
            DrawNode(hdc, baseRoot_, baseContent, -1);
            DrawFrame(hdc, stage, kFrameColor);

            for (size_t i = 0; i < layers_.size(); ++i)
            {
                RECT layerRect = GetLayerRect(static_cast<int>(i), stage);
                if (!IsRectUsable(layerRect) || !PaintIntersects(layerRect))
                {
                    continue;
                }

                FillSolidRect(hdc, layerRect, kGrayColor);
                RECT layerContent = ContentRect(layerRect);
                DrawNode(hdc, layers_[i]->root, layerContent, static_cast<int>(i));
                DrawFrame(hdc, layerRect, selectedLayer_ == static_cast<int>(i) ? kSelectedFrameColor : kFrameColor);
            }
        }

    void SplitViewerWindow::DrawProjectedStage(HDC hdc, const RECT& sourceStage, const RECT& targetStage, bool includeEmbeddedWindows)
    {
            if (!IsRectUsable(sourceStage) || !IsRectUsable(targetStage))
            {
                return;
            }

            FillSolidRect(hdc, targetStage, kGrayColor);

            RECT sourceBase = ContentRect(sourceStage);
            RECT targetBase = ContentRect(targetStage);
            DrawProjectedNode(hdc, baseRoot_, sourceBase, targetBase, -1, includeEmbeddedWindows);
            DrawFrame(hdc, targetStage, kFrameColor);

            for (size_t i = 0; i < layers_.size(); ++i)
            {
                RECT sourceLayer = GetLayerRect(static_cast<int>(i), sourceStage);
                RECT targetLayer = GetLayerRect(static_cast<int>(i), targetStage);
                if (!IsRectUsable(sourceLayer) || !IsRectUsable(targetLayer))
                {
                    continue;
                }

                FillSolidRect(hdc, targetLayer, kGrayColor);
                DrawProjectedNode(hdc,
                                  layers_[i]->root,
                                  ContentRect(sourceLayer),
                                  ContentRect(targetLayer),
                                  static_cast<int>(i),
                                  includeEmbeddedWindows);
                DrawFrame(hdc, targetLayer, selectedLayer_ == static_cast<int>(i) ? kSelectedFrameColor : kFrameColor);
            }
        }

    void SplitViewerWindow::DrawNode(HDC hdc, Node* node, const RECT& rc, int layerIndex)
    {
            if (!node || !IsRectUsable(rc) || !PaintIntersects(rc))
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
                RECT drawRect = SplitterDrawRect(splitterRect, node->direction, hover ? kSplitterHoverSize : SplitterSize());
                if (PaintIntersects(drawRect))
                {
                    FillSolidRect(hdc, drawRect, hover ? kSplitterHoverColor : kSplitterColor);
                }
            }
        }

    void SplitViewerWindow::DrawProjectedNode(HDC hdc, Node* node, const RECT& sourceRect, const RECT& targetRect, int layerIndex, bool includeEmbeddedWindows)
    {
            if (!node || !IsRectUsable(sourceRect) || !IsRectUsable(targetRect))
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
                FillSolidRect(hdc, drawRect, kSplitterColor);
            }
        }

    void SplitViewerWindow::DrawLeaf(HDC hdc, Node* node, const RECT& rc)
    {
            if (!PaintIntersects(rc))
            {
                return;
            }

            FillSolidRect(hdc, rc, kGrayColor);
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
            graphics.SetClip(Gdiplus::Rect(rc.left, rc.top, RectWidth(rc), RectHeight(rc)));
            graphics.SetInterpolationMode(CurrentImageInterpolationMode());
            graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

            Gdiplus::RectF dest(static_cast<Gdiplus::REAL>(centerX - imageWidth * 0.5),
                                static_cast<Gdiplus::REAL>(centerY - imageHeight * 0.5),
                                static_cast<Gdiplus::REAL>(imageWidth),
                                static_cast<Gdiplus::REAL>(imageHeight));
            graphics.DrawImage(node->view.image, dest);
            graphics.ResetClip();
        }

    void SplitViewerWindow::DrawProjectedLeaf(HDC hdc, Node* node, const RECT& sourceRect, const RECT& targetRect, bool includeEmbeddedWindows)
    {
            FillSolidRect(hdc, targetRect, kGrayColor);
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

            const double sourceWidth = static_cast<double>(RectWidth(sourceRect));
            const double sourceHeight = static_cast<double>(RectHeight(sourceRect));
            const double targetWidth = static_cast<double>(RectWidth(targetRect));
            const double targetHeight = static_cast<double>(RectHeight(targetRect));
            if (sourceWidth <= 0.0 || sourceHeight <= 0.0 || targetWidth <= 0.0 || targetHeight <= 0.0)
            {
                return;
            }

            const ImageView& view = node->view;
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
            graphics.SetClip(Gdiplus::Rect(targetRect.left, targetRect.top, RectWidth(targetRect), RectHeight(targetRect)));
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
            if (!IsRectUsable(rc) || !PaintIntersects(rc))
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
            if (!hdc || !window || !IsWindow(window) || !IsRectUsable(targetRect))
            {
                return false;
            }

            const int targetWidth = std::max(1, RectWidth(targetRect));
            const int targetHeight = std::max(1, RectHeight(targetRect));
            RECT originalScreenRect = { 0 };
            GetWindowRect(window, &originalScreenRect);

            POINT originalClientPoint = { originalScreenRect.left, originalScreenRect.top };
            ScreenToClient(hwnd_, &originalClientPoint);
            const int originalWidth = std::max(1, RectWidth(originalScreenRect));
            const int originalHeight = std::max(1, RectHeight(originalScreenRect));
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
                FillSolidRect(memDc, MakeRect(0, 0, targetWidth, targetHeight), kGrayColor);

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
            if (!IsRectUsable(rc) || !PaintIntersects(rc))
            {
                return;
            }

            HPEN pen = CreatePen(PS_SOLID, 3, kExternalDropFrameColor);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

    void SplitViewerWindow::DrawFrame(HDC hdc, const RECT& rc, COLORREF color)
    {
            if (!borderVisible_ || !IsRectUsable(rc) || !PaintIntersects(rc))
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

            HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
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
                return MakeRect(0, 0, 0, 0);
            }

            RECT owner = tipOwnerRect_;
            if (!IsRectUsable(owner))
            {
                owner = client;
            }

            HDC hdc = hwnd_ ? GetDC(hwnd_) : NULL;
            SIZE textSize = { 42, 16 };
            if (hdc)
            {
                HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
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
            left = ClampInt(left, client.left + 4, client.right - width - 4);
            top = ClampInt(top, client.top + 4, client.bottom - height - 4);
            return MakeRect(left, top, left + width, top + height);
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
            if (IsRectUsable(stage))
            {
                InvalidateRect(hwnd_, &stage, FALSE);
                return;
            }

            InvalidateRect(hwnd_, NULL, FALSE);
        }

    void SplitViewerWindow::InvalidateStageRect(const RECT& rc)
    {
            if (!IsRectUsable(rc))
            {
                return;
            }

            RECT stage = GetStageRect();
            RECT invalidRect = { 0 };
            if (IsRectUsable(stage) && IntersectRect(&invalidRect, &stage, &rc) && IsRectUsable(invalidRect))
            {
                InvalidateRect(hwnd_, &invalidRect, FALSE);
            }
        }

    void SplitViewerWindow::InvalidateStageRects(const RECT& first, const RECT& second)
    {
            if (!IsRectUsable(first))
            {
                InvalidateStageRect(second);
                return;
            }
            if (!IsRectUsable(second))
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
            if (dragMode_ != DRAG_NONE)
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
            return GetEncoderClsid(ImageMimeForPath(path), &clsid) >= 0 &&
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
            if (GetEncoderClsid(L"image/png", &clsid) < 0)
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
            if (index < 0 || index >= static_cast<int>(layers_.size()) || !IsRectUsable(stage))
            {
                return;
            }

            const double width = static_cast<double>(RectWidth(stage));
            const double height = static_cast<double>(RectHeight(stage));
            layers_[index]->rect.left = ClampDouble(static_cast<double>(rc.left - stage.left) / width, 0.0, 1.0);
            layers_[index]->rect.top = ClampDouble(static_cast<double>(rc.top - stage.top) / height, 0.0, 1.0);
            layers_[index]->rect.right = ClampDouble(static_cast<double>(rc.right - stage.left) / width, 0.0, 1.0);
            layers_[index]->rect.bottom = ClampDouble(static_cast<double>(rc.bottom - stage.top) / height, 0.0, 1.0);
        }

            void SplitViewerWindow::UpdateForCurrentStage(bool scaleManualViews)
    {
            RECT newStage = GetStageRect();
            if (scaleManualViews &&
                IsRectUsable(lastStageRect_) &&
                IsRectUsable(newStage) &&
                !AreRectsEqual(lastStageRect_, newStage))
            {
                ScaleViewsForStageChange(lastStageRect_, newStage);
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

    void SplitViewerWindow::ScaleViewsForRectChange(Node* node, const RECT& oldRect, const RECT& newRect)
    {
            if (!node || !IsRectUsable(oldRect) || !IsRectUsable(newRect))
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

    void SplitViewerWindow::ScaleLeafViewForRectChange(ImageView& view, const RECT& oldRect, const RECT& newRect)
    {
            if (!view.HasImage() || !IsRectUsable(oldRect) || !IsRectUsable(newRect))
            {
                return;
            }

            const double oldWidth = static_cast<double>(RectWidth(oldRect));
            const double oldHeight = static_cast<double>(RectHeight(oldRect));
            const double newWidth = static_cast<double>(RectWidth(newRect));
            const double newHeight = static_cast<double>(RectHeight(newRect));
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

    void SplitViewerWindow::SyncAutoFitOwner(Node* root, const RECT& ownerRect)
    {
            if (!root || !IsRectUsable(ownerRect))
            {
                return;
            }

            SyncAutoFitNode(root, ownerRect);
            SyncEmbeddedNode(root, ownerRect);
        }

    void SplitViewerWindow::SyncAutoFitNode(Node* node, const RECT& rc)
    {
            if (!node || !IsRectUsable(rc))
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

    void SplitViewerWindow::SyncEmbeddedOwner(Node* root, const RECT& ownerRect)
    {
            if (!root || !IsRectUsable(ownerRect))
            {
                return;
            }

            SyncEmbeddedNode(root, ownerRect);
        }

    void SplitViewerWindow::SyncEmbeddedNode(Node* node, const RECT& rc)
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

                if (!IsRectUsable(rc))
                {
                    ShowWindow(embedded, SW_HIDE);
                    return;
                }

                SetWindowPos(embedded,
                             HWND_TOP,
                             rc.left,
                             rc.top,
                             std::max(1, RectWidth(rc)),
                             std::max(1, RectHeight(rc)),
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
                return;
            }

            RECT firstRect = { 0 };
            RECT splitterRect = { 0 };
            RECT secondRect = { 0 };
            SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);
            SyncEmbeddedNode(node->first, firstRect);
            SyncEmbeddedNode(node->second, secondRect);
        }

            bool SplitViewerWindow::DeleteLeafFromSplit(Node* root, Node* target)
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

    void SplitViewerWindow::PromoteSiblingIntoNode(Node* parent, Node* sibling, Node* removed)
    {
            if (!parent || !sibling || !removed)
            {
                return;
            }

            Node* oldFirst = parent->first;
            Node* oldSecond = parent->second;

            if (sibling->IsLeaf())
            {
                parent->view.TakeFrom(sibling->view);
                parent->kind = NODE_LEAF;
                parent->direction = SPLIT_HORIZONTAL;
                parent->ratio = 0.5;
                parent->first = NULL;
                parent->second = NULL;
            }
            else
            {
                parent->view.Clear();
                parent->kind = NODE_SPLIT;
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
            if (dragMode_ != DRAG_NONE)
            {
                ReleaseCapture();
            }

            dragMode_ = DRAG_NONE;
            dragNode_ = NULL;
            dragRoot_ = NULL;
            dragLayerIndex_ = -1;
            dragResizeHit_ = RESIZE_NONE;
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

            if (!isFullScreen_ && pt.y >= 0 && pt.y < kToolbarHeight)
            {
                ReleaseCapture();
                SendMessageW(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                return;
            }

            RECT stage = GetStageRect();
            if (!PointInRect(stage, pt))
            {
                return;
            }

            int layerIndex = -1;
            RECT layerRect = { 0 };
            if (FindTopLayer(pt, layerIndex, layerRect))
            {
                selectedLayer_ = layerIndex;
                int resizeHit = HitLayerResize(layerRect, pt);
                if (resizeHit != RESIZE_NONE)
                {
                    BeginLayerResize(layerIndex, layerRect, resizeHit, pt);
                    InvalidateStageRect(InflatedRect(layerRect, kLayerHitSize, kLayerHitSize));
                    return;
                }

                if (keyState & MK_CONTROL)
                {
                    BeginLayerMove(layerIndex, layerRect, pt);
                    InvalidateStageRect(InflatedRect(layerRect, kLayerHitSize, kLayerHitSize));
                    return;
                }
            }
            else
            {
                selectedLayer_ = -1;
            }

            SplitterHit splitterHit;
            if (FindTopSplitter(pt, splitterHit))
            {
                BeginSplitterDrag(splitterHit, pt);
                return;
            }

            LeafHit leafHit;
            if (FindTopLeaf(pt, leafHit))
            {
                selectedLayer_ = leafHit.layerIndex;
                if (!leafHit.leaf->view.HasContent() && PointInRect(PlusButtonRect(leafHit.leafRect), pt))
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
            if (dragMode_ != DRAG_NONE)
            {
                const DragMode completedDragMode = dragMode_;
                RECT invalidRect = dragLastInvalidRect_;
                if (completedDragMode == DRAG_LAYER_MOVE || completedDragMode == DRAG_LAYER_RESIZE)
                {
                    RECT startInvalidRect = InflatedRect(dragLayerStartRect_, kLayerHitSize, kLayerHitSize);
                    if (IsRectUsable(startInvalidRect))
                    {
                        if (IsRectUsable(invalidRect))
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
                dragMode_ = DRAG_NONE;
                dragNode_ = NULL;
                dragRoot_ = NULL;
                dragLayerIndex_ = -1;
                dragResizeHit_ = RESIZE_NONE;
                ZeroMemory(&dragLastInvalidRect_, sizeof(dragLastInvalidRect_));
                ZeroMemory(&dragLayerStartRect_, sizeof(dragLayerStartRect_));
                HideEmbeddedResizePreviewFrames();
                if (completedDragMode == DRAG_SPLITTER ||
                    completedDragMode == DRAG_LAYER_MOVE ||
                    completedDragMode == DRAG_LAYER_RESIZE)
                {
                    SyncEmbeddedWindows();
                }
                InvalidateStageRect(invalidRect);
            }
        }

    void SplitViewerWindow::OnMouseMove(POINT pt, WPARAM)
    {
            TrackMouse();

            switch (dragMode_)
            {
            case DRAG_IMAGE:
                if (dragNode_ && dragNode_->IsLeaf())
                {
                    dragNode_->view.offsetX = dragImageOffsetX_ + static_cast<double>(pt.x - dragStartPoint_.x);
                    dragNode_->view.offsetY = dragImageOffsetY_ + static_cast<double>(pt.y - dragStartPoint_.y);
                    InvalidateStageRect(dragLastInvalidRect_);
                }
                return;
            case DRAG_SPLITTER:
                UpdateSplitterDrag(pt);
                return;
            case DRAG_LAYER_MOVE:
                UpdateLayerMove(pt);
                return;
            case DRAG_LAYER_RESIZE:
                UpdateLayerResize(pt);
                return;
            case DRAG_NONE:
            default:
                UpdateHover(pt);
                return;
            }
        }

    void SplitViewerWindow::OnRightButtonUp(POINT pt)
    {
            RECT stage = GetStageRect();
            if (!PointInRect(stage, pt))
            {
                return;
            }

            LeafHit leafHit;
            if (!FindTopLeaf(pt, leafHit))
            {
                return;
            }

            selectedLayer_ = leafHit.layerIndex;
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, IDM_SPLIT_HORIZONTAL, L"\u6C34\u5E73\u5206\u5272");
            AppendMenuW(menu, MF_STRING, IDM_SPLIT_VERTICAL, L"\u5782\u76F4\u5206\u5272");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu,
                        CanDeleteLeafFromSplit(leafHit.root, leafHit.leaf) ? MF_STRING : (MF_STRING | MF_GRAYED),
                        IDM_DELETE_SPLIT,
                        L"\u5220\u9664\u5206\u5C4F");

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

            if (command == IDM_SPLIT_HORIZONTAL || command == IDM_SPLIT_VERTICAL)
            {
                leafHit.leaf->MakeSplit(command == IDM_SPLIT_HORIZONTAL ? SPLIT_HORIZONTAL : SPLIT_VERTICAL);
                SyncAutoFitAll();
                UpdatePrimaryButtonText();
                UpdateMouseHookState();
                InvalidateStage();
            }
            else if (command == IDM_DELETE_SPLIT)
            {
                if (DeleteLeafFromSplit(leafHit.root, leafHit.leaf))
                {
                    ClearTransientNodeReferences();
                    SyncAutoFitAll();
                    UpdatePrimaryButtonText();
                    UpdateMouseHookState();
                    InvalidateStage();
                }
            }
        }

    void SplitViewerWindow::OnDoubleClick(POINT pt)
    {
            RECT stage = GetStageRect();
            if (PointInRect(stage, pt))
            {
                ToggleFullScreen();
            }
        }

    void SplitViewerWindow::OnMouseWheel(int delta, WPARAM keyState, POINT screenPt)
    {
            POINT pt = screenPt;
            ScreenToClient(hwnd_, &pt);

            LeafHit leafHit;
            if (!FindTopLeaf(pt, leafHit) || !leafHit.leaf->view.HasImage())
            {
                return;
            }

            RECT client = { 0 };
            GetClientRect(hwnd_, &client);
            RECT oldTipRect = GetTipRect(client);

            selectedLayer_ = leafHit.layerIndex;
            ImageView& view = leafHit.leaf->view;
            const double fitScale = FitImageScale(view, leafHit.leafRect);
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
            view.scale = ClampDouble(view.scale * factor, minScale, maxScale);

            ShowZoomTip(leafHit.layerIndex, leafHit.ownerRect, view.scale, fitScale);
            RECT newTipRect = GetTipRect(client);
            RECT invalidRect = leafHit.leafRect;
            if (IsRectUsable(oldTipRect))
            {
                UnionRect(&invalidRect, &invalidRect, &oldTipRect);
            }
            if (IsRectUsable(newTipRect))
            {
                UnionRect(&invalidRect, &invalidRect, &newTipRect);
            }

            if (IsRectUsable(zoomFinishInvalidRect_))
            {
                UnionRect(&zoomFinishInvalidRect_, &zoomFinishInvalidRect_, &invalidRect);
            }
            else
            {
                zoomFinishInvalidRect_ = invalidRect;
            }

            InvalidateStageRect(invalidRect);
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
                UpdateForCurrentStage(false);
                InvalidateStage();
                break;
            case VK_F11:
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

    void SplitViewerWindow::BeginImageDrag(const LeafHit& hit, POINT pt)
    {
            dragMode_ = DRAG_IMAGE;
            dragNode_ = hit.leaf;
            dragRoot_ = hit.root;
            dragLayerIndex_ = hit.layerIndex;
            dragStartPoint_ = pt;
            dragImageOffsetX_ = hit.leaf->view.offsetX;
            dragImageOffsetY_ = hit.leaf->view.offsetY;
            dragLastInvalidRect_ = hit.leafRect;
            SetCapture(hwnd_);
            InvalidateStageRect(dragLastInvalidRect_);
        }

    void SplitViewerWindow::BeginSplitterDrag(const SplitterHit& hit, POINT pt)
    {
            dragMode_ = DRAG_SPLITTER;
            dragRoot_ = hit.root;
            dragNode_ = hit.splitNode;
            dragLayerIndex_ = hit.layerIndex;
            dragLastInvalidRect_ = InflatedRect(hit.nodeRect, kSplitterHoverSize, kSplitterHoverSize);
            dragStartPoint_ = pt;
            ShowEmbeddedResizePreviews(dragNode_, hit.nodeRect);
            SetCapture(hwnd_);
        }

    void SplitViewerWindow::BeginLayerMove(int layerIndex, const RECT& layerRect, POINT pt)
    {
            dragMode_ = DRAG_LAYER_MOVE;
            dragLayerIndex_ = layerIndex;
            dragLayerStartRect_ = layerRect;
            dragLastInvalidRect_ = InflatedRect(layerRect, kLayerHitSize, kLayerHitSize);
            dragStartPoint_ = pt;
            ShowEmbeddedResizePreviews(layers_[layerIndex]->root, ContentRect(layerRect));
            SetCapture(hwnd_);
        }

    void SplitViewerWindow::BeginLayerResize(int layerIndex, const RECT& layerRect, int resizeHit, POINT pt)
    {
            dragMode_ = DRAG_LAYER_RESIZE;
            dragLayerIndex_ = layerIndex;
            dragResizeHit_ = resizeHit;
            dragLayerStartRect_ = layerRect;
            dragLastInvalidRect_ = InflatedRect(layerRect, kLayerHitSize, kLayerHitSize);
            dragStartPoint_ = pt;
            ShowEmbeddedResizePreviews(layers_[layerIndex]->root, ContentRect(layerRect));
            SetCapture(hwnd_);
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
            if (dragNode_->direction == SPLIT_HORIZONTAL)
            {
                const int splitterSize = SplitterSize();
                const int available = std::max(2, RectHeight(nodeRect) - splitterSize);
                double ratio = (static_cast<double>(pt.y - nodeRect.top) - splitterSize * 0.5) / static_cast<double>(available);
                const double minRatio = available > kMinPaneSize * 2 ? static_cast<double>(kMinPaneSize) / static_cast<double>(available) : 0.1;
                newRatio = ClampDouble(ratio, minRatio, 1.0 - minRatio);
            }
            else
            {
                const int splitterSize = SplitterSize();
                const int available = std::max(2, RectWidth(nodeRect) - splitterSize);
                double ratio = (static_cast<double>(pt.x - nodeRect.left) - splitterSize * 0.5) / static_cast<double>(available);
                const double minRatio = available > kMinPaneSize * 2 ? static_cast<double>(kMinPaneSize) / static_cast<double>(available) : 0.1;
                newRatio = ClampDouble(ratio, minRatio, 1.0 - minRatio);
            }

            if (std::fabs(newRatio - dragNode_->ratio) < 0.000001)
            {
                return;
            }

            dragNode_->ratio = newRatio;
            SyncAutoFitNode(dragNode_, nodeRect);
            ShowEmbeddedResizePreviews(dragNode_, nodeRect);
            dragLastInvalidRect_ = InflatedRect(nodeRect, kSplitterHoverSize, kSplitterHoverSize);
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
            RECT newInvalidRect = InflatedRect(layerRect, kLayerHitSize, kLayerHitSize);
            if (AreRectsEqual(dragLastInvalidRect_, newInvalidRect))
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

            if (dragResizeHit_ & RESIZE_LEFT)
            {
                rc.left += dx;
            }
            if (dragResizeHit_ & RESIZE_RIGHT)
            {
                rc.right += dx;
            }
            if (dragResizeHit_ & RESIZE_TOP)
            {
                rc.top += dy;
            }
            if (dragResizeHit_ & RESIZE_BOTTOM)
            {
                rc.bottom += dy;
            }

            NormalizeLayerResizeRect(rc, stage);
            SetLayerRectFromStage(dragLayerIndex_, rc, stage);
            RECT layerRect = GetLayerRect(dragLayerIndex_, stage);
            RECT newInvalidRect = InflatedRect(layerRect, kLayerHitSize, kLayerHitSize);
            if (AreRectsEqual(dragLastInvalidRect_, newInvalidRect))
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
            int resizeHit = RESIZE_NONE;
            RECT layerRect = { 0 };
            if (FindTopLayer(pt, resizeLayer, layerRect))
            {
                resizeHit = HitLayerResize(layerRect, pt);
            }

            Node* splitNode = NULL;
            int splitLayer = -2;
            if (resizeHit == RESIZE_NONE)
            {
                SplitterHit splitterHit;
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
                Node* oldRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
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
                    Node* newRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
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
            if (hoveredSplitter_ || hoverResizeHit_ != RESIZE_NONE)
            {
                RECT oldSplitterRect = { 0 };
                bool hasOldSplitterRect = false;
                if (hoveredSplitter_)
                {
                    Node* oldRoot = hoveredSplitterLayer_ >= 0 && hoveredSplitterLayer_ < static_cast<int>(layers_.size()) ?
                                    layers_[hoveredSplitterLayer_]->root :
                                    baseRoot_;
                    hasOldSplitterRect = GetSplitterVisualRect(oldRoot, hoveredSplitterLayer_, hoveredSplitter_, true, oldSplitterRect);
                }

                hoveredSplitter_ = NULL;
                hoveredSplitterLayer_ = -2;
                hoverResizeLayer_ = -1;
                hoverResizeHit_ = RESIZE_NONE;
                if (hasOldSplitterRect)
                {
                    InvalidateStageRect(oldSplitterRect);
                }
            }
        }

    bool SplitViewerWindow::ApplyHoverCursor()
    {
            if (dragMode_ == DRAG_SPLITTER && dragNode_)
            {
                SetCursor(LoadCursor(NULL, dragNode_->direction == SPLIT_HORIZONTAL ? IDC_SIZENS : IDC_SIZEWE));
                return true;
            }
            if (dragMode_ == DRAG_LAYER_MOVE)
            {
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                return true;
            }
            if (dragMode_ == DRAG_LAYER_RESIZE)
            {
                SetCursorForResizeHit(dragResizeHit_);
                return true;
            }
            if (hoverResizeHit_ != RESIZE_NONE)
            {
                SetCursorForResizeHit(hoverResizeHit_);
                return true;
            }
            if (hoveredSplitter_)
            {
                SetCursor(LoadCursor(NULL, hoveredSplitter_->direction == SPLIT_HORIZONTAL ? IDC_SIZENS : IDC_SIZEWE));
                return true;
            }
            return false;
        }

    void SplitViewerWindow::SetCursorForResizeHit(int hit)
    {
            if ((hit & RESIZE_LEFT) && (hit & RESIZE_TOP))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            }
            else if ((hit & RESIZE_RIGHT) && (hit & RESIZE_BOTTOM))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            }
            else if ((hit & RESIZE_RIGHT) && (hit & RESIZE_TOP))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZENESW));
            }
            else if ((hit & RESIZE_LEFT) && (hit & RESIZE_BOTTOM))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZENESW));
            }
            else if (hit & (RESIZE_LEFT | RESIZE_RIGHT))
            {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            }
            else if (hit & (RESIZE_TOP | RESIZE_BOTTOM))
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

    void SplitViewerWindow::LoadImageForLeaf(const LeafHit& hit)
    {
            std::wstring path;
            if (!BrowseOpenImage(path))
            {
                return;
            }

            if (!hit.leaf->view.Load(path))
            {
                MessageBoxW(hwnd_, L"\u56FE\u7247\u52A0\u8F7D\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return;
            }

            SyncAutoFitAll();
            UpdatePrimaryButtonText();
            UpdateMouseHookState();
            InvalidateStage();
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
                return false;
            }

            path = fileName;
            return true;
        }

    bool SplitViewerWindow::BrowseSaveImage(std::wstring& path)
    {
            wchar_t fileName[4096] = { 0 };
            std::wstring defaultPath = lastImageSavePath_;
            if (defaultPath.empty())
            {
                defaultPath = MakeTimestampName() + L".png";
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
                return false;
            }

            path = WithDefaultExtension(fileName, L".png");
            return true;
        }

    bool SplitViewerWindow::BrowseSaveConfig(std::wstring& path)
    {
            wchar_t fileName[4096] = { 0 };
            std::wstring defaultPath = currentConfigPath_;
            if (defaultPath.empty())
            {
                defaultPath = MakeTimestampName() + L".sv";
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
                return false;
            }

            path = WithDefaultExtension(fileName, L".sv");
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
                return false;
            }

            path = fileName;
            return true;
        }

    void SplitViewerWindow::SaveStageImage()
    {
            std::wstring path;
            if (!BrowseSaveImage(path))
            {
                return;
            }

            RECT stage = GetStageRect();
            SIZE fullSize = GetFullScreenStageSize();
            if (!IsRectUsable(stage) || fullSize.cx <= 0 || fullSize.cy <= 0)
            {
                return;
            }

            HBITMAP bitmapHandle = NULL;
            if (!RenderProjectedStageBitmap(stage, fullSize.cx, fullSize.cy, bitmapHandle, true) ||
                !SaveBitmapToFile(bitmapHandle, path))
            {
                MessageBoxW(hwnd_, L"\u622A\u56FE\u4FDD\u5B58\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
            }
            else
            {
                lastImageSavePath_ = path;
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
                return;
            }

            if (SaveConfigToFile(path))
            {
                currentConfigPath_ = path;
                UpdatePrimaryButtonText();
            }
        }

    void SplitViewerWindow::LoadConfigDialog()
    {
            std::wstring path;
            if (BrowseOpenConfig(path))
            {
                LoadConfigFromFile(path);
            }
        }

    bool SplitViewerWindow::SaveConfigToFile(const std::wstring& path)
    {
            TempFileGuard profileTemp;
            if (!CreateTempProfilePath(profileTemp.path) || !CreateUtf16ProfileFile(profileTemp.path))
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
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
            WriteProfileString(profilePath, L"SplitViewer", L"Version", L"2");
            WriteProfileDouble(profilePath, L"SplitViewer", L"StageAspect", stageAspect_);
            WriteProfileInt(profilePath, L"SplitViewer", L"BorderVisible", borderVisible_ ? 1 : 0);
            WriteProfileInt(profilePath, L"SplitViewer", L"SelectedLayer", selectedLayer_);
            WriteProfileInt(profilePath, L"SplitViewer", L"LayerCount", static_cast<int>(layers_.size()));
            WriteProfileInt(profilePath, L"SplitViewer", L"StageWidth", RectWidth(stage));
            WriteProfileInt(profilePath, L"SplitViewer", L"StageHeight", RectHeight(stage));

            WriteProfileInt(profilePath, L"Window", L"Left", windowRect.left);
            WriteProfileInt(profilePath, L"Window", L"Top", windowRect.top);
            WriteProfileInt(profilePath, L"Window", L"Right", windowRect.right);
            WriteProfileInt(profilePath, L"Window", L"Bottom", windowRect.bottom);

            int baseNextId = 0;
            const int baseRootId = SaveNode(profilePath, L"Base", baseRoot_, baseNextId);
            WriteProfileInt(profilePath, L"Base", L"Root", baseRootId);
            WriteProfileInt(profilePath, L"Base", L"NodeCount", baseNextId);

            for (size_t i = 0; i < layers_.size(); ++i)
            {
                std::wstring layerSection = std::wstring(L"Layer") + FormatInt(static_cast<int>(i));
                WriteProfileDouble(profilePath, layerSection, L"Left", layers_[i]->rect.left);
                WriteProfileDouble(profilePath, layerSection, L"Top", layers_[i]->rect.top);
                WriteProfileDouble(profilePath, layerSection, L"Right", layers_[i]->rect.right);
                WriteProfileDouble(profilePath, layerSection, L"Bottom", layers_[i]->rect.bottom);

                int nextId = 0;
                std::wstring prefix = layerSection + L"_";
                const int rootId = SaveNode(profilePath, prefix, layers_[i]->root, nextId);
                WriteProfileInt(profilePath, layerSection, L"Root", rootId);
                WriteProfileInt(profilePath, layerSection, L"NodeCount", nextId);
            }

            WritePrivateProfileStringW(NULL, NULL, NULL, profilePath.c_str());

            std::vector<BYTE> configBytes;
            if (!ReadFileBytes(profilePath, configBytes))
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
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

            if (!thumbnailOk || !WriteConfigPackage(path, thumbnailPng, configBytes))
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u521B\u5EFA\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return false;
            }

            return true;
        }

    int SplitViewerWindow::SaveNode(const std::wstring& file, const std::wstring& prefix, Node* node, int& nextId)
    {
            const int id = nextId++;
            const std::wstring section = ProfileSectionName(prefix, id);

            if (!node || node->IsLeaf())
            {
                WriteProfileString(file, section, L"Kind", L"Leaf");
                if (node)
                {
                    WriteProfileString(file, section, L"Path", node->view.path);
                    WriteProfileInt(file, section, L"HasImage", node->view.HasImage() ? 1 : 0);
                    WriteProfileInt(file, section, L"AutoFit", node->view.autoFit ? 1 : 0);
                    WriteProfileDouble(file, section, L"Scale", node->view.scale);
                    WriteProfileDouble(file, section, L"OffsetX", node->view.offsetX);
                    WriteProfileDouble(file, section, L"OffsetY", node->view.offsetY);
                }
                return id;
            }

            WriteProfileString(file, section, L"Kind", L"Split");
            WriteProfileInt(file, section, L"Direction", node->direction == SPLIT_HORIZONTAL ? 0 : 1);
            WriteProfileDouble(file, section, L"Ratio", node->ratio);
            const int firstId = SaveNode(file, prefix, node->first, nextId);
            const int secondId = SaveNode(file, prefix, node->second, nextId);
            WriteProfileInt(file, section, L"First", firstId);
            WriteProfileInt(file, section, L"Second", secondId);
            return id;
        }

    bool SplitViewerWindow::LoadConfigFromFile(const std::wstring& path)
    {
            if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u4E0D\u5B58\u5728\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return false;
            }

            TempFileGuard tempProfile;
            std::wstring profilePath;
            if (!PrepareProfileReadFile(path, profilePath, tempProfile))
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u8BFB\u53D6\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return false;
            }

            if (isFullScreen_)
            {
                ExitFullScreen();
            }

            Node* newBase = LoadNode(profilePath, L"Base", ReadProfileIntValue(profilePath, L"Base", L"Root", 0));
            if (!newBase)
            {
                MessageBoxW(hwnd_, L"\u914D\u7F6E\u6587\u4EF6\u8BFB\u53D6\u5931\u8D25\u3002", kAppTitle, MB_OK | MB_ICONERROR);
                return false;
            }

            std::vector<Layer*> newLayers;
            const int layerCount = ClampInt(ReadProfileIntValue(profilePath, L"SplitViewer", L"LayerCount", 0), 0, 256);
            for (int i = 0; i < layerCount; ++i)
            {
                std::wstring layerSection = std::wstring(L"Layer") + FormatInt(i);
                Layer* layer = new Layer();
                layer->rect.left = ClampDouble(ReadProfileDoubleValue(profilePath, layerSection, L"Left", layer->rect.left), 0.0, 1.0);
                layer->rect.top = ClampDouble(ReadProfileDoubleValue(profilePath, layerSection, L"Top", layer->rect.top), 0.0, 1.0);
                layer->rect.right = ClampDouble(ReadProfileDoubleValue(profilePath, layerSection, L"Right", layer->rect.right), 0.0, 1.0);
                layer->rect.bottom = ClampDouble(ReadProfileDoubleValue(profilePath, layerSection, L"Bottom", layer->rect.bottom), 0.0, 1.0);

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
                layer->root = LoadNode(profilePath, prefix, ReadProfileIntValue(profilePath, layerSection, L"Root", 0));
                if (!layer->root)
                {
                    layer->root = new Node();
                }
                newLayers.push_back(layer);
            }

            delete baseRoot_;
            baseRoot_ = newBase;
            ClearLayers();
            layers_.swap(newLayers);
            ZeroMemory(&lastStageRect_, sizeof(lastStageRect_));

            stageAspect_ = ReadProfileDoubleValue(profilePath, L"SplitViewer", L"StageAspect", stageAspect_);
            if (stageAspect_ < 0.1)
            {
                stageAspect_ = 4.0 / 3.0;
            }
            borderVisible_ = ReadProfileIntValue(profilePath, L"SplitViewer", L"BorderVisible", 1) != 0;
            selectedLayer_ = ReadProfileIntValue(profilePath, L"SplitViewer", L"SelectedLayer", -1);
            if (selectedLayer_ >= static_cast<int>(layers_.size()))
            {
                selectedLayer_ = -1;
            }

            const int left = ReadProfileIntValue(profilePath, L"Window", L"Left", CW_USEDEFAULT);
            const int top = ReadProfileIntValue(profilePath, L"Window", L"Top", CW_USEDEFAULT);
            const int right = ReadProfileIntValue(profilePath, L"Window", L"Right", CW_USEDEFAULT);
            const int bottom = ReadProfileIntValue(profilePath, L"Window", L"Bottom", CW_USEDEFAULT);
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
            return true;
        }

    Node* SplitViewerWindow::LoadNode(const std::wstring& file, const std::wstring& prefix, int id)
    {
            const std::wstring section = ProfileSectionName(prefix, id);
            const std::wstring kind = ReadProfileString(file, section, L"Kind", L"Leaf");
            Node* node = new Node();

            if (SameText(kind.c_str(), L"Split"))
            {
                node->kind = NODE_SPLIT;
                node->direction = ReadProfileIntValue(file, section, L"Direction", 0) == 0 ? SPLIT_HORIZONTAL : SPLIT_VERTICAL;
                node->ratio = ClampDouble(ReadProfileDoubleValue(file, section, L"Ratio", 0.5), 0.02, 0.98);
                delete node->first;
                delete node->second;
                node->first = LoadNode(file, prefix, ReadProfileIntValue(file, section, L"First", 0));
                node->second = LoadNode(file, prefix, ReadProfileIntValue(file, section, L"Second", 0));
                if (!node->first)
                {
                    node->first = new Node();
                }
                if (!node->second)
                {
                    node->second = new Node();
                }
                return node;
            }

            const std::wstring imagePath = ReadProfileString(file, section, L"Path", L"");
            if (!imagePath.empty())
            {
                if (!node->view.Load(imagePath))
                {
                    node->view.Clear();
                }
            }
            node->view.autoFit = ReadProfileIntValue(file, section, L"AutoFit", 1) != 0;
            node->view.scale = ReadProfileDoubleValue(file, section, L"Scale", node->view.scale);
            node->view.offsetX = ReadProfileDoubleValue(file, section, L"OffsetX", 0.0);
            node->view.offsetY = ReadProfileDoubleValue(file, section, L"OffsetY", 0.0);
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
            Layer* layer = new Layer();
            const double offset = 0.03 * static_cast<double>(layers_.size() % 6);
            layer->rect.left = ClampDouble(0.22 + offset, 0.0, 0.72);
            layer->rect.top = ClampDouble(0.22 + offset, 0.0, 0.72);
            layer->rect.right = ClampDouble(layer->rect.left + 0.46, 0.25, 1.0);
            layer->rect.bottom = ClampDouble(layer->rect.top + 0.46, 0.25, 1.0);
            layers_.push_back(layer);
            selectedLayer_ = static_cast<int>(layers_.size()) - 1;
            UpdateForCurrentStage(true);
            UpdatePrimaryButtonText();
            UpdateMouseHookState();
            InvalidateRect(hwnd_, NULL, FALSE);
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

            SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_ & ~(WS_CAPTION | WS_THICKFRAME));
            SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_ & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
            LayoutToolbar();
            SetWindowPos(hwnd_,
                         HWND_TOP,
                         monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.top,
                         RectWidth(monitorInfo.rcMonitor),
                         RectHeight(monitorInfo.rcMonitor),
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            UpdateForCurrentStage(true);
            InvalidateRect(hwnd_, NULL, FALSE);
        }

    void SplitViewerWindow::ExitFullScreen()
    {
            if (!isFullScreen_)
            {
                return;
            }

            isFullScreen_ = false;
            SetWindowLongW(hwnd_, GWL_STYLE, savedStyle_);
            SetWindowLongW(hwnd_, GWL_EXSTYLE, savedExStyle_);
            SetWindowPlacement(hwnd_, &savedPlacement_);
            LayoutToolbar();
            SetWindowPos(hwnd_, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            UpdateForCurrentStage(true);
            InvalidateRect(hwnd_, NULL, FALSE);
        }

    void SplitViewerWindow::ShowZoomTip(int layerIndex, const RECT& ownerRect, double scale, double fitScale)
    {
            const double percent = fitScale > 0.00001 ? scale / fitScale * 100.0 : 100.0;
            wchar_t buffer[64] = { 0 };
            StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%.0f%%", percent);
            tipText_ = buffer;
            tipLayerIndex_ = layerIndex;
            tipOwnerRect_ = ownerRect;
            tipTick_ = GetTickCount();
            SetTimer(hwnd_, TIMER_TIP, 1000, NULL);
        }

    void SplitViewerWindow::OnTipTimer()
    {
            if (!tipText_.empty() && GetTickCount() - tipTick_ >= 1000)
            {
                RECT client = { 0 };
                GetClientRect(hwnd_, &client);
                RECT invalidRect = zoomFinishInvalidRect_;
                RECT tipRect = GetTipRect(client);
                if (IsRectUsable(tipRect))
                {
                    if (IsRectUsable(invalidRect))
                    {
                        UnionRect(&invalidRect, &invalidRect, &tipRect);
                    }
                    else
                    {
                        invalidRect = tipRect;
                    }
                }

                tipText_.clear();
                KillTimer(hwnd_, TIMER_TIP);
                ZeroMemory(&zoomFinishInvalidRect_, sizeof(zoomFinishInvalidRect_));
                InvalidateStageRect(invalidRect);
            }
        }

    HWND SplitViewerWindow::hwnd() const
    {
        return hwnd_;
    }

    bool SplitViewerWindow::FindEmbeddedLeafAtScreen(POINT screenPt, LeafHit& hit) const
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
        return std::abs(screenPt.x - embeddedDragStartPoint_.x) >= kExternalDropMoveThreshold ||
               std::abs(screenPt.y - embeddedDragStartPoint_.y) >= kExternalDropMoveThreshold;
    }

    bool SplitViewerWindow::IsOutsideStageAtScreen(POINT screenPt) const
    {
        POINT clientPt = screenPt;
        ScreenToClient(hwnd_, &clientPt);
        RECT stage = GetStageRect();
        return !PointInRect(stage, clientPt);
    }

    RECT SplitViewerWindow::MakeDetachedWindowRect(HWND window, POINT screenPt) const
    {
        RECT current = { 0 };
        if (!window || !IsWindow(window) || !GetWindowRect(window, &current))
        {
            current = MakeRect(screenPt.x, screenPt.y, screenPt.x + 640, screenPt.y + 360);
        }

        const int width = std::max(160, RectWidth(current));
        const int height = std::max(120, RectHeight(current));
        const int captionOffset = std::min(height / 2, std::max(24, GetSystemMetrics(SM_CYCAPTION)));
        RECT rc = MakeRect(screenPt.x - width / 2,
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

    bool SplitViewerWindow::GetLeafScreenRect(Node* leaf, int layerIndex, RECT& screenRect) const
    {
        if (!leaf || !hwnd_)
        {
            return false;
        }

        RECT stage = GetStageRect();
        RECT rootRect = ContentRect(stage);
        Node* root = baseRoot_;
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
        screenRect = MakeRect(topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
        return IsRectUsable(screenRect);
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
        if (dx >= kExternalDropMoveThreshold || dy >= kExternalDropMoveThreshold)
        {
            return true;
        }

        RECT currentRect = { 0 };
        if (externalDragWindow_ && IsWindow(externalDragWindow_) && GetWindowRect(externalDragWindow_, &currentRect))
        {
            return std::abs(currentRect.left - externalDragStartRect_.left) >= kExternalDropMoveThreshold ||
                   std::abs(currentRect.top - externalDragStartRect_.top) >= kExternalDropMoveThreshold;
        }

        return false;
    }

    bool SplitViewerWindow::FindExternalDropLeafAtScreen(POINT screenPt, LeafHit& hit) const
    {
        POINT clientPt = screenPt;
        ScreenToClient(hwnd_, &clientPt);

        LeafHit found;
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

    bool SplitViewerWindow::NodeHasContent(const Node* node) const
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
            available.left += kMargin;
            available.right -= kMargin;
            available.top += kToolbarHeight + kMargin;
            available.bottom -= kMargin;
        }

        if (RectWidth(available) <= 0 || RectHeight(available) <= 0)
        {
            return MakeRect(0, 0, 0, 0);
        }

        return FitRectToAspect(available, stageAspect_);
    }

    RECT SplitViewerWindow::FitRectToAspect(const RECT& bounds, double aspect) const
    {
        const int boundsWidth = RectWidth(bounds);
        const int boundsHeight = RectHeight(bounds);
        int width = boundsWidth;
        int height = static_cast<int>(static_cast<double>(width) / aspect + 0.5);
        if (height > boundsHeight)
        {
            height = boundsHeight;
            width = static_cast<int>(static_cast<double>(height) * aspect + 0.5);
        }

        const int left = bounds.left + (boundsWidth - width) / 2;
        const int top = bounds.top + (boundsHeight - height) / 2;
        return MakeRect(left, top, left + width, top + height);
    }

    SIZE SplitViewerWindow::GetFullScreenStageSize() const
    {
        MONITORINFO monitorInfo = { 0 };
        monitorInfo.cbSize = sizeof(monitorInfo);
        GetMonitorInfoW(MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &monitorInfo);

        RECT monitorRect = MakeRect(0,
                                    0,
                                    RectWidth(monitorInfo.rcMonitor),
                                    RectHeight(monitorInfo.rcMonitor));
        RECT fullStage = FitRectToAspect(monitorRect, stageAspect_);

        SIZE size = { std::max(1, RectWidth(fullStage)), std::max(1, RectHeight(fullStage)) };
        return size;
    }

    SIZE SplitViewerWindow::GetThumbnailStageSize(const RECT& sourceStage) const
    {
        const int sourceWidth = std::max(1, RectWidth(sourceStage));
        const int sourceHeight = std::max(1, RectHeight(sourceStage));
        double scale = std::min(static_cast<double>(kConfigThumbnailMaxEdge) / static_cast<double>(sourceWidth),
                                static_cast<double>(kConfigThumbnailMaxEdge) / static_cast<double>(sourceHeight));
        scale = std::min(1.0, std::max(0.01, scale));

        SIZE size = {
            std::max(1, static_cast<int>(sourceWidth * scale + 0.5)),
            std::max(1, static_cast<int>(sourceHeight * scale + 0.5))
        };
        return size;
    }

    bool SplitViewerWindow::RenderProjectedStageBitmap(const RECT& sourceStage, int width, int height, HBITMAP& bitmapHandle, bool includeEmbeddedWindows)
    {
        bitmapHandle = NULL;
        if (!IsRectUsable(sourceStage) || width <= 0 || height <= 0)
        {
            return false;
        }

        HDC screenDc = GetDC(hwnd_);
        HDC memDc = screenDc ? CreateCompatibleDC(screenDc) : NULL;
        bitmapHandle = screenDc ? CreateCompatibleBitmap(screenDc, width, height) : NULL;
        if (!screenDc || !memDc || !bitmapHandle)
        {
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
        RECT targetStage = MakeRect(0, 0, width, height);
        DrawProjectedStage(memDc, sourceStage, targetStage, includeEmbeddedWindows);
        SelectObject(memDc, oldBitmap);

        DeleteDC(memDc);
        ReleaseDC(hwnd_, screenDc);
        return true;
    }

    RECT SplitViewerWindow::ContentRect(const RECT& rc) const
    {
        RECT result = rc;
        if (borderVisible_ && RectWidth(result) > 2 && RectHeight(result) > 2)
        {
            InflateRect(&result, -1, -1);
        }
        return result;
    }

    RECT SplitViewerWindow::PlusButtonRect(const RECT& rc) const
    {
        const int size = std::min(kPlusButtonSize, std::min(RectWidth(rc), RectHeight(rc)) - 8);
        if (size <= 8)
        {
            return MakeRect(0, 0, 0, 0);
        }

        const int left = rc.left + (RectWidth(rc) - size) / 2;
        const int top = rc.top + (RectHeight(rc) - size) / 2;
        return MakeRect(left, top, left + size, top + size);
    }

    RECT SplitViewerWindow::GetLayerRect(int index, const RECT& stage) const
    {
        if (index < 0 || index >= static_cast<int>(layers_.size()))
        {
            return MakeRect(0, 0, 0, 0);
        }

        const FloatRect& nr = layers_[index]->rect;
        const int width = RectWidth(stage);
        const int height = RectHeight(stage);
        RECT rc = MakeRect(stage.left + static_cast<int>(nr.left * width + 0.5),
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

    void SplitViewerWindow::SplitNodeRects(const RECT& rc, Node* node, RECT& firstRect, RECT& splitterRect, RECT& secondRect) const
    {
        if (!node || node->IsLeaf())
        {
            firstRect = rc;
            splitterRect = MakeRect(0, 0, 0, 0);
            secondRect = MakeRect(0, 0, 0, 0);
            return;
        }

        const double ratio = ClampDouble(node->ratio, 0.02, 0.98);
        const int splitterSize = SplitterSize();
        if (node->direction == SPLIT_HORIZONTAL)
        {
            if (RectHeight(rc) <= splitterSize + 2)
            {
                const int middle = rc.top + RectHeight(rc) / 2;
                firstRect = MakeRect(rc.left, rc.top, rc.right, middle);
                splitterRect = MakeRect(rc.left, middle, rc.right, std::min<int>(static_cast<int>(rc.bottom), middle + splitterSize));
                secondRect = MakeRect(rc.left, splitterRect.bottom, rc.right, rc.bottom);
                return;
            }

            const int available = RectHeight(rc) - splitterSize;
            int firstHeight = static_cast<int>(available * ratio + 0.5);
            firstHeight = ClampInt(firstHeight, 1, available - 1);
            firstRect = MakeRect(rc.left, rc.top, rc.right, rc.top + firstHeight);
            splitterRect = MakeRect(rc.left, firstRect.bottom, rc.right, firstRect.bottom + splitterSize);
            secondRect = MakeRect(rc.left, splitterRect.bottom, rc.right, rc.bottom);
        }
        else
        {
            if (RectWidth(rc) <= splitterSize + 2)
            {
                const int middle = rc.left + RectWidth(rc) / 2;
                firstRect = MakeRect(rc.left, rc.top, middle, rc.bottom);
                splitterRect = MakeRect(middle, rc.top, std::min<int>(static_cast<int>(rc.right), middle + splitterSize), rc.bottom);
                secondRect = MakeRect(splitterRect.right, rc.top, rc.right, rc.bottom);
                return;
            }

            const int available = RectWidth(rc) - splitterSize;
            int firstWidth = static_cast<int>(available * ratio + 0.5);
            firstWidth = ClampInt(firstWidth, 1, available - 1);
            firstRect = MakeRect(rc.left, rc.top, rc.left + firstWidth, rc.bottom);
            splitterRect = MakeRect(firstRect.right, rc.top, firstRect.right + splitterSize, rc.bottom);
            secondRect = MakeRect(splitterRect.right, rc.top, rc.right, rc.bottom);
        }
    }

    int SplitViewerWindow::SplitterSize() const
    {
        return borderVisible_ ? kSplitterSize : 0;
    }

    RECT SplitViewerWindow::SplitterDrawRect(const RECT& splitterRect, SplitDirection direction, int thickness) const
    {
        RECT result = splitterRect;
        if (direction == SPLIT_HORIZONTAL)
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

    bool SplitViewerWindow::GetSplitterVisualRect(Node* root, int layerIndex, Node* splitNode, bool hover, RECT& visualRect) const
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
                                      hover ? kSplitterHoverSize : SplitterSize());
        visualRect = InflatedRect(visualRect, 2, 2);
        return IsRectUsable(visualRect);
    }

    double SplitViewerWindow::FitImageScale(const ImageView& view, const RECT& rc) const
    {
        if (!view.HasImage() || RectWidth(rc) <= 0 || RectHeight(rc) <= 0)
        {
            return 1.0;
        }

        const double sx = static_cast<double>(RectWidth(rc)) / static_cast<double>(view.image->GetWidth());
        const double sy = static_cast<double>(RectHeight(rc)) / static_cast<double>(view.image->GetHeight());
        const double scale = std::min(sx, sy);
        return scale > 0.00001 ? scale : 1.0;
    }

    Node* SplitViewerWindow::FirstLeaf(Node* node) const
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

    bool SplitViewerWindow::FindLeafAt(Node* node, const RECT& rc, POINT pt, Node*& leaf, RECT& leafRect) const
    {
        if (!node || !IsRectUsable(rc) || !PointInRect(rc, pt))
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

    bool SplitViewerWindow::FindSplitterAt(Node* node, const RECT& rc, POINT pt, Node*& splitNode, RECT& nodeRect) const
    {
        if (!node || node->IsLeaf() || !IsRectUsable(rc))
        {
            return false;
        }

        RECT firstRect = { 0 };
        RECT splitterRect = { 0 };
        RECT secondRect = { 0 };
        SplitNodeRects(rc, node, firstRect, splitterRect, secondRect);

        RECT hitRect = InflatedRect(splitterRect, kSplitterHitPadding, kSplitterHitPadding);
        if (PointInRect(hitRect, pt))
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

    bool SplitViewerWindow::FindNodeRect(Node* current, const RECT& rc, Node* target, RECT& targetRect) const
    {
        if (!current || !target || !IsRectUsable(rc))
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

    bool SplitViewerWindow::CanDeleteLeafFromSplit(Node* root, Node* target) const
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
            if (PointInRect(rc, pt))
            {
                layerIndex = i;
                layerRect = rc;
                return true;
            }
        }

        layerIndex = -1;
        layerRect = MakeRect(0, 0, 0, 0);
        return false;
    }

    bool SplitViewerWindow::FindTopLeaf(POINT pt, LeafHit& hit) const
    {
        RECT stage = GetStageRect();
        for (int i = static_cast<int>(layers_.size()) - 1; i >= 0; --i)
        {
            RECT rc = GetLayerRect(i, stage);
            if (PointInRect(rc, pt))
            {
                RECT rootRect = ContentRect(rc);
                Node* leaf = NULL;
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

        if (!PointInRect(stage, pt))
        {
            return false;
        }

        RECT rootRect = ContentRect(stage);
        Node* leaf = NULL;
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

    bool SplitViewerWindow::FindTopSplitter(POINT pt, SplitterHit& hit) const
    {
        RECT stage = GetStageRect();
        int layerIndex = -1;
        RECT layerRect = { 0 };
        if (FindTopLayer(pt, layerIndex, layerRect))
        {
            RECT rootRect = ContentRect(layerRect);
            Node* splitNode = NULL;
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

        if (!PointInRect(stage, pt))
        {
            return false;
        }

        RECT rootRect = ContentRect(stage);
        Node* splitNode = NULL;
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
        if (!PointInRect(layerRect, pt))
        {
            return RESIZE_NONE;
        }

        int hit = RESIZE_NONE;
        if (pt.x - layerRect.left <= kLayerHitSize)
        {
            hit |= RESIZE_LEFT;
        }
        if (layerRect.right - pt.x <= kLayerHitSize)
        {
            hit |= RESIZE_RIGHT;
        }
        if (pt.y - layerRect.top <= kLayerHitSize)
        {
            hit |= RESIZE_TOP;
        }
        if (layerRect.bottom - pt.y <= kLayerHitSize)
        {
            hit |= RESIZE_BOTTOM;
        }
        return hit;
    }

    void SplitViewerWindow::NormalizeLayerResizeRect(RECT& rc, const RECT& bounds) const
    {
        if (RectWidth(rc) < kMinLayerWidth)
        {
            if (dragResizeHit_ & RESIZE_LEFT)
            {
                rc.left = rc.right - kMinLayerWidth;
            }
            else
            {
                rc.right = rc.left + kMinLayerWidth;
            }
        }

        if (RectHeight(rc) < kMinLayerHeight)
        {
            if (dragResizeHit_ & RESIZE_TOP)
            {
                rc.top = rc.bottom - kMinLayerHeight;
            }
            else
            {
                rc.bottom = rc.top + kMinLayerHeight;
            }
        }

        if (rc.left < bounds.left)
        {
            if (dragResizeHit_ & RESIZE_LEFT)
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
            if (dragResizeHit_ & RESIZE_TOP)
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
            if (dragResizeHit_ & RESIZE_RIGHT)
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
            if (dragResizeHit_ & RESIZE_BOTTOM)
            {
                rc.bottom = bounds.bottom;
            }
            else
            {
                OffsetRect(&rc, 0, bounds.bottom - rc.bottom);
            }
        }

        if (RectWidth(rc) < kMinLayerWidth)
        {
            rc.right = std::min(bounds.right, rc.left + kMinLayerWidth);
            rc.left = std::max(bounds.left, rc.right - kMinLayerWidth);
        }
        if (RectHeight(rc) < kMinLayerHeight)
        {
            rc.bottom = std::min(bounds.bottom, rc.top + kMinLayerHeight);
            rc.top = std::max(bounds.top, rc.bottom - kMinLayerHeight);
        }

        ConstrainRectToBounds(rc, bounds);
    }

    void SplitViewerWindow::ConstrainRectToBounds(RECT& rc, const RECT& bounds) const
    {
        const int width = RectWidth(rc);
        const int height = RectHeight(rc);
        if (width > RectWidth(bounds))
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

        if (height > RectHeight(bounds))
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

}
