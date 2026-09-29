#ifndef SPLITVIEWER_UI_TESTS_H
#define SPLITVIEWER_UI_TESTS_H
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include <QtCore/QStringList>
#include <functional>

/** 只替换系统文件对话框的返回值，产品事件、模型和绘制均使用真实代码。 */
class SplitViewerTestWindow : public SplitViewer
{
public:
    QString nextFile;
    // 记录真实产品入口传入的文件过滤器，取消路径选择不替换文档
    QString lastFilter;
    QStringList errors;
    int browseCount;
    // 模拟文件选择嵌套事件循环期间发生的文档变更；执行前清空，避免重入。
    std::function<void()> duringBrowse;
    /** 创建并显示独立用例的产品窗口 */
    SplitViewerTestWindow();
protected:
    /** 记录参数，执行一次嵌套动作并返回下个测试路径
    @param [in] save 是否保存
    @param [in] title 窗口标题
    @param [in] initial 初始路径
    @param [in] filter 产品文件过滤器
    @return 预设路径，默认取消
    */
    QString browseFile(bool save, const QString& title, const QString& initial, const QString& filter) override;
    /** 保存产品错误供独立断言，不弹出阻塞窗口
    @param [in] message 错误内容
    */
    void reportError(const QString& message) override;
};
/** 执行所选真实UI及Core回归
@param [in] reportDirectory 证据目录
@param [in] selectedCase 数字用例，0表示全部
@return 失败数，非法选择返回2
*/
int SplitViewerRunUiTests(const QString& reportDirectory, int selectedCase = 0);
/** 获取按编号排序的用例名称
@return 含编号的名称列表
*/
QStringList SplitViewerAuditCaseNames();
#endif