#pragma once

// protobuf与Variant的二进制边界测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceProtoTestCases
{
public:
    /** 用手写protobuf3.3.0向量核验状态码、NUL和旧Variant线格式
    */
    static void wireCompatibility();

    /** map/list/table中的非法类型及数字长度均拒绝，失败不留下旧值
    */
    static void invalidValues();

    /** 验证深拷贝、自赋值、空值及数字对象的安全生命周期
    */
    static void ownership();

    /** 验证真实C++主类入口、大小查询、失败缓冲不变及累计预算
    */
    static void cBoundary();

    /** 验证消息字节、条目、递归深度和截断边界
    */
    static void limits();

    /** 空容器和空行保留、未知字段兼容、独立编码和失败回滚
    */
    static void atomicContainers();

    /** 初始化抛异常后允许重试，只有成功的初始化才能被后续调用复用
    */
    static void initializationRecovery();
};