#pragma once
#include <QtCore/QString>

/** 配置边界回归；使用公开接口和独立输入，不读取产品默认常量作为预期 */
class SplitViewerConfigTests
{
public:
    /** 执行配置或Core防护回归
    @param [in] id 用例191至199、201
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

    /** 检查通用数字解析及损坏配置事务性
    @return 无效输入被拒绝，正常输入正确
    */
    static bool numericParsing();

    /** 用独立字节验证UTF-16代理对和失败保留输出
    @return 字节、字符及事务性全部满足预期
    */
    static bool unicodeConversion();

    /** 检查浮点溢出及无效分屏方向
    @return 结果有效或原状态保持
    */
    static bool geometryBounds();

    /** 检查图片缓存跨图层引用及最后引用回收
    @return 缓存键集合符合预期
    */
    static bool imageCacheLifetime();

    /** 检查中文路径、长度上限及读取失败输出保持
    @param [in] directory 测试自产文件目录
    @return 所有读取边界满足预期
    */
    static bool boundedFileRead(const QString& directory);

    /** 检查原位PNG封装、提取及非法长度
    @return 成功字节正确，失败保留输出
    */
    static bool packageBounds();

    /** 检查序列化树深度、图层数限制
    @return 超限拒绝且正常文档可保存
    */
    static bool serializationBounds();
};