#pragma once
#include "SplitViewerVideoTarget.h"
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSizeF>
#include <QtGui/QImage>
#include <QtGui/qwindowdefs.h>

class QWidget;
class SplitViewerVideoSession;
class SplitViewerTaskManager;

/** 每个主窗口的媒体加载与资源绑定，所有方法只在GUI线程调用
*/
class SplitViewerMediaManager : public QObject
{
    Q_OBJECT
public:
    /** 借用主窗口的唯一文档、缓存和容器映射
    @param [in] document 主窗口持有的文档
    @param [in,out] images 主窗口持有的图片缓存
    @param [in,out] embedded 主窗口持有的容器映射
    @param [in] canvas 借用画布
    @param [in] tasks 借用本窗口任务管理器，生命周期覆盖本对象
    @param [in] parent 主窗口，须在上述成员销毁前显式释放本对象
    */
    SplitViewerMediaManager(SplitViewerCoreDocument* document, QMap<QString, QImage>* images,
        QMap<SplitViewerCoreNode*, QWidget*>* embedded, QWidget* canvas, SplitViewerTaskManager* tasks,
        QObject* parent);

    /** 撤销回调并清理仍持有的资源；借用对象此时仍须有效
    */
    ~SplitViewerMediaManager();

    /** 根据本次环境条件取得图片或媒体文件筛选
    @return Qt文件类型筛选字符串
    */
    QString fileFilter() const;

    /** 查询本次文件选择标题
    @return 有效播放器条件下的媒体标题，否则为原图片标题
    */
    QString selectionTitle() const;

    /** 查询本次右键加载动作文字
    @return 条件对应的加载动作文案
    */
    QString loadActionText() const;

    /** 将图片或视频加载到有效目标，视频准备失败不替换旧内容
    @param [in] leaf 本文档持有的叶子
    @param [in] path 文件路径
    @param [in] restoring 是否恢复已经保存的视频内容
    */
    void load(SplitViewerCoreNode* leaf, const QString& path, bool restoring = false);

    /** 复用唯一的外部窗口嵌入入口
    @param [in] leaf 本文档持有的叶子
    @param [in] windowId 外部借用句柄
    @param [in] videoPath 联动视频路径；为空时必须是空叶子
    @return 是否成功嵌入，失败保留原内容
    */
    bool embedWindow(SplitViewerCoreNode* leaf, WId windowId, const QString& videoPath = QString());

    /** 释放指定叶子的在途请求和外部容器，不清除Core内容
    @param [in] leaf 资源映射中的叶子，允许节点已被文档替换
    @return 是否恢复了独立外部窗口
    */
    bool release(SplitViewerCoreNode* leaf);

    /** 判断嵌入窗口可继续呈现；内部关闭后的播放器不得再被尺寸同步显示
    @param [in] container 借用嵌入容器
    @return 稳定进程及原窗口仍可用
    */
    bool windowValid(QWidget* container) const;

    /** 撤销全部在途加载并释放全部容器，不清除Core树
    */
    void releaseAll();

    /** 树结构变化时重绑定现有容器和启动目标
    @param [in] previous 原叶子地址，不解引用
    @param [in] replacement 新叶子地址
    */
    void rebind(SplitViewerCoreNode* previous, SplitViewerCoreNode* replacement);

    /** 为文档中已保存的视频路径逐个发起恢复
    */
    void restoreVideos();

    /** 是否仍有该叶子的启动意图
    @param [in] leaf 叶子地址，不解引用
    @return 是否有未提交加载
    */
    bool pending(SplitViewerCoreNode* leaf) const;

    /** 查询内容代次，用于拒绝弹窗期间失效的覆盖
    @return 单调内容代次
    */
    quint64 revision() const;

    /** 拒绝新加载、撤销在途意图并启动资源收尾
    */
    void shutdown();

    /** 是否仍有启动或关闭事务
    @return 是否需要异步收尾
    */
    bool busy() const;

signals:
    /** 媒体资源或内容发生变化，主窗口刷新显示
    */
    void changed();

    /** 可呈现的加载错误
    @param [in] message 错误文本
    */
    void error(const QString& message);

    /** 所有在途事务已经结束
    */
    void idle();

private:
    /** 从环境取得有效的程序文件，支持Unicode路径
    @return 绝对程序路径，无效时为空
    */
    static QString playerPath();

    /** 先通过当前文档核对借用节点，再允许访问叶子内容
    @param [in] leaf 待核对地址
    @return 是否仍为当前文档的叶子
    */
    bool validLeaf(SplitViewerCoreNode* leaf) const;

    /** 取消该叶子的全部未提交意图
    @param [in] leaf 地址匹配，不解引用
    */
    void cancelPending(SplitViewerCoreNode* leaf);

    /** 提交对应请求的窗口，失效请求只关闭自己的未提交进程
    @param [in] id 启动请求ID
    @param [in] windowId 此请求窗口
    */
    void windowReady(quint64 id, quintptr windowId);

private:
    // 借用唯一文档，主窗口显式销毁本对象后才释放文档
    SplitViewerCoreDocument* m_document;
    // 借用唯一缓存和容器映射，不复制权威状态
    QMap<QString, QImage>* m_images;
    // 借用主窗口拥有的嵌入容器映射
    QMap<SplitViewerCoreNode*, QWidget*>* m_embedded;
    // 画布由Qt父子关系持有，销毁后自动判空
    QPointer<QWidget> m_canvas;
    // 本管理器拥有的进程事务层，不持有Core节点
    SplitViewerVideoSession* m_session;
    // 主窗口持有的任务提交边界，仅GUI线程访问
    SplitViewerTaskManager* m_tasks;
    // 未提交媒体意图，只有本管理器修改
    QMap<quint64, SplitViewerVideoTarget> m_targets;
    // 内容或绑定变化时递增
    quint64 m_revision;
    // 收尾后不再接受加载
    bool m_stopping;
};