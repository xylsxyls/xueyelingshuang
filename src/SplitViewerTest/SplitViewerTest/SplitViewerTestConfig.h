#pragma once
#include <QtCore/QSize>
#include <QtCore/QString>
#include <stdint.h>
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
    // 仅Test子进程使用的窗口关闭后继续运行的固定素材名称
    QString m_lifetimeProbeFileName;
    // 测试进程是否生成日志
    bool m_logEnabled;
    // 测试进程是否同时输出控制台日志
    bool m_logConsoleEnabled;
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
    int32_t m_maximumOutputBlocks;
    // 全部瞬时入口文案
    QString m_instantText;
    // 全部压力入口文案
    QString m_pressureText;
    // 全部场景入口文案
    QString m_allText;
    // 取消入口文案
    QString m_cancelText;
    // 选择表达式提示
    QString m_selectionHint;
    // 取消等待文案
    QString m_cancelPendingText;
    // Release被测主程序路径，只读展示，候选目录优先
    QString m_releaseProgramPath;
    // Debug被测主程序路径，只读展示
    QString m_debugProgramPath;
    // Release测试入口，须与被测构建类型一致
    QString m_releaseTestPath;
    // Debug测试入口，缺失时明确报错不退回Release
    QString m_debugTestPath;
    // 构建类型复选框文案
    QString m_debugText;
    // 被测主程序不存在的提示
    QString m_missingProgramText;
    // 用户输入的字符预算
    int32_t m_selectionMaximumLength;

private:
    /** 在初始化列表设置测试界面默认值
    */
    SplitViewerTestConfig();

    /** 基于EXE目录计算候选与发布主程序路径，不创建业务资源
    */
    void init();

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