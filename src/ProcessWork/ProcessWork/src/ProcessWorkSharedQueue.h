#pragma once
#include "ProcessWorkStorage.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <utility>
#include <vector>

/** 本地发送线程登记缓存；同一记录仅由对应实际线程使用，通道拥有其生命期
*/
class ProcessWorkProducerState
{
public:
    /** 建立尚未分配共享下标的登记缓存
    */
    ProcessWorkProducerState();

public:
    // 共享登记下标，0尚未申请
    uint32_t m_index;
    // 借用本通道稳定映射，关闭前有效
    ProcessWorkSharedStream* m_shared;
    // 本地元数据映射在通道静止关闭前稳定；仅当前物理发送线程借用
    ProcessWorkPage* m_bodyPage;
    // 上述缓存对应的正文页号，UINT32_MAX表示尚未命中
    uint32_t m_bodyPageIndex;
    // 缓存当前尾段地址，切换同一共享流的其他通道后必须重新核对完整代次
    ProcessWorkSharedNode* m_tailNode;
    // 缓存尾段的完整代次及下标，0表示尚未命中
    int64_t m_tail;
};

/** 唯一接收线程维护的流读取缓存，不进入共享内存
*/
class ProcessWorkReadState
{
public:
    /** 建立一个已发布流的读取缓存
    @param [in] index 登记下标
    @param [in] stream 借用稳定共享记录
    */
    ProcessWorkReadState(uint32_t index, ProcessWorkSharedStream* stream);

public:
    // 共享流下标
    uint32_t m_index;
    // 借用稳定流元数据
    ProcessWorkSharedStream* m_shared;
    // 缓存当前读段的元数据地址
    ProcessWorkSharedNode* m_node;
    // 当前读段的下标及代次
    int64_t m_head;
};

/** 每发送线程一条SPSC段链，唯一接收器按有界配额轮转；跨流不保证全局FIFO。
    同线程经多个本地通道对象仍绑定同一共享流。完整正文单次release提交，不竞争公共尾部。
    资源不足时登记等待请求；接收器把归还资源直接授予排队者，新请求不能反复插队。
*/
class ProcessWorkAPI ProcessWorkSharedQueue
{
public:
    /** 借用稳定存储和控制区，使用者全部停止后才能销毁
    @param [in] storage 稳定存储
    @param [in,out] control 本代共享控制区
    */
    ProcessWorkSharedQueue(ProcessWorkStorage& storage, ProcessWorkSharedControl& control);

    /** 在静止期销毁本地登记及读取缓存
    */
    ~ProcessWorkSharedQueue();

    /** 尚无生产者且外部静止时配置本地登记Map
    @param [in] config 节点和值池配置
    @return true成功，false参数无效或准备失败
    */
    bool configureProducers(const LockFreeMapConfig& config);

    /** 唯一接收器准备初始元数据页和段池
    @return true成功
    */
    bool initialize();

    /** 启用正文扩容所附带的一个段
    @param [in] index 正文下标
    @return true成功
    */
    bool activate(uint32_t index);

    /** 绑定调用线程的共享发布端；尚未准备的元数据页由外层按重试策略处理
    @param [in] token 已批准的发送进程登记号
    @param [out] failure 可空，仅失败时读取，区分待准备、容量、分配及实际错误
    @param [in] retryContention true在本地登记表内重试CAS竞争；容量不足总是交给外层处理
    @return 借用本地登记缓存，未准备好或分配失败返回nullptr
    */
    ProcessWorkProducerState* producer(int64_t token, int32_t* failure = nullptr, bool retryContention = true);

    /** 取得独占空段并初始化新代次
    @param [in] token 所属进程登记号
    @param [in] extent 正文最大下标
    @param [in] start 扫描提示
    @param [out] index 取得的段下标
    @param [in] serving true仅供唯一接收器兑现排队请求
    @param [out] contended 可空，置true表示确实发生过空段CAS竞争失败
    @return true成功
    */
    bool reserve(int64_t token, uint32_t extent, uint32_t start, uint32_t& index, bool serving = false, bool* contended = nullptr);

    /** 归还尚未提交的候选段
    @param [in] index 段下标
    @param [in] token 原所有者
    */
    void cancel(uint32_t index, int64_t token);

    /** 向本调用线程的流提交一条已完成正文，有界失败完全不提交
    @param [in] token 正文所属发送进程
    @param [in] bufferIndex 正文下标
    @param [in] inlined 是否内联正文
    @param [in] extent 正文最大下标
    @param [in,out] hint 段扫描提示
    @return true已提交
    */
    bool enqueue(int64_t token, int32_t bufferIndex, bool inlined, uint32_t extent, uint32_t& hint);

