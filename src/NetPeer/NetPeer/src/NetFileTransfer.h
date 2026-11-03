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

    /** 仅done后销毁；不拥有共享I/O队列，不在析构等待线程
    */
    ~NetFileTransfer();

    /** 接管已打开的文件句柄，一个句柄只交给一个传输
    @param [in] io 借用至done，宿主先init
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

    /** 推進摘要前缀/读写/提交及终态关闭；一次至多提交一个磁盘工作
    @param [in] nowMs 同一单调时钟
    @return OK/NEED_MORE运行，READY成功且已释放句柄，负数失败；失败后仍须驱动至done
    */
    int32_t poll(uint64_t nowMs);

    /** 分派共享队列完成结果，调用后宿主consume；不执行磁盘I/O
    @param [in] result io.peek借用结果；接纳任务号后还核对file/operation/offset，READ精确核对长度
    @return true属于本流且已接纳；false交给其他流/控制逻辑
    */
    bool onIoResult(const NetFileIoResult& result);

    /** 接纳已验证网关送来的文件帧
    @param [in] frame DATA/ACK/文件撤销通知
    @return OK接纳，LIMIT_EXCEEDED无副作用背压，其他负数流失败
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
    @param [in] reason 负数，默认CANCELLED；协议已成功时只关闭仍拥有的资源，不改写成功，不回滚已提交文件
    */
    void cancel(int32_t reason = NI_CANCELLED);

    /** 文件句柄与工作均已收敛，可以销毁
    @return 是否已收敛
    */
    bool done() const;

    /** 借用进度快照，不将token写入界面或日志
    @param [out] info 当前信息
    @param [out] result 结果
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
};