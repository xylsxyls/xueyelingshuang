#pragma once
#include "NetInterface.h"

// 文件线格式的临时秘密只借用当前栈，不拥有内存；离开作用域统一擦除。
class NetInterfaceFileGuard
{
public:
    /** 登记临时秘密，调用者保证其存活至本对象析构
    @param [in,out] bytes 待擦除区域
    @param [in] size 固定字节数
    */
    NetInterfaceFileGuard(void* bytes, uint32_t size);

    /** 正常返回及异常退出均擦除，不分配内存
    */
    ~NetInterfaceFileGuard();

    /** 在写输出前排除别名和地址加法溢出，不读取待比较内存
    @param [in] left 第一段首地址
    @param [in] leftSize 第一段长度
    @param [in] right 第二段首地址
    @param [in] rightSize 第二段长度
    @return 重叠或非空区间地址溢出返回true
    */
    static bool overlap(const void* left, uint32_t leftSize, const void* right, uint32_t rightSize);

private:
    /** 禁止复制清理职责
    @param [in] other 原清理器
    */
    NetInterfaceFileGuard(const NetInterfaceFileGuard& other);

    /** 禁止赋值清理职责
    @param [in] other 原清理器
    @return 不可调用
    */
    NetInterfaceFileGuard& operator=(const NetInterfaceFileGuard& other);

private:
    // 借用区域及固定长度，析构不释放底层存储。
    uint8_t* m_bytes;
    // 借用区域的固定字节数，不能超过该区域生命周期内的有效容量
    uint32_t m_size;
};