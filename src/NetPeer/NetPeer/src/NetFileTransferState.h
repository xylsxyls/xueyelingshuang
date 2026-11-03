#pragma once
#include "NetFileTransfer.h"
#include <memory>
#include <string>

// 只由文件所属执行者推进；I/O在途身份与协议进度分别只有一个拥有者。
enum NetFileTransferPhase
{
    // 补摘要前缀或传输文件数据。
    NET_FILE_TRANSFER_DATA,
    // 收端已验证END，等待持久刷新接纳和完成。
    NET_FILE_TRANSFER_FLUSH,
    // 刷新成功，等待持句柄发布或关闭。
    NET_FILE_TRANSFER_FINALIZE,
    // 文件已提交且句柄已消耗，等待最终ACK交付。
    NET_FILE_TRANSFER_DRAIN,
    // 不再接纳新工作，收敛在途任务并关闭仍拥有的句柄。
    NET_FILE_TRANSFER_STOPPING,
    // 不再拥有任务、文件或待交付的输出。
    NET_FILE_TRANSFER_DONE
};

// 一个文件流的本地I/O、协议与停止协调状态，单执行者推进
class NetFileTransferState
{
public:
    /** 初始化无借用、无任务、无文件的状态
    */
    NetFileTransferState();

    /** 擦除本地路径的有效内容，流对象自行擦除协议秘密
    */
    ~NetFileTransferState();

public:
    // 共享后端借用至DONE；不得由本流停止或释放。
    NetFileIo* m_io;
    // 独占的文件协议及其有界缓存。
    std::unique_ptr<NetInterfaceFileFlow> m_flow;
    // 成功start后独占，CLOSE/PUBLISH完成或后端done后清零。
    uint64_t m_file;
    // 尚未被队列接纳的唯一工作；WRITE数据仍由flow借用。
    NetFileIoRequest m_waiting;
    // 已被队列接纳的唯一身份；只保留校验所需字段，不再借用输入数据。
    NetFileIoRequest m_job;
    // 已补摘要的前缀和本次恢复的固定目标。
    uint64_t m_prime;
    // 本次恢复起始位置，补摘要只推进到这个固定字节偏移
    uint64_t m_resume;
    // 最近一次poll时间；cancel不推进时钟。
    uint64_t m_now;
    // 协调阶段，不从I/O操作数值或多个完成标志反推。
    NetFileTransferPhase m_phase;
    // 首个失败原因；成功终态不被迟到取消改写。
    int32_t m_failure;
    // 固定角色与仅由本机调用方授权的覆盖策略。
    bool m_sender;
    // 宿主是否明确允许发布覆盖目标，不能由远端自行决定
    bool m_replace;
    // 本机授权路径，赋值前已核对4096字节上限。
    std::string m_temporary;
    // 宿主授权的最终本地发布路径，只有接收端使用
    std::string m_destination;
};