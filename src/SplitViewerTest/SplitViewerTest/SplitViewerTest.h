#ifndef SPLITVIEWER_TEST_H
#define SPLITVIEWER_TEST_H

#include "SplitViewerTestConfig.h"
#include "QtControls/MainWindow.h"
#include <QtCore/QProcess>

class PlainTextEdit;
class ComboBox;
class PushButton;

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
    /** 追加单个Core结果
    @param [in] name 用例名称
    @param [in] passed 独立断言结果
    @param [in] detail 补充信息，可空
    */
    void appendResult(const QString& name, bool passed, const QString& detail = QString());

    /** 统一切换批次输入可用性
    @param [in] running 是否已有在途批次
    */
    void setRunning(bool running);

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
};

#endif