#pragma once
#include <QtCore/QSize>
#include <QtCore/QString>
#include <string>

/** 测试界面的固定值，不参与被测算法的预期计算
*/
class SplitViewerTestConfig
{
public:
    /** 获取Test宿主配置；在QApplication创建后的GUI线程首次访问
    @return 进程内唯一配置引用
    */
    static SplitViewerTestConfig& instance();

public:
    // 主界面初始尺寸，逻辑像素
    QSize m_windowSize;
    // 测试窗口标题
    QString m_title;
    // Core冒烟按钮文案
    QString m_coreRunText;
    // UI回归按钮文案
    QString m_uiRunText;
    // 选择全部用例的文案
    QString m_allCasesText;
    // 报告目录提示前缀
    QString m_reportPrefix;
    // 批次失败提示
    QString m_failedText;
    // 子进程启动失败提示前缀
    QString m_startErrorPrefix;
    // 等待测试恢复资源后关闭的提示
    QString m_closePendingText;
    // 输出框保留的最大行数
    int m_maximumOutputBlocks;
    // 单项结果格式，依次为状态、名称和细节
    std::wstring m_resultFormat;
    // 汇总结果格式，依次为通过数及总数
    std::wstring m_summaryFormat;

private:
    /** 在初始化列表设置测试界面默认值 */
    SplitViewerTestConfig();
    /** 禁止复制进程配置
    @param [in] other 不可复制的配置
    */
    SplitViewerTestConfig(const SplitViewerTestConfig& other);
    /** 禁止赋值进程配置
    @param [in] other 不可赋值的配置
    @return 不提供实现
    */
    SplitViewerTestConfig& operator=(const SplitViewerTestConfig& other);
};

// Test还链接被测产品Config，使用独立宏避免污染产品配置。
#define g_testConfig SplitViewerTestConfig::instance()