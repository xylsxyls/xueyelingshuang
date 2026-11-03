#pragma once
#include "NetInterfaceTunnelTypes.h"
#include "NetChannel/NetChannelAPI.h"

// 原生与Wasm共用的NTW3拆包，不持有密钥，不解压/解密，不解释业务消息。
class NetInterfaceAPI NetInterfaceTunnel
{
public:
    /** 创建有界空解码器
    */
    NetInterfaceTunnel();

    /** 在分配缓冲的本模块释放资源
    */
    ~NetInterfaceTunnel();

    /** 只消费到一条完整记录；未接纳后缀仍由宿主持有
    @param [in] data 原始网络/IPC字节，size为0可空
    @param [in] size 字节数
    @param [out] consumed 实际消费，READY未consume时始终0
    @return NI_READY、NI_NEED_MORE或持续负数错误
    */
    int32_t feed(const uint8_t* data, uint32_t size, uint32_t& consumed);

    /** 借用完整外层记录
    @param [out] record 当前借用视图，非READY清零
    @return NI_READY、NI_NEED_MORE或持续错误
    */
    int32_t peek(NetInterfaceTunnelRecord& record) const;

    /** 消费后释放借用视图，容量仍不超过16KiB
    @return NI_OK或当前错误/INVALID_STATE
    */
    int32_t consume();

    /** 检查EOF，半个头/体必须报截断
    @return NI_OK、NI_READY或持续错误
    */
    int32_t finish();

    /** 编码显式v3路由记录，外层字段不能当作账号或业务成功证明
    @param [in] record 控制通道为0；业务/关闭通道非0；单块至多16KiB
    @param [out] output 调用者空间，不允许与record数据重叠
    @param [in] capacity 空间，探测时0且output可空
    @param [out] required 合法输入所需大小，非法输入为0
    @return NI_OK、NI_BUFFER_TOO_SMALL或负数，拒绝时不改输出
    */
    static int32_t encode(const NetInterfaceTunnelRecord& record, uint8_t* output, uint32_t capacity, uint32_t& required);

private:
    /** 禁止复制持有活动协议状态的对象
    @param [in] other 不可复制的实例
    */
    NetInterfaceTunnel(const NetInterfaceTunnel& other);

    /** 禁止赋值持有活动协议状态的对象
    @param [in] other 不可赋值的实例
    @return 不可调用
    */
    NetInterfaceTunnel& operator=(const NetInterfaceTunnel& other);

private:
    // 旧公开入口只委托，实际记录状态由底层NetChannel独占
    NetChannel m_decoder;
};