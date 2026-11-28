#pragma once
#include "NetFileIo.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetFileTransferState;

// 原生文件流驱动器。授权/OPEN RPC由宿主完成；网络执行者非阻塞驱动，磁盘由共享NetFileIo处理。
class NetPeerAPI NetFileTransfer
{
public:
    /** 创建未启动对象
    */
    NetFileTransfer();

    /** 启动前设置接收端每批持久化的块数
    @param [in] chunks 1至8且不超过协商窗口，默认1；发送端使用1
    @return NI_OK或参数、生命周期错误
    */
    int32_t setWriteBatchChunks(uint32_t chunks);

    /** 设置文件流无进度时限，启动前调用
    @param [in] timeoutMs 毫秒，1000至60000，默认15000；接收端提交后，首次 poll 看到待交付 ACK 时开始计算同长度的输出期限
    @return NI_OK或参数、生命周期错误
    */
    int32_t setProgressTimeout(uint32_t timeoutMs);

    /** 仅done后销毁；不拥有共享I/O队列，不在析构等待线程
    */
    ~NetFileTransfer();

    /** 接管已打开的文件句柄，一个句柄只交给一个传输
    @param [in] io 借用至done，宿主先init；停止中、状态查询失败或查询期间发起停止时拒绝接管
    @param [in] file io返回的文件句柄，成功后由本流关闭或持句柄原子发布；后端PUBLISH须处理非零file
    @param [in] info 后端授权、网关映射的TRANSFERRING元信息
    @param [in] product 已注册产品
    @param [in] sender true发送文件、false接收
    @param [in] temporary 接收端自己的临时路径，UTF8；发送端可空
    @param [in] destination 接收端提交路径，不能来自远端路径，可空表示只关闭临时文件
    @param [in] nowMs 同一单调时钟
    @param [in] replace 是否允许覆盖本地目标，默认false
    @return OK已接管，其余负数仍由调用者关闭句柄
    */
    int32_t start(NetFileIo& io, uint64_t file, const NetInterfaceFileInfo& info, uint32_t product, bool sender, const char* temporary,
                  const char* destination, uint64_t nowMs, bool replace = false);

    /** 推进摘要前缀/读写/提交及终态关闭；一次至多提交一个磁盘工作。已接纳工作须经 onIoResult 结算，即使共享后端已结束。
    @param [in] nowMs 同一单调时钟
    @return OK/NEED_MORE运行，READY成功且已释放句柄，负数失败；最终 ACK 持续背压至期限返回 TIMEOUT，已提交文件保留。失败后仍须驱动至done。已有文件任务等待分派或收尾任务提交遇到背压时返回 NEED_MORE，可从 snapshot 读取已知失败；done 后 poll 返回最终结果
    */
    int32_t poll(uint64_t nowMs);

    /** 分派共享队列完成结果；接纳后宿主 consume，当前 NetExecution 的下一次 wait 立即返回
    @param [in] result io.peek借用结果；接纳任务号后还核对file/operation/offset，READ 核对返回字节数，WRITE 核对实际完成长度
    @return true属于本流且已接纳；false交给其他流/控制逻辑
    */
    bool onIoResult(const NetFileIoResult& result);

    /** 接纳已验证网关送来的文件帧
    @param [in] frame DATA/ACK/文件撤销通知
    @return OK接纳，LIMIT_EXCEEDED无副作用背压。已成功完成或已因 LIMIT_EXCEEDED 失败的协议流收到后续帧时返回 NI_NOT_FOUND，保留本地终态。对端失败 ACK 的原因恰为 LIMIT_EXCEEDED 时返回 OK 表示该帧已消费，snapshot 立即报告原始失败，poll 的收尾状态见其返回约定。
    */
    int32_t accept(const NetInterfaceFrame& frame);

    /** 借用一个网络输出，postData/send接纳后才能consume
    @param [out] frame 输出借用到consumeOutput/cancel/析构
    @return READY、NEED_MORE或负数
    */
    int32_t peekOutput(NetInterfaceFrame& frame);

    /** 消费已被网络接纳的输出
    @return OK或负数
    */
    int32_t consumeOutput();

    /** 停止本流；宿主另发FILE_DATA_TYPE无体CANCEL使网关撤销流
    @param [in] reason 本地取消原因，默认 CANCELLED。已结算发布但最终 ACK 尚未消费时，保留文件 COMPLETED，停止输出并报告该原因；消费最终 ACK 后取消保留成功。取消在途 PUBLISH 时仍须分派后端完成结果
    */
    void cancel(int32_t reason = NI_CANCELLED);

    /** 文件句柄与工作均已收敛，可以销毁
    @return 是否已收敛
    */
    bool done() const;

    /** 借用进度快照，不将token写入界面或日志
    @param [out] info 当前信息
    @param [out] result 协议进度或本地首个错误；文件已提交时 info 可为 COMPLETED，而本地关闭或最终 ACK 交付失败
    */
    void snapshot(NetInterfaceFileInfo& info, int32_t& result) const;

private:
    /** 准备唯一待接纳工作，不分配编号；WRITE缓存借用至submit接纳或取消
    @param [in] operation 操作
    @param [in] offset 偏移
    @param [in] bytes WRITE数据，可空
    @param [in] size 长度
    */
    void prepare(uint32_t operation, uint64_t offset = 0, const uint8_t* bytes = nullptr, uint32_t size = 0);

    /** 为当前待接纳工作分配新编号并投递，背压保留正文及操作
    @return OK已接纳，LIMIT_EXCEEDED稍后重试，其他负数停止本流
    */
    int32_t submit();

    /** 禁止复制所有权
    @param [in] other 原对象
    */
    NetFileTransfer(const NetFileTransfer& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetFileTransfer& operator=(const NetFileTransfer& other);

private:
    // 独占协调状态；所有操作由同一执行者调用，生命周期结束前须done。
    NetFileTransferState* m_state;
    // 启动前确定的接收合并块数
    uint32_t m_writeBatchChunks;
    // 文件流无进度期限，毫秒
    uint32_t m_progressTimeout;
};