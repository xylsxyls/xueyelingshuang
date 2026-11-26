#pragma once
#include "NetInterface.h"
#include <stddef.h>

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

    /** 从互不重叠的区域复制帧正文，短片段只访问其实际范围
    @param [out] output 至少 size 字节的独占存储，长度为0时可空
    @param [in] input 至少 size 字节的输入，长度为0时可空
    @param [in] size 正文字节数
    */
    static void copyFrameBody(uint8_t* output, const uint8_t* input, uint32_t size);

    /** 在写输出前排除别名和地址加法溢出，不读取待比较内存
    @param [in] left 第一段首地址
    @param [in] leftSize 第一段长度
    @param [in] right 第二段首地址
    @param [in] rightSize 第二段长度
    @return 重叠或非空区间地址溢出返回true
    */
    static bool overlap(const void* left, size_t leftSize, const void* right, size_t rightSize);

    /** 检查待写结果是否覆盖当前借用的帧描述或正文
    @param [in] frame 调用期间保持有效的输入帧
    @param [in] result 待写区域，长度为0时可空
    @param [in] size 待写字节数
    @return 写入会改变输入或区域地址溢出时为true
    */
    static bool overlapsFrame(const NetInterfaceFrame& frame, const void* result, size_t size);

    /** 初始化字节计数，避免计数写入覆盖本次输入或目标缓冲
    @param [in] bytes 输入或输出缓冲的首地址
    @param [in] size 缓冲字节数，空缓冲允许零长度
    @param [out] count 独立的计数字段，成功时置零
    @return 计数非空且与缓冲独立时为true
    */
    static bool prepareCount(const void* bytes, uint32_t size, uint32_t* count);

    /** 校验编码长度输出是否与调用方的帧、正文或输出空间重叠
    @param [in] frame 本次借用的帧元数据及正文
    @param [in] output 输出缓冲首地址
    @param [in] capacity 本次声明的可写字节数
    @param [in] length 用于返回长度的对象，本检查仅读取其地址
    @return 长度对象无法独立写入时返回true
    */
    static bool overlapsFrameLength(const NetInterfaceFrame& frame, const uint8_t* output, uint32_t capacity,
        const uint32_t& length);

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

#include "NetInterfaceFileGuard.inl"