    /** 使用已绑定发布端提交，避免再次查询线程登记
    @param [in,out] producer 仅当前发送线程使用的登记
    @param [in] bufferIndex 已完成正文下标
    @param [in] inlined 正文位置标志
    @param [in] extent 正文最大下标
    @param [in,out] hint 扫描提示
    @param [in] granted 已授予候选段，UINT32_MAX表示未提供
    @param [out] contended 可空，置true表示段预留发生CAS竞争，调用方先初始化为false
    @return true已提交，false未提交
    */
    bool enqueue(ProcessWorkProducerState& producer, int32_t bufferIndex, bool inlined, uint32_t extent, uint32_t& hint,
        uint32_t granted = UINT32_MAX, bool* contended = nullptr);

    /** 是否存在尚未完成分配的公告；归还资源时优先直接授予排队者
    @return true需要服务等待请求，尚未授予的FREE资源仍可正常申请
    */
    bool hasWaiters() const;

    /** 发布资源请求；同一流同一时刻最多一个，重复调用不重复登记
    @param [in,out] producer 当前发送线程登记
    @param [in] node true申请段，false申请正文
    */
    void request(ProcessWorkProducerState& producer, bool node);

    /** 接收接收器已经授予的资源
    @param [in,out] producer 当前发送线程登记
    @param [in] node 请求类型
    @param [out] index 资源下标
    @return true转移所有权，false仍等待
    */
    bool takeGrant(ProcessWorkProducerState& producer, bool node, uint32_t& index);

    /** 取消尚未领取的申请，由接收器清理排队项及可能交错到达的授予
    @param [in,out] producer 当前发送线程登记
    */
    void cancelRequest(ProcessWorkProducerState& producer);

    /** 唯一接收器准备登记页、收集公告并有界分配空闲资源
    @param [in] extent 正文最大下标
    */
    void service(uint32_t extent);

    /** 唯一接收器归还已经复制完成的正文，优先直接交给老等待者
    @param [in] index 正文下标
    */
    void releaseBody(uint32_t index);

    /** 唯一接收器复用本轮已核验的正文地址归还资源，避免再次查找稳定目录
    @param [in] index 正文下标，必须与body相符
    @param [in,out] body 尚未归还的完整正文元数据，本次调用期间保持有效
    */
    void releaseBody(uint32_t index, ProcessWorkSharedBlock& body);

    /** 确认进程死亡后取消其资源申请，已提交消息仍由正常读取路径排空
    @param [in] token 已确认失效的进程登记号
    */
    void recoverToken(int64_t token);

    /** 为死亡回收标记所有未消费消息及仍在队列中的段
    @param [in] extent 正文最大下标
    @param [out] nodes 按nodeAt的紧凑序号标记
    @param [out] bodies 按正文下标标记
    @return true完整遍历，false协议损坏或资源不足
    */
    bool markCommitted(uint32_t extent, std::vector<unsigned char>& nodes, std::vector<unsigned char>& bodies);

    /** 取得普通段或登记附带段的稳定元数据
    @param [in] index 段编码下标
    @return 元数据或nullptr
    */
    ProcessWorkSharedNode* node(uint32_t index);

    /** 段池紧凑遍历所需的数量
    @param [in] extent 正文最大下标
    @return 普通段加登记附带段数量
    */
    uint32_t nodeCount(uint32_t extent) const;

    /** 把紧凑序号转换成段编码下标
    @param [in] ordinal 紧凑序号
    @param [in] extent 正文最大下标
    @return 编码下标
    */
    uint32_t nodeAt(uint32_t ordinal, uint32_t extent) const;

    /** 窥视下一条完整消息，连续窥视不会越过尚未consume的消息
    @param [out] slot 本次位置标识
    @param [out] bufferIndex 正文下标，负数表示损坏
    @return true有消息
    */
    bool front(uint32_t& slot, int32_t& bufferIndex);

    /** 窥视并获取正文位置
    @param [out] slot 位置标识
    @param [out] bufferIndex 正文下标
    @param [out] inlined 是否内联
    @return true有消息
    */
    bool front(uint32_t& slot, int32_t& bufferIndex, bool& inlined);

    /** 唯一接收器确认本次窥视的正文已经拥有独立副本
    @param [in] slot 最近一次成功front的位置标识
    @param [in] bytes 本次复制字节数，用于限制一个流的连续搬运预算；0只按消息数计算
    @return true消费成功，false没有相应窥视或重复消费
    */
    bool consume(uint32_t slot, uint32_t bytes = 0);

private:
    /** 当前段已满时准备后继并发布链接，调用方已经核验当前写入游标
    @param [in,out] producer 当前物理发送线程的登记
    @param [in,out] last 已填满的尾段
    @param [in] tail 该段的完整代次及编码下标
    @param [in] payload 正文下标及内联标记
    @param [in] extent 正文最大下标
    @param [in,out] hint 段扫描起点提示
    @param [in] granted 已授予段，UINT32_MAX为自行申请
    @param [out] contended 可空；实际CAS失败时置true
    @return true已发布新消息，false未提交
    */
    bool enqueueSegment(ProcessWorkProducerState& producer, ProcessWorkSharedNode& last, int64_t tail,
        int64_t payload, uint32_t extent, uint32_t& hint, uint32_t granted, bool* contended);

