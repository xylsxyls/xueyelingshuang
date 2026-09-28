#pragma once
#include <QtCore/QString>

/** 配置边界回归；使用公开接口和独立输入，不读取产品默认常量作为预期 */
class SplitViewerConfigTests
{
public:
    /** 执行配置或Core防护回归
    @param [in] id 用例191至193
    @param [in] directory 已创建的报告目录
    @return 全部断言及证据写入成功
    */
    static bool runCase(int id, const QString& directory);

private:
    /** 检查2与4阴影切换时主体及附属窗口保持一致
    @param [in] directory 截图输出目录
    @return 几何、范围、回收和截图均符合预期
    */
    static bool shadowSize(const QString& directory);

    /** 检查异常配置不能替换有效文档
    @return 所有异常输入均拒绝且保留原文档
    */
    static bool invalidProfile();

    /** 检查非有限宽高比和错误删除目标
    @return 边界调用不会破坏模型不变量
    */
    static bool modelGuards();
};