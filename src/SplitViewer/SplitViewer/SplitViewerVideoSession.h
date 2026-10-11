#pragma once
#include "SplitViewerVideoRequest.h"
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QElapsedTimer>

class QTimer;
class QWidget;

/** GUI事件驱动的播放器启动与关闭事务；不拥有Core节点，不创建业务线程
*/
class SplitViewerVideoSession : public QObject
{
    Q_OBJECT
public:
    /** 创建本窗口的进程事务管理器
    @param [in] parent QObject父对象，负责最终释放
    */
    explicit SplitViewerVideoSession(QObject* parent);

    /** 兜底清理仍未提交或关闭中的进程，不等待GUI事件
    */
    ~SplitViewerVideoSession();

    /** 排队启动独立播放器，异步回报窗口或失败
    @param [in] program 已确认存在的可执行文件绝对路径
    @param [in] path 视频绝对路径
    @return 唯一请求ID，容量或关闭状态拒绝时返回0
    */
    quint64 start(const QString& program, const QString& path);

    /** 将已成功嵌入且符合关闭策略的请求转入活动记录，保留稳定进程身份
    @param [in] id 本次已就绪的请求ID
    */
    void accept(quint64 id);

    /** 撤销未提交请求，只关闭该请求自己启动的进程
    @param [in] id 请求ID，不存在时无操作
    */
    void cancel(quint64 id);

    /** 按Config进程名策略关闭或恢复嵌入窗口，并释放容器
    @param [in] container 待释放容器，调用后不再有效
    @return true表示已恢复独立窗口，false表示关闭中或原窗口已不存在
    */
    bool releaseWindow(QWidget* container);

    /** 为符合关闭策略的外部窗口保留稳定进程身份；联动请求复用已有身份
    @param [in] windowId 刚嵌入且仍有效的借用窗口句柄
    @return 可安全管理，false时调用方恢复并释放新容器
    */
    bool trackWindow(WId windowId);

    /** 校验被管理窗口自己的可见状态和进程身份，防止重新显示已关闭的播放器
    @param [in] container 借用嵌入容器，可为空
    @return 窗口可继续呈现；关闭中、窗口销毁或身份不符返回false
    */
    bool windowValid(QWidget* container) const;

    /** 拒绝新请求并收敛在途启动；可重复调用
    */
    void shutdown();

    /** 查询是否仍有未提交或关闭中的事务
    @return 是否尚有事务需要收尾
    */
    bool busy() const;

signals:
    /** 本请求的窗口已可嵌入，接收者必须accept或cancel
    @param [in] id 请求ID
    @param [in] windowId 本次进程的借用窗口句柄
    */
    void windowReady(quint64 id, quintptr windowId);

    /** 本请求失败，接收者不得再提交该窗口
    @param [in] id 请求ID
    @param [in] message 可呈现的错误
    */
    void failed(quint64 id, const QString& message);

    /** 在途事务全部收尾，关闭中的主窗口可继续退出
    */
    void idle();

private:
    /** 推进有限的启动、窗口发现和退出查询，不同步等待
    */
    void poll();

    /** 释放指定请求的身份并移除记录
    @param [in] id 请求ID
    */
    void finish(quint64 id);

    /** 发起该请求的正常关闭，超时按稳定身份处理
    @param [in] id 已持有进程身份的请求ID
    */
    void beginClose(quint64 id);

    /** 启动并原子取得进程身份，避免启动后遗漏无法OpenProcess的进程
    @param [in,out] request 启动记录，成功后拥有进程身份
    @return 是否成功启动并持有身份
    */
    static bool launchProcess(SplitViewerVideoRequest& request);

    /** 校验活动记录的进程与原窗口，窗口自己隐藏表示内部关闭；父宿主隐藏不算关闭
    @param [in] request 本会话持有的稳定进程及借用窗口身份
    @return 进程在运行，窗口属于原PID且自身可见
    */
    static bool windowAvailable(const SplitViewerVideoRequest& request);

    /** 根据PID取得稳定进程身份，平台不支持时返回0
    @param [in] processId 本次进程ID
    @return 本管理器负责释放的原生身份
    */
    static quintptr captureProcess(qint64 processId);

    /** 非阻塞查询稳定身份是否仍在运行
    @param [in] token 已持有的进程身份
    @return 是否仍在运行
    */
    static bool running(quintptr token);

    /** 释放进程身份，不结束已提交的播放器
    @param [in] token 已持有的身份
    */
    static void releaseProcess(quintptr token);

    /** 只终止所持稳定身份的进程，不按名称结束其他实例
    @param [in] token 已持有的身份
    @return 是否成功发出终止
    */
    static bool forceClose(quintptr token);

private:
    // GUI线程独占的在途事务；每个原生身份只有一个记录拥有
    QMap<quint64, SplitViewerVideoRequest> m_requests;
    // GUI线程独占的已嵌入关闭策略窗口；窗口消失后仍持有其进程身份
    QMap<WId, SplitViewerVideoRequest> m_embeddedRequests;
    // 本对象拥有的GUI单线程计时器
    QTimer* m_timer;
    // 单调时间，用于启动和关闭预算
    QElapsedTimer m_clock;
    // 本会话内不复用的请求编号
    quint64 m_nextId;
    // shutdown后拒绝新启动
    bool m_stopping;
};