    /** 有等待请求时，归还正文优先直接授予旧等待者
    @param [in] index 正文下标
    @param [in,out] body 已核验的正文元数据
    */
    void releaseBodyToWaiter(uint32_t index, ProcessWorkSharedBlock& body);

    /** 收集新登记流，不在读取热路径分配逐包对象
    @return true成功或无变化
    */
    bool refreshStreams();

    /** 唯一接收器移出已确认死亡且排空的流，保留其稳定元数据及缓存所有权
    */
    void compactStreams();

    /** 移到本轮的下一流；到达本轮末尾后才让新增流参加下一轮
    */
    void advanceRead();

    /** 收集公告，分别追加正文/段等待链；只由接收器调用
    */
    void refreshRequests();

    /** 取下一名未取消的等待者，删除取消项
    @param [in] kind 0正文、1段
    @param [out] index 等待者登记下标
    @return 等待者或nullptr
    */
    ProcessWorkSharedStream* pending(uint32_t kind, uint32_t& index);

    /** 弹出等待者并发布资源授予
    @param [in] kind 0正文、1段
    @param [in,out] stream 等待记录
    @param [in] resource 资源下标
    */
    void grant(uint32_t kind, ProcessWorkSharedStream& stream, uint32_t resource);

    /** 唯一接收器归还段，优先满足排队者
    @param [in] index 已退役段下标
    */
    void releaseNode(uint32_t index);

    /** 线程快缓存未命中后建立或查找登记；隔离准备阶段的分配及异常回滚
    @param [in] token 本进程在本代接收器上的身份
    @param [out] failure 可空，失败时记录分类结果
    @param [in] retryContention 是否在登记Map内部重试竞争
    @return 由本队列拥有的稳定状态或nullptr
    */
    ProcessWorkProducerState* registerProducer(int64_t token, int32_t* failure, bool retryContention);

    /** 重新初始化已经独占的段，代次耗尽时永久退役
    @param [in,out] node 已独占的段
    @return true可以用于新提交
    */
    bool resetNode(ProcessWorkSharedNode& node);

    /** 读取指定流的当前单元并跨越已耗尽段
    @param [in,out] reader 唯一接收器的读取缓存
    @param [out] slot 位置标识
    @param [out] bufferIndex 正文下标
    @param [out] inlined 正文位置
    @return true有消息
    */
    bool readFront(ProcessWorkReadState& reader, uint32_t& slot, int32_t& bufferIndex, bool& inlined);

    /** 禁止复制资源拥有者
    @param [in] other 不可复制来源
    */
    ProcessWorkSharedQueue(const ProcessWorkSharedQueue& other);

    /** 禁止覆盖资源拥有者
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkSharedQueue& operator=(const ProcessWorkSharedQueue& other);

private:
    // 借用本代存储
    ProcessWorkStorage* m_storage;
    // 借用本代共享控制区
    ProcessWorkSharedControl* m_control;
    // 本地对象唯一标识，避免TLS缓存命中已经析构后复用的地址
    uint64_t m_instance;
    // 首次登记的本地缓存，运行期间不删除其对象
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMap<std::pair<int64_t, uint64_t>, std::shared_ptr<ProcessWorkProducerState>> m_producers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 按流编号拥有稳定读取缓存
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::vector<std::unique_ptr<ProcessWorkReadState>> m_readers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 已完整登记流的轮转列表，元素地址由m_readers拥有
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::vector<ProcessWorkReadState*> m_active;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收器已观察的登记公告代次
    int64_t m_seenStreamEpoch;
    // 接收器已收集的申请公告代次
    int64_t m_seenRequestEpoch;
    // 正文/段申请的两个FIFO链首
    uint32_t m_pendingHead[2];
    // 正文/段申请的两个FIFO链尾
    uint32_t m_pendingTail[2];
    // 已准备的元数据页数
    uint32_t m_preparedPages;
    // 接收器寻找空闲正文的轮转提示
    uint32_t m_freeCursor;
    // 接收器寻找空闲段的提示
    uint32_t m_nodeCursor;
    // 当前轮转位置
    size_t m_readCursor;
    // 本轮开始时的活动列表上界，新加入的流不能持续延长旧流等待的这一轮
    size_t m_roundCount;
    // 当前流本轮已消费条数
    uint32_t m_readQuota;
    // 当前流本轮已复制字节数，避免大消息连续占用整轮搬运时间
    uint64_t m_readBytes;
    // 有死亡流排空后才压缩活动列表，不逐包移动指针
    bool m_compactStreams;
    // 最近成功窥视的流，consume之前保持固定
    ProcessWorkReadState* m_frontReader;
    // 最近窥视的位置标识
    uint32_t m_frontSlot;
    // 最近窥视的段内位置，-1表示没有窥视
    int32_t m_frontOffset;
};

#include "ProcessWorkSharedQueue.inl"