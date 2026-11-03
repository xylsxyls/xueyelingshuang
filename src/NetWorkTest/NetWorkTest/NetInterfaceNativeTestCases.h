#pragma once

#include <stdint.h>

// 原生文件、资源及生命周期测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceNativeTestCases
{
public:
    /** 重复查询共享区容量不能创建不释放的映射；仅Windows执行平台区域检查
    */
    static void sharedMemoryCapacity();

    /** 真实TCP二进制、发送数量/字节上限、恢复后完整交付及幂等关闭
    */
    static void boundedTcp();

    /** 连接容量拒绝、回调异常关闭、旧连接号不能发送到新连接
    */
    static void tcpLifecycle();

    /** 公开系统文件句柄的实际读写、持久化、截断及无覆盖发布
    */
    static void durableFile();

    /** 验证共享CTask I/O与文件流、实际持久化提交组合
    */
    static void asyncFileTransfer();

    /** 具备取消删除所有权的临时文件也必须完成原子发布，不能先关闭而丢失临时文件。
    */
    static void ownedTemporaryTransfer();

    /** 已有可写临时文件从1024字节续传，重新摘要前缀后持句柄发布。
    */
    static void resumedFileTransfer();

    /** 用真实原生后端验证错误完成身份被拒绝，done前关闭两端文件并删除自有临时文件
    */
    static void fileTransferIdentity();
private:
    /** 复用实际文件流和持久化后端，不模拟协议或提交结果
    @param [in] createOperation OPEN_NEW、OPEN_TEMPORARY或恢复已有文件的OPEN_WRITE
    @param [in] resumeOffset 恢复前缀长度；0新建，1024验证实际前缀重读／摘要
    @param [in] failureMode 0正常，1至3分别污染读取完成的文件号、操作及偏移
    */
    static void runAsyncFileTransfer(uint32_t createOperation, uint32_t resumeOffset = 0, uint32_t failureMode = 0);
};

// 仅测试素材的句柄所有权，操作直接调用CSystem静态API，异常路径也关闭素材。
class NetNativeFileTestOwner
{
public:
    /** 初始化测试无效句柄
    */
    NetNativeFileTestOwner();

    /** 幂等关闭测试句柄
    */
    ~NetNativeFileTestOwner();
private:
    /** 禁止复制
    @param [in] other 原对象
    */
    NetNativeFileTestOwner(const NetNativeFileTestOwner& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetNativeFileTestOwner& operator=(const NetNativeFileTestOwner& other);
public:
    // 本用例独占的系统资源句柄，析构兜底回收
    intptr_t m_handle;
};