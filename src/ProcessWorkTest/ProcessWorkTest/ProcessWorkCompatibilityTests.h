#pragma once
#include "ProcessWorkTest.h"

/** 已迁移到当前公开接口的边界回归，不依赖生产测试宏 */
class ProcessWorkCompatibilityTests
{
public:
    /** 最大正文、缩小后的同PID新代、空包专用接收器和旧通道失效 */
    static void payloadRestart();

    /** 五种发送路径覆盖完整非负类型边界，负类型不得交付 */
    static void typeDomain();

    /** 五种路径的源缓冲区边界及返回后撤销访问，验证独立副本 */
    static void guardedInput();

    /** 四生产者、八种重载、四种正文容量，夹杂拒绝消息仍保持各流FIFO */
    static void overloadRejection();
};

/** 测试独占保护页；析构释放本次申请，不修改产品进程内存 */
class ProcessWorkInputPages
{
public:
    /** 创建64KiB可访问区域及两侧不可访问保护页 */
    ProcessWorkInputPages();

    /** 释放本次完整映射 */
    ~ProcessWorkInputPages();

    /** 切换正文区域可访问性
    @param [in] enabled true读写，false不可访问
    */
    void protect(bool enabled);

    /** 取得正文区域端点内的缓冲区
    @param [in] length 0到65536字节
    @param [in] tail true紧靠末端保护页，false紧靠首端保护页
    @return length为0时nullptr，否则本对象借用的地址
    */
    char* data(size_t length, bool tail);

private:
    // 系统申请的完整映射
    void* m_allocation;
    // 系统页大小
    size_t m_page;
    // 中间可访问区长度
    size_t m_accessible;
};