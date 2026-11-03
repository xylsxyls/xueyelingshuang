#pragma once
#include "NetInterfaceFileFlow.h"
#include "NetInterface.h"
#include <vector>

// 文件协议只由所属执行者推进；落盘提交与网络交付是不同阶段。
enum NetInterfaceFilePhase
{
    // 恢复前重算已有持久前缀摘要，禁止接纳新的文件块。
    NI_FILE_PHASE_PREFIX,
    // 可以产生/接纳文件块，发送额度按已发送减已确认计算。
    NI_FILE_PHASE_TRANSFER,
    // 收端已验证结束及整文件摘要，等待宿主取提交任务。
    NI_FILE_PHASE_COMMIT_WAIT,
    // 宿主已经取得提交任务，等实际提交结果。
    NI_FILE_PHASE_COMMIT_ACTIVE,
    // 发端结束帧已被下层接纳，等收端最终提交ACK。
    NI_FILE_PHASE_FINAL_ACK,
    // 成功终态，不再被迟到取消或时钟更新改写。
    NI_FILE_PHASE_DONE,
    // 保持首个失败；收端可以输出唯一失败ACK。
    NI_FILE_PHASE_FAILED
};

// 一个输出槽由peek借用至consume或失败，不在背压时改写内容。
enum NetInterfaceFileOutput
{
    // 没有待交付的完整帧。
    NI_FILE_OUTPUT_NONE,
    // 发端文件数据，consume之后才推进已发送位置。
    NI_FILE_OUTPUT_DATA,
    // 发端结束，consume之后进入等待最终ACK阶段。
    NI_FILE_OUTPUT_END,
    // 收端累计/成功/失败ACK，保存生成时的前缀及标志。
    NI_FILE_OUTPUT_ACK
};

/** C句柄唯一拥有的C++文件状态机，创建模块负责销毁
*/
struct NetInterfaceFileFlowOpaque
{
public:
    // C与C++共用的唯一流对象。
    NetInterfaceFileFlow m_flow;

public:
    /** 创建同一C++核心
    @param [in] info 元数据
    @param [in] product 产品
    @param [in] sender 发端角色
    @param [in] nowMs 单调时间
    @param [in] timeoutMs 无进度期限
    */
    NetInterfaceFileFlowOpaque(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs, uint32_t timeoutMs);
};

/** 单执行者文件协议状态，拥有缓存和摘要，文件读写由宿主提交结果
*/
class NetInterfaceFileFlowState
{
public:
    /** 初始化固定元数据和唯一摘要，不访问文件或启动线程
    @param [in] info 已授权的传输信息
    @param [in] product 非零产品
    @param [in] sender 发端角色
    @param [in] nowMs 单调时间
    @param [in] timeoutMs 无进度期限1000至60000毫秒
    */
    NetInterfaceFileFlowState(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs, uint32_t timeoutMs);

    /** 擦除全部有效秘密和文件字节后释放存储及摘要
    */
    ~NetInterfaceFileFlowState();

    /** 擦除接收批次；空缓存可复用，容量始终有界
    */
    void clearData();

    /** 擦除输出槽并取消借用，容量可以复用
    */
    void clearOutput();

    /** 在写入秘密前准备完整输出容量，后续不再搬移有效旧副本
    @param [in] size 本次编码长度
    */
    void prepareOutput(uint32_t size);

    /** 返回保持的错误或成功终态，不从业务显示快照反推状态
    @return NI_OK、NI_READY或首个负数错误
    */
    int32_t status() const;

public:
    // 授权元数据，其中offset固定为本次恢复的原始前缀。
    NetInterfaceFileInfo m_info;
    // 本流独占的增量摘要，不接纳外部SHA内部状态。
    NetInterfaceHashHandle m_hash;
    // 收端未落盘的连续字节，size即唯一批次长度；首次接收前预留整批容量。
    std::vector<uint8_t> m_data;
    // 一个完整待发帧，发送背压期间保持地址及内容。
    std::vector<uint8_t> m_output;
    // 唯一流阶段和输出槽类型。
    NetInterfaceFilePhase m_phase;
    // 当前输出槽类别，未消费前保持稳定，消费后归还槽位
    NetInterfaceFileOutput m_outputKind;
    // 已重新计算摘要的持久前缀长度。
    uint64_t m_primed;
    // 发端已交付位置，或收端已持久化位置。
    uint64_t m_position;
    // 发端已由对端确认的持久前缀。
    uint64_t m_acknowledged;
    // 最近推进时刻及最近实际进度时刻。
    uint64_t m_now;
    // 最近实际文件进度对应的单调毫秒，空轮询不会延长期限
    uint64_t m_progress;
    // 当前ACK输出槽包含的前缀，不能跟随新进度改写借用帧。
    uint64_t m_outputAckOffset;
    // 固定产品及无进度超时。
    uint32_t m_product;
    // 无实际进度的超时毫秒，范围1000至60000
    uint32_t m_timeout;
    // DATA输出槽的真实文件字节数，END及ACK为0。
    uint32_t m_outputSize;
    // 收端合并写的协商块数，最大不超过window。
    uint32_t m_writeBatchChunks;
    // 当前ACK输出槽包含的标志。
    uint32_t m_outputAckFlags;
    // FAILED阶段的首错，其他阶段保持NI_OK。
    int32_t m_error;
    // 固定发端/收端角色。
    bool m_sender;
    // 宿主已借用当前批次；完成/失败前禁止追加或改写。
    bool m_writeTaken;
    // 当前持久前缀或终态还有尚未交付的ACK。
    bool m_ackPending;
};