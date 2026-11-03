#pragma once
#include "NetInterfaceFile.h"

class NetInterfaceFileFlowState;

// 一个流一个执行者。文件I/O由宿主异步完成，默认缓存一块，可选合并至协商窗口，不创建线程。
class NetInterfaceAPI NetInterfaceFileFlow
{
public:
    /** 从经过授权/网关映射的TRANSFERRING信息创建流
    @param [in] info 固定信息，token/stream必须有效
    @param [in] product 非零产品
    @param [in] sender true发DATA、收ACK；false相反
    @param [in] nowMs 单调时间
    @param [in] timeoutMs 无实际进度时限1000至60000，默认15000
    */
    NetInterfaceFileFlow(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs, uint32_t timeoutMs = 15000);

    /** 擦除秘密/缓存并释放摘要，不执行宿主文件I/O
    */
    ~NetInterfaceFileFlow();

    /** 接收宿主可在开始前选择合并写，默认仍逐块写入
    @param [in] chunks 1至已协商window；宿主须支持chunks乘chunk的单次写入和持久化
    @return OK；发端、已开始接收或超出窗口返回错误；线上DATA格式不变
    */
    int32_t setWriteBatchChunks(uint32_t chunks);

    /** 恢复前按顺序计算已确认前缀摘要，不信任对端传来的SHA内部状态
    @param [in] offset 本次前缀位置，必须是当前未补齐位置；拒绝旧I/O结果
    @param [in] bytes 宿主读取的文件前缀，仅调用时借用
    @param [in] size 最大16MiB，不超过恢复偏移剩余长度
    @return OK或负数；前缀未补齐不得传输
    */
    int32_t prime(uint64_t offset, const uint8_t* bytes, uint32_t size);

    /** 更新时钟并检查固定无进度期限，重复ACK不延长期限
    @param [in] nowMs 同一单调时间
    @return OK在传输，READY成功完成，NEED_MORE待前缀/文件提交，负数失败
    */
    int32_t progress(uint64_t nowMs);

    /** 发端申请下一有界文件读取
    @param [out] offset 成功时文件偏移，否则0
    @param [out] size 成功时精确所需长度，否则0
    @return READY可读，NEED_MORE等额度/输出/结束，负数错误
    */
    int32_t readRequest(uint64_t& offset, uint32_t& size) const;

    /** 提交读取结果，只有长度精确且有额度才接纳，复制后宿主可释放
    @param [in] offset 对应readRequest位置，拒绝旧结果
    @param [in] bytes 精确readRequest长度
    @param [in] size 长度
    @return OK或负数，LIMIT_EXCEEDED无副作用
    */
    int32_t produced(uint64_t offset, const uint8_t* bytes, uint32_t size);

    /** 接纳另一端的完整数据/确认，核对产品、流号、秘密、偏移及摘要
    @param [in] frame 借用完整帧
    @return OK已接纳，LIMIT_EXCEEDED当前写未完成且无副作用，可暂停网络输入后重试；负数流失败
    */
    int32_t accept(const NetInterfaceFrame& frame);

    /** 请求异步写入当前块或有界批次，返回借用数据，宿主投递时须复制
    @param [out] offset 文件位置
    @param [out] bytes 借用至written/cancel/destroy
    @param [out] size 长度
    @return READY取一次，NEED_MORE无新工作，负数错误
    */
    int32_t writeRequest(uint64_t& offset, const uint8_t*& bytes, uint32_t& size);

    /** 只有字节写入且持久化成功后确认，随后才能发累计ACK
    @param [in] offset 对应工作偏移
    @param [in] size 对应精确长度
    @param [in] result NI_OK实际落盘，负数错误
    @return OK或负数，过时/不匹配工作拒绝
    */
    int32_t written(uint64_t offset, uint32_t size, int32_t result);

    /** 整文件摘要验证成功后，宿主取一次文件提交请求
    @return READY须异步关闭/提交文件，NEED_MORE暂无，负数错误
    */
    int32_t takeCommit();

    /** 实际提交成功之后才允许发最终成功ACK
    @param [in] result NI_OK已经提交，负数实际失败
    @return READY成功终态或负数
    */
    int32_t committed(int32_t result);

    /** 借用待发DATA/ACK；背压时不消费，下一次取得同一内容
    @param [out] frame 成功借用至consumeOutput/cancel/destroy，否则清空
    @return READY有输出，NEED_MORE等待，负数错误
    */
    int32_t peekOutput(NetInterfaceFrame& frame);

    /** 下层接纳完整帧之后才消费，发送/入队不代表文件已提交
    @return OK或INVALID_STATE
    */
    int32_t consumeOutput();

    /** 本地取消/断线，不回滚已确认前缀，不伪造远端履约
    @param [in] reason 负数错误，默认CANCELLED
    */
    void cancel(int32_t reason = NI_CANCELLED);

    /** 当前进度显示快照；失败/完成stream清零，不当作离线授权凭据
    @param [out] info 快照
    @param [out] result 当前结果
    */
    void snapshot(NetInterfaceFileInfo& info, int32_t& result) const;

private:
    /** 发端验证累计/最终ACK，不让重复确认延长超时
    @param [in] frame 已核对产品及流号的输入
    @return OK或保持首错
    */
    int32_t acceptAck(const NetInterfaceFrame& frame);

    /** 收端接纳连续块；相同未落盘副本不占新空间也不提前确认
    @param [in] frame 已核对产品及流号的输入
    @return OK、无副作用LIMIT_EXCEEDED或保持首错
    */
    int32_t acceptData(const NetInterfaceFrame& frame);

    /** 锁定首错，收端可回报失败ACK
    @param [in] result 负数
    @return 同一错误
    */
    int32_t fail(int32_t result);

    /** 禁止复制独占流
    @param [in] other 原对象
    */
    NetInterfaceFileFlow(const NetInterfaceFileFlow& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetInterfaceFileFlow& operator=(const NetInterfaceFileFlow& other);

private:
    // 独占协议状态，所有入口由同一执行者调用，不拥有宿主文件或线程。
    NetInterfaceFileFlowState* m_state;
};