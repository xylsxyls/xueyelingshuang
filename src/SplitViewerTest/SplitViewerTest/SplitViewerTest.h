#ifndef SPLITVIEWER_TEST_H
#define SPLITVIEWER_TEST_H

#include "SplitViewerTestConfig.h"
#include "QtControls/MainWindow.h"
#include <QtCore/QProcess>

class PlainTextEdit;
class ComboBox;
class PushButton;
class LineEdit;
class CheckBox;

/** 测试主界面；UI回归在本Test子进程执行，控件始终由GUI线程管理
*/
class SplitViewerTest : public MainWindow
{
    Q_OBJECT
public:
    /** 建立测试控件和子进程完成通知
    @param [in] parent 父窗口，可空
    */
    explicit SplitViewerTest(QWidget* parent = nullptr);

    /** 释放Qt持有的控件；正常关闭先收敛测试子进程
    */
    ~SplitViewerTest();

    /** 查询最近一次完整结束的测试结果
    @return 全部成功且当前没有在途批次
    */
    bool allTestsPassed() const;

protected:
    /** 在途批次关闭时等待子进程自行恢复测试资源，不同步等待GUI
    @param [in] event Qt关闭请求
    */
    void closeEvent(QCloseEvent* event) override;

private slots:
    /** 执行轻量Core冒烟，运行中的UI批次不能重入
    */
    void runTests();

    /** 异步启动选择的UI回归子进程并锁定本轮输入
    */
    void runUiTests();

    /** 启动全部瞬时场景，使用与指定选择相同的调度
    */
    void runInstantTests();

    /** 启动登记的压力场景，参数由用例目录固定
    */
    void runPressureTests();

    /** 启动整个有效目录
    */
    void runAllTests();

    /** 请求批次在当前场景安全收尾后取消
    */
    void cancelTests();

    /** 读取当前批次结果，更新界面并按需完成关闭
    @param [in] exitCode 进程退出码
    @param [in] status 正常退出或崩溃
    */
    void uiProcessFinished(int exitCode, QProcess::ExitStatus status);

    /** 启动失败时恢复界面，其他错误等待finished统一收尾
    @param [in] error 子进程错误类型
    */
    void uiProcessError(QProcess::ProcessError error);

private:
    /** 统一切换批次输入可用性
    @param [in] running 是否已有在途批次
    */
    void setRunning(bool running);

    /** 校验选择后异步创建本批次子进程
    @param [in] selection 数字选择表达式
    */
    void startBatch(const QString& selection);

private:
    // Qt父子关系持有的纯文本输出
    PlainTextEdit* m_output;
    // 最近完整批次结果，启动时清零
    bool m_allTestsPassed;
    // Qt父子关系持有的用例选择控件
    ComboBox* m_uiCases;
    // Core测试入口
    PushButton* m_coreRun;
    // UI测试入口
    PushButton* m_uiRun;
    // 仅持有本界面启动的Test子进程
    QProcess* m_uiProcess;
    // 子进程在途标志，防止连续点击和结果混批
    bool m_running;
    // 收到关闭后等待本批进程结束
    bool m_closeRequested;
    // 当前批次唯一报告目录
    QString m_reportDirectory;
    // 指定编号的编辑入口
    LineEdit* m_caseSelection;
    // 全部瞬时入口，由Qt持有
    PushButton* m_instantRun;
    // 全部压力入口，由Qt持有
    PushButton* m_pressureRun;
    // 整个目录入口，由Qt持有
    PushButton* m_allRun;
    // 协作取消入口，由Qt持有
    PushButton* m_cancelRun;
    // 被测Release主程序路径，只读文本框
    LineEdit* m_releasePath;
    // 被测Debug主程序路径，只读文本框
    LineEdit* m_debugPath;
    // 选择Debug时启动对应Debug Test入口
    CheckBox* m_useDebug;
    // 本批选择数，报告必须完整覆盖才能通过
    int32_t m_expectedCaseCount;
};

#endif