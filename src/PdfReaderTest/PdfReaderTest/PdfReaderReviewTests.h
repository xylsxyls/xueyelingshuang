#pragma once
#include <QString>

/** 配置、阴影与异步边界；预期来自独立素材及受控时序 */
class PdfReaderReviewTests
{
public:
    /** 执行21至26号用例，失败抛异常交由统一报告记录
    @param [in] id 稳定用例ID
    @param [in] input 独立生成的三页PDF
    @param [in] directory 当前用例输出目录
    */
    static void run(int32_t id, const QString& input, const QString& directory);
private:
    /** 配置单例、恢复和明确的弹窗布局选项
    @param [in] directory 截图输出位置
    */
    static void configuration(const QString& directory);
    /** 阴影生命周期、像素和菜单不透明
    @param [in] input 独立PDF
    @param [in] directory 截图位置
    */
    static void shadowAndMenu(const QString& input, const QString& directory);
    /** 受控执行者阻塞检查接纳、取消和关闭
    @param [in] id 请求、旧结果、关闭或失败测试
    @param [in] input 独立PDF
    @param [in] directory 输出位置
    */
    static void asynchronous(int32_t id, const QString& input, const QString& directory);
};