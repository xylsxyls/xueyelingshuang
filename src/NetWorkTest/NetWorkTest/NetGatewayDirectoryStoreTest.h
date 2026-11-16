#pragma once
#include "NetFileIoTest.h"
#include <vector>

/** 手动完成的无磁盘后端，仅用于缓存忙、畸形结果及停止契约测试。
*/
class NetDirectoryStoreTestBackend : public NetFileIoTestBackend
{
public:
    /** 创建没有已接纳任务的观察器。
    */
    NetDirectoryStoreTestBackend();

    /** 忙时无副作用，接纳时复制输入并记录任务。
    @param [in] request 只在本调用借用
    @return OK、LIMIT_EXCEEDED或INVALID_STATE
    */
    virtual int32_t submit(const NetFileIoRequest& request);

    /** 由用例决定是否完成，不主动推进任务。
    @param [out] result 借用到consume
    @return READY或NEED_MORE
    */
    virtual int32_t peek(NetFileIoResult& result);

    /** 只删除当前结果，不产生新任务。
    @param [in] id 精确完成编号
    @return OK或INVALID_STATE
    */
    virtual int32_t consume(uint64_t id);

    /** 填写受控完成；bytes由用例保持到consume。
    @param [in] result 后端完成码
    @param [in] file 文件能力号
    @param [in] length 受控文件或完成长度
    @param [in] bytes 可空受控读结果
    @param [in] size 结果长度
    */
    void finish(int32_t result, uint64_t file, uint64_t length, const uint8_t* bytes = nullptr, uint32_t size = 0);

    /** 创建独占观察器；基类记录当前实例。
    @return 新后端
    */
    static NetFileIoBackend* create();

public:
    // 尚需拒绝的提交次数。
    uint32_t m_busy;
    // 已接纳次数，不包含忙。
    uint32_t m_accepted;
    // 已消费次数。
    uint32_t m_consumed;
    // 当前完成是否可取。
    bool m_ready;
    // 指针已清空的任务元数据。
    NetFileIoRequest m_job;
    // 用例可注入错误id/operation，字节借用到consume。
    NetFileIoResult m_reply;
    // 已复制的WRITE正文。
    std::vector<uint8_t> m_copy;
};