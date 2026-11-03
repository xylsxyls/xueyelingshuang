#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include <string>
#include <vector>

/** 本模块持有NI创建的只读目录句柄，只通过NI释放和复制值。
不导出此内部类；不跨模块传递STL所有权，不实现协议算法。
*/
class NetDirectorySnapshot
{
public:
    /** 创建空句柄，不分配NI状态。
    */
    NetDirectorySnapshot();

    /** 由NI释放NI拥有的候选和容器。
    */
    ~NetDirectorySnapshot();

    /** 释放当前句柄，可重复调用。
    */
    void reset();

    /** 从调用者值记录建立NI拥有的快照，失败保持空。
    @param [in] nodes 借用至返回
    @param [in] count 1至64项
    @return NI结果
    */
    int32_t create(const NetInterfaceDirectoryNodeData* nodes, uint32_t count);

    /** 由NI解析发现体或NGC3缓存，失败保持空。
    @param [in] bytes 借用输入
    @param [in] size 输入长度
    @param [in] utcMs 0为发现体，非0为缓存校验UTC
    @return NI结果
    */
    int32_t parse(const uint8_t* bytes, uint32_t size, uint64_t utcMs = 0);

    /** 复制到调用者数组；容量不足保留快照，输出元数据必须为v1。
    @param [out] nodes 调用者已有数组，预填size/version；可空查询
    @param [in] capacity 0至64
    @param [out] count 必需项数，不得与nodes重叠
    @return NI结果
    */
    int32_t copy(NetInterfaceDirectoryNodeData* nodes, uint32_t capacity, uint32_t& count) const;

    /** 内部调用者在本模块分配值容器，NI只向其中复制固定字节。
    @param [out] nodes 成功替换，失败保留；本内部类不跨DLL使用
    @return NI结果
    */
    int32_t values(std::vector<NetInterfaceDirectoryNodeData>& nodes) const;

    /** 在本模块分配输出缓存，由NI填入字节，输出也由本模块调用者保持。
    @param [in] utcMs 当前UTC，只作缓存期限
    @param [out] bytes 成功为NGC3，失败为空；不跨DLL调用本内部方法
    @return NI结果
    */
    int32_t cache(uint64_t utcMs, std::string& bytes) const;

private:
    /** 禁止复制句柄所有权。
    @param [in] other 原对象
    */
    NetDirectorySnapshot(const NetDirectorySnapshot& other);

    /** 禁止赋值。
    @param [in] other 原对象
    @return 不可调用
    */
    NetDirectorySnapshot& operator=(const NetDirectorySnapshot& other);

    // 只由NI创建及销毁；本类不解引用它。
    NetInterfaceDirectoryHandle m_handle;
};