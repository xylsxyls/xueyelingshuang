#pragma once
#include "TestConfig.h"
#include "TestCaseRegistry.h"
#include "TestJob.h"
#include "ui_LumaPlayerTest.h"
#include <QMainWindow>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonArray>
#include <vector>
#include <memory>

class TestCommandTask;
class QLabel;
class QCheckBox;
class QLineEdit;
class QProgressBar;
class QPlainTextEdit;
class QPushButton;

/** 独立运行测试调度窗口，子进程隔离异常，报告持续落盘 */
class LumaPlayerTest : public QMainWindow
{
    Q_OBJECT
public:
    /** 初始化进度窗口与本轮参数
    @param [in] root 报告根目录
    @param [in] media 真实媒体
    @param [in] fixture 规则媒体
    @param [in] selfCheck true仅验证测试调度器
    */
    LumaPlayerTest(const QString& root, const QString& media, const QString& fixture, bool selfCheck = false);

    /** 队列启动，不阻塞构造 */
    void start();

    /** 事件循环退出后按管理器ID回收有限命令线程 */
    virtual ~LumaPlayerTest();

signals:
    /** 后台命令结果经队列返回，Qt对象不跨线程操作 */
    void commandReady(const QString& mode, const QStringList& selected, const QStringList& skipped, const QString& error, const QString& media, const QString& fixture);

public:

    /** 从按钮或命令行选择同一执行路径
    @param [in] mode specified/instant/pressure/all
    @param [in] ids 指定ID文本
    */
    void beginRun(const QString& mode, const QString& ids);

protected:
    /** 运行时阻止误关，完成后允许退出
    @param [in] event Qt关闭事件
    */
    virtual void closeEvent(QCloseEvent* event);

private slots:
    /** 启动下一子进程或发布完成报告 */
    void next();

    /** 检查超时、启动压力的就绪及正常退出 */
    void tick();

    /** 接收真实退出结果
    @param [in] code 子进程退出码
    @param [in] status 正常退出或崩溃
    */
    void finished(int code, QProcess::ExitStatus status);

    /** 启动失败时形成失败记录
    @param [in] error QProcess错误
    */
    void processError(QProcess::ProcessError error);

private:
    /** 命令结果到达GUI后执行非阻塞进程状态机
    @param [in] mode 测试方式或stop
    @param [in] ids 数字选择
    */
    void executeCommand(const QString& mode, const QStringList& selected, const QStringList& skipped, const QString& error, const QString& media, const QString& fixture);

    /** 校验路径，缺失时提示用户选择
    @return true表示路径完整
    */
    bool validatePaths();

    /** 选择被测程序目录
    @param [in] debug 是否选择Debug
    */
    void choosePath(bool debug);

    /** 按批次状态更新路径控件 */
    void updatePathControls();

    /** 原子保存本批报告和最新入口 */
    void saveReport();

    /** 保存stdout/stderr而非丢弃异常现场 */
    void drainOutput();

private:
    // 保留用户提供的UI资源
    Ui::LumaPlayerTestClass m_ui;
    // 媒体路径
    QString m_media;
    // CTaskThreadManager返回的管理ID，不是系统线程ID
    uint32_t m_commandThread;
    // 已提交命令，禁止重复叠加
    bool m_commandPending;
    // 规则素材路径
    QString m_fixture;
    // 本轮选择模式
    QString m_mode;
    // 选定且去重的用例
    std::vector<int32_t> m_selected;
    // 已删除而跳过的ID
    std::vector<int32_t> m_skipped;
    // 四个启动按钮，Qt父子管理
    std::vector<QPushButton*> m_buttons;
    // 真实视频目录控件，由Qt管理
    QLineEdit* m_videoPath;
    QPushButton* m_videoBrowse;
    // 准备阶段任务，用于协作取消
    std::shared_ptr<TestCommandTask> m_prepareTask;
    // Debug选择及只读路径控件，由Qt管理
    QCheckBox* m_debugCheck;
    QLineEdit* m_debugPath;
    QLineEdit* m_releasePath;
    QPushButton* m_debugBrowse;
    QPushButton* m_releaseBrowse;
    // 停止按钮
    QPushButton* m_stop;
    // 批次活动中
    bool m_active;
    // 用户要求停止
    bool m_cancel;
    // 命令行自动退出
    bool m_autoExit;
    // 子进程系统ID，退出后仍可关联日志
    int64_t m_childPid;
    // 默认文案和规模
    TestConfig m_config;
    // 根目录
    QString m_root;
    // 唯一批次目录
    QString m_batch;
    // 测试任务列表
    std::vector<TestJob> m_jobs;
    // 当前索引
    size_t m_index;
    // 是否已经结束
    bool m_done;
    // 本任务已收到终态，防止重复回调
    bool m_terminal;
    // 当前是否已请求正常关闭
    bool m_closeSent;
    // 超时失败标记
    bool m_timedOut;
    // 测试调度器自检模式
    bool m_selfCheck;
    // 首次取消或超时的时间，之后给予协作退出机会
    int64_t m_cancelAt;
    // 子进程管道
    QProcess m_process;
    // 看门狗
    QTimer m_timer;
    // 当前项耗时
    QElapsedTimer m_elapsed;
    // 整轮耗时
    QElapsedTimer m_total;
    // 完成项记录
    QJsonArray m_records;
    // 所有断言
    QJsonArray m_checks;
    // 进度文案，Qt管理
    QLabel* m_status;
    // 真实完成进度，Qt管理
    QProgressBar* m_progress;
    // 输出摘要，Qt管理
    QPlainTextEdit* m_output;
};