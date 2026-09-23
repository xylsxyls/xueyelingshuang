#ifndef SPLITVIEWER_H
#define SPLITVIEWER_H

#include "QtControls/MainWindow.h"
#include <QtCore/QMap>
#include <QtCore/QRectF>
#include <QtGui/QImage>

#include "SplitViewerCoreAPI.h"

class QPainter;
class QResizeEvent;
class SplitViewerCanvas;
class QAction;
class ToolBar;
class Label;

#include "SplitViewerHit.h"

class SplitViewer : public MainWindow
{
    Q_OBJECT
public:
    /** 初始化对象及其默认状态。
    @param [in] parent Qt宿主窗口
    */
    explicit SplitViewer(QWidget* parent = NULL);
    /** 释放本对象持有的资源。
    */
    ~SplitViewer();

    /** 绘制工作区及原生窗口拖放预览。
    @param [in,out] painter 绘图目标
    @param [in] size 数据长度或区域尺寸
    */
    void paintCanvas(QPainter& painter, const QSize& size);
    /** 开始鼠标选中、拖动或加号加载操作。
    @param [in] point 工作区坐标
    @param [in] button 触发按钮
    @param [in] modifiers 键盘修饰键
    */
    void canvasMousePress(const QPoint& point, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    /** 更新拖动、缩放边缘和鼠标提示。
    @param [in] point 工作区坐标
    @param [in] buttons 当前按下按钮
    @param [in] modifiers 键盘修饰键
    */
    void canvasMouseMove(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    /** 结束鼠标交互并清除临时拖动状态。
    @param [in] point 工作区坐标
    @param [in] button 触发按钮
    @param [in] modifiers 键盘修饰键
    */
    void canvasMouseRelease(const QPoint& point, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    /** 处理图片滚轮缩放和缩放提示。
    @param [in] point 工作区坐标
    @param [in] delta 滚轮角度
    @param [in] modifiers 键盘修饰键
    */
    void canvasWheel(const QPoint& point, int delta, Qt::KeyboardModifiers modifiers);
    /** 显示命中位置对应的分屏操作菜单。
    @param [in] point 工作区坐标
    @param [in] globalPoint 屏幕坐标
    */
    void canvasContextMenu(const QPoint& point, const QPoint& globalPoint);
    /** 处理文件打开或默认位置拖放。
    @param [in] path 文件路径
    */
    void canvasFileDropped(const QString& path);
    /** 将拖入的图片加载到实际命中的分屏。
    @param [in] path 文件路径
    @param [in] point 工作区坐标
    */
    void canvasFileDroppedAt(const QString& path, const QPoint& point);
    /** 在有效内容区域切换内部全屏。
    @param [in] point 工作区坐标
    */
    void canvasDoubleClick(const QPoint& point);

public slots:
    /** 调整视图比例并同步嵌入窗口尺寸。
    */
    void canvasResized();
    /** 创建浮动图层并更新界面。
    */
    void newLayer();
    /** 选择并加载配置文件。
    */
    void openProfile();
    /** 选择路径并保存配置与缩略图。
    */
    void saveProfile();
    /** 选择Qt图片格式并导出当前背景板。
    */
    void saveImage();
    /** 切换工作区内部全屏并同步原生窗口。
    */
    void toggleFullscreen();
    /** 恢复全屏前的窗口状态和布局。
    */
    void leaveFullscreen();
    /** 切换背景板与图层边框。
    */
    void toggleBorder();
    /** 显示关于信息。
    */
    void showAbout();
    /** 读取用户输入的外部句柄并尝试嵌入。
    */
    void embedExternalWindow();
    /** 尝试嵌入鼠标下的外部窗口。
    */
    void embedWindowUnderCursor();
    /** 分离当前选中分屏的外部窗口。
    */
    void detachExternalWindow();

protected:
    /** 关闭前恢复全部外部窗口。
    @param [in] event 输入事件
    */
    void closeEvent(QCloseEvent* event);
    /** 窗口尺寸变化后刷新工作区布局。
    @param [in] event 输入事件
    */
    void resizeEvent(QResizeEvent* event);
    /** 平台文件选择适配，返回空串表示用户取消
    @param [in] save 是否保存
    @param [in] title 对话框标题
    @param [in] initial 初始路径
    @param [in] filter 文件过滤器
    @return 选择的路径
    */
    virtual QString browseFile(bool save, const QString& title, const QString& initial, const QString& filter);
    /** 呈现操作失败，测试适配可记录而不阻塞
    @param [in] message 错误描述
    */
    virtual void reportError(const QString& message);

private:
    /** 创建工具栏、工作区和状态栏。
    */
    void buildUi();
    /** 处理平台鼠标通知中的拖入、拖出和双击。
    @param [in] event 输入事件
    @param [in] screenPoint 鼠标屏幕位置
    @param [in] id 窗口句柄或节点编号
    */
    void nativeMouseEvent(int event, const QPoint& screenPoint, WId id);
    /** 刷新保存按钮可用性及状态栏。
    */
    void updateStatus();
    /** 根据背景板比例和窗口尺寸计算居中区域。
    @param [in] size 数据长度或区域尺寸
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QRectF stageRect(const QSize& size) const;
    /** 将归一化图层矩形转换为背景板像素坐标。
    @param [in] rect 区域矩形
    @param [in] stage 背景板像素矩形
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QRectF normalizedToPixel(const SplitViewerCoreRect& rect, const QRectF& stage) const;
    /** 计算空白分屏中央加号的可点击区域。
    @param [in] rect 区域矩形
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QRectF plusButtonRect(const QRectF& rect) const;
    /** 按图层层叠顺序查询当前命中区域。
    @param [in] point 工作区坐标
    @param [in,out] hit 命中结果
    @return 是否满足条件或操作成功。
    */
    bool hitAll(const QPointF& point, SplitViewerHit& hit) const;
    /** 对命中叶节点分屏，并重新绑定嵌入容器。
    @param [in] hit 命中结果
    @param [in] direction 分屏方向
    */
    void splitAt(const SplitViewerHit& hit, SplitViewerCoreSplitDirection direction);
    /** 删除命中分屏并同步借用节点和嵌入映射。
    @param [in] hit 命中结果
    */
    void deleteAt(const SplitViewerHit& hit);
    /** 解码图片后替换叶节点内容，失败保留原内容。
    @param [in] leaf 目标叶节点
    @param [in] path 文件路径
    */
    void loadImageToLeaf(SplitViewerCoreNode* leaf, const QString& path);
    /** 将窗口状态及文档交给文件适配层保存。
    @param [in] path 文件路径
    @return 是否满足条件或操作成功。
    */
    bool writeProfile(const QString& path);
    /** 成功加载配置后恢复图片资源并重置交互状态。
    @param [in] path 文件路径
    @return 是否满足条件或操作成功。
    */
    bool readProfile(const QString& path);
    /** 生成配置文件使用的PNG缩略图。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QByteArray renderPng();
    /** 导出背景板图像并恢复所有现场图片变换。
    @param [in] size 数据长度或区域尺寸
    @param [in] includeEmbedded 是否导出嵌入窗口快照
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QImage renderStage(const QSize& size, bool includeEmbedded);
    /** 按边框可见性计算内容区域。
    @param [in] owner 父区域或事件接收者
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    QRectF contentRect(const QRectF& owner) const;
    /** 将Core分屏几何转换为Qt矩形。
    @param [in] owner 父区域或事件接收者
    @param [in] node 文档持有的节点
    @param [in,out] first 第一子区域
    @param [in,out] splitter 分割线区域
    @param [in,out] second 第二子区域
    */
    void nodeRects(const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter, QRectF& second) const;
    /** 递归按新区域尺寸更新图片变换。
    @param [in] node 文档持有的节点
    @param [in] oldRect 原区域
    @param [in] newRect 新区域
    */
    void scaleViews(SplitViewerCoreNode* node, const QRectF& oldRect, const QRectF& newRect);
    /** 递归同步指定子树中的嵌入窗口。
    @param [in] node 文档持有的节点
    @param [in] rect 区域矩形
    */
    void syncEmbeddedNode(SplitViewerCoreNode* node, const QRectF& rect);
    /** 同步背景板和全部图层中的嵌入窗口。
    */
    void syncEmbeddedViews();
    /** 查询是否包含内容。
    @return 是否满足条件或操作成功。
    */
    bool hasContent() const;
    /** 清除树修改后失效的命中和拖动状态。
    */
    void clearInteraction();
    /** 保存有效命中并切换当前图层。
    @param [in] hit 命中结果
    */
    void setSelectedHit(const SplitViewerHit& hit);
    /** 将外部窗口嵌入当前选中的空白分屏。
    @param [in] windowId 借用的外部窗口句柄
    @return 是否满足条件或操作成功。
    */
    bool embedWindowId(WId windowId);

    SplitViewerCanvas* m_canvas;
    ToolBar* m_toolbar;
    QAction* m_saveProfileAction;
    Label* m_statusLabel;
    SplitViewerCoreDocument m_document;
    QMap<QString, QImage> m_imageCache;
    QMap<SplitViewerCoreNode*, QWidget*> m_embedded;
    bool m_exporting;
    bool m_contentFullscreen;
    QByteArray m_normalGeometry;
    bool m_wasMaximized;
    QRectF m_lastStage;
    Label* m_zoomTip;
    bool m_draggingLayer;
    bool m_resizingLayer;
    int m_resizeEdges;
    bool m_draggingImage;
    QPoint m_dragStart;
    QRectF m_dragStartRect;
    SplitViewerHit m_activeHit;
    QString m_profilePath;
    WId m_nativeDragWindow;
    SplitViewerCoreNode* m_nativeDragNode;
    QPoint m_nativeDragStart;
    QPoint m_nativeLastClick;
    qint64 m_nativeClickTime;
    QRectF m_nativePreview;
};

#endif