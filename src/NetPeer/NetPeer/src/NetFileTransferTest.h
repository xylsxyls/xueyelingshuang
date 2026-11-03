#pragma once
#include "NetPeer.h"
#include <vector>

// 只由隔离main安装的单任务后端，独立维护文件字节与实际释放次数。
class NetFileTransferTestBackend : public NetFileIoBackend
{
public:
    /** 创建未停止、持有测试文件7的后端，不操作磁盘或线程
    */
    NetFileTransferTestBackend();

    /** 解除观察指针
    */
    virtual ~NetFileTransferTestBackend();

    /** 初始化测试后端
    @return OK
    */
    virtual int32_t init();

    /** 分配严格递增编号
    @param [out] id 非零编号
    @return OK
    */
    virtual int32_t nextRequestId(uint64_t& id);

    /** 每项先拒绝两次，接纳时复制WRITE正文
    @param [in] request 当前工作
    @return OK或LIMIT_EXCEEDED
    */
    virtual int32_t submit(const NetFileIoRequest& request);

    /** 取消已接纳工作，释放类工作仍消耗文件号
    @param [in] id 工作号
    @return OK或NOT_FOUND
    */
    virtual int32_t cancel(uint64_t id);

    /** 显式模拟完成一步，READ按独立字节模型返回
    @param [out] result 借用完成正文
    @return READY或NEED_MORE
    */
    virtual int32_t peek(NetFileIoResult& result);

    /** 擦除当前结果及复制正文
    @param [in] id 当前工作号
    @return OK或INVALID_STATE
    */
    virtual int32_t consume(uint64_t id);

    /** 测试后端立即停止并释放尚持有的文件
    */
    virtual void stop();

    /** 检查测试后端停止标志
    @return 是否停止
    */
    virtual bool done() const;

    /** 只供done后调用
    */
    virtual void uninit();

    /** 返回新测试后端
    @return 独占指针
    */
    static NetFileIoBackend* create();

public:
    // 最近创建实例，只由隔离测试执行者读取。
    static NetFileTransferTestBackend* m_current;
    // 独立文件内容和submit复制的正文。
    std::vector<uint8_t> m_content;
    // 模拟文件后端拥有的输入文件字节
    std::vector<uint8_t> m_input;
    // 模拟文件后端拥有的写入结果字节，独立对照原输入
    std::vector<uint8_t> m_output;
    // 唯一接纳任务及其完成结果。
    NetFileIoRequest m_request;
    // 尚未consume的模拟文件操作完成结果
    NetFileIoResult m_result;
    // 编号与最近接纳尝试号，验证拒绝后不复用编号。
    uint64_t m_next;
    // 最后一次尝试提交的请求号，用于检查失败编号不复用
    uint64_t m_attempted;
    // 每项两次可恢复背压，以及累计拒绝/释放/刷新/发布次数。
    uint32_t m_busy;
    // 被受控背压拒绝的工作次数
    uint32_t m_rejected;
    // 实际释放文件所有权的次数
    uint32_t m_released;
    // 实际持久化操作计数
    uint32_t m_flushed;
    // 实际发布操作计数
    uint32_t m_published;
    // 指定操作在已执行释放规则后报告失败，0不注入。
    uint32_t m_failOperation;
    // 持久化前缀，只在WRITE持久成功后推进。
    uint64_t m_durable;
    // 任务已完成、收到取消、后端停止及文件存在。
    bool m_ready;
    // 本轮模拟文件工作是否收到取消
    bool m_cancelled;
    // 模拟后端是否已收到停止请求
    bool m_stopped;
    // 模拟文件号是否仍处于本后端拥有状态
    bool m_file;
    // 用于记录独立模型发现的契约违规。
    bool m_bad;
};

// 文件流状态、失败和所有权测试集合，使用实际文件协议实现
class NetFileTransferTest
{
public:
    /** 运行独立字节模型、背压、取消和完成身份检查
    @return 是否全部通过
    */
    static bool run();

    /** 用有界实例核对正常完成及指定提交边界取消
    @param [in] total 文件长度
    @param [in] chunk 块长
    @param [in] resume 已持久前缀
    @param [in] publish 是否原子发布
    @param [in] cancelOperation 0正常，其他操作在完成前/后取消两端
    @param [in] after 是否在指定操作完成后取消
    @param [in] failOperation 0正常，其他操作注入后端失败
    @return 是否通过
    */
    static bool transfer(uint32_t total, uint32_t chunk, uint32_t resume, bool publish,
        uint32_t cancelOperation, bool after, uint32_t failOperation);

    /** 拒绝匹配编号但其他身份错误的结果，并仅关闭一次
    @return 是否通过
    */
    static bool identity();

    /** 有界驱动一个后端完成，并检查重复/外来完成不会被接纳
    @param [in,out] io 当前后端
    @param [in,out] flow 对应传输
    @return 是否通过
    */
    static bool drain(NetFileIo& io, NetFileTransfer& flow);

    /** 创建固定身份元数据
    @param [in] bytes 独立预期正文
    @param [in] chunk 块长
    @param [in] resume 已持久前缀
    @return 完整协议元数据
    */
    static NetInterfaceFileInfo info(const std::vector<uint8_t>& bytes, uint32_t chunk, uint32_t resume);

public:
    // 隔离单执行者的本轮检查计数。
    static uint32_t m_checks;
};