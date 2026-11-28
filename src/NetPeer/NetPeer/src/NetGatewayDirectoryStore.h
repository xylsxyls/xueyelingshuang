#pragma once
#include "NetFileIo.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetGatewayDirectoryStoreState;

// 公开候选缓存的异步协调；全部操作由同一执行者串行调用，仅通过文件后端工作
class NetPeerAPI NetGatewayDirectoryStore
{
public:
    /** 分配静止协议状态，不创建文件工作后端
    */
    NetGatewayDirectoryStore();

    /** 指定两次缓存写尝试的最小间隔
    @param [in] writeIntervalMs 毫秒，0允许立即写
    */
    explicit NetGatewayDirectoryStore(uint32_t writeIntervalMs);

    /** 拥有方必须先shutdown并poll至stopped，禁止在回调销毁
    */
    ~NetGatewayDirectoryStore();

    /** 旧同步读取明确不支持，迁移为open/poll/takeLoaded
    @param [in] path 不读取
    @param [out] points 保持调用者原值，不跨模块清理调用者容器
    @return NOT_SUPPORTED
    */
    static int32_t load(const std::string& path, std::vector<NetInterfaceDirectoryPoint>& points);

    /** 接纳异步加载，路径只作为宿主文件参数，父目录由临时创建工作准备
    @param [in] path 非空UTF8路径，最多4096字节，不接受远端提供的文件名
    @param [in] entropy 临时名强随机，可空采用已配置字节宿主熵；上下文借用至shutdown
    @param [in] context entropy上下文
    @return OK接纳、NOT_SUPPORTED缺后端或负数，不表示磁盘已读取
    */
    int32_t open(const std::string& path, NetInterfaceEntropyFunction entropy = nullptr, void* context = nullptr);

    /** 取一次已加载且校验有效的公开候选，仍不构成网络认证
    @param [out] points 调用者数组，每项预填v1 size/version；失败不改数组
    @param [in] capacity 调用方数组容量，允许空数组及0查询必需项数
    @param [out] count 必需项数，不得与points重叠
    @return READY、NEED_MORE、BUFFER_TOO_SMALL或参数错误；只有READY消费快照
    */
    int32_t takeLoaded(NetInterfaceDirectoryNodeData* points, uint32_t capacity, uint32_t& count);

    /** 从调用者v1值数组合并最新快照，不转交或跨模块释放容器。
    @param [in] points 借用至返回，统一由NI校验
    @param [in] count 非零节点数，所有节点编码后的目录正文合计最多16 KiB
    @return OK接纳最新快照。目标路径超过4059字节返回INVALID_ARGUMENT，仍可继续读取
    */
    int32_t queue(const NetInterfaceDirectoryNodeData* points, uint32_t count);

    /** 合并一份最新有界快照，不覆盖正在写出的快照
    @param [in] points 完整合法候选
    @return OK接纳。目标路径超过4059字节返回INVALID_ARGUMENT，磁盘提交结果由poll推进
    */
    int32_t queue(const std::vector<NetInterfaceDirectoryPoint>& points);

    /** 每次最多接纳/结算一个有界文件工作，不等待磁盘
    同一 NetExecutionScope 中按 begin、poll、wait 推进；open/queue/shutdown 可在 poll 后调用
    文件完成、提交背压或停止尚未收敛时，向当前执行者登记最多100毫秒的补查等待
    未绑定执行者时由宿主自行提供完成通知或有限定时推进
    @return 最近缓存结果，缓存错误不能解释为业务认证成功
    */
    int32_t poll();

    /** 请求停止并丢弃待写快照，临时文件由后端回收。首次停止要求立即推进，重复调用不忙等
    已停止时可重复调用，随后可直接 open
    */
    void shutdown();

    /** 是否已经停止所有文件工作并释放后端
    @return true后可销毁或重新open
    */
    bool stopped() const;

    /** 仅用于可丢弃缓存过期，不参与网络期限
    @return UTC毫秒
    */
    static uint64_t utc();

private:
    /** 构造当前阶段请求，背压不推进阶段
    @return OK接纳、LIMIT_EXCEEDED未接纳或负数
    */
    int32_t submit();

    /** 消费前解释本次结果，借用字节不保存到下一poll
    @param [in] result 当前唯一工作结果
    */
    void complete(const NetFileIoResult& result);

    /** 禁止复制独占文件协调
    @param [in] other 原对象
    */
    NetGatewayDirectoryStore(const NetGatewayDirectoryStore& other);

    /** 禁止赋值独占文件协调
    @param [in] other 原对象
    @return 不可调用
    */
    NetGatewayDirectoryStore& operator=(const NetGatewayDirectoryStore& other);

private:
    // 仅本库维护状态，公开类不暴露STL布局
    NetGatewayDirectoryStoreState* m_state;
};