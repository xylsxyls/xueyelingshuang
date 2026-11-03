#pragma once
#include "NetInterfaceHandles.h"

// 协议主入口：提供NIF2解码实例及消息、TLS、会话、文件等工作对象的创建和操作
// 各工作对象由单执行者推进、在本库释放；不拥有socket、OS线程或业务授权判定
// 完整32字节固定头通过校验后才分配正文，READY借用直到consume/reset/析构
class NetInterfaceAPI NetInterface
{
public:
    /** 创建空解码器，非法限额形成持续INVALID_ARGUMENT
    @param [in] payloadLimit 单帧明文及压缩记录体上限，1至16MiB
    */
    explicit NetInterface(uint32_t payloadLimit = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT);

    /** 在创建模块释放独占存储，调用方先结束所有借用
    */
    ~NetInterface();

    /** 单执行者接纳到一条完整帧为止，READY时保留调用方的剩余输入
    @param [in] data 至少size字节的借用输入，仅size为0时可空
    @param [in] size 可读字节数，不得与实例借出的存储重叠
    @param [out] consumed 本次复制的字节数，包含拒绝的完整固定头；参数错误为0
    @return NEED_MORE、READY或持续错误；分配、协议和截断错误须关闭当前连接
    */
    int32_t feed(const uint8_t* data, uint32_t size, uint32_t& consumed);

    /** 借用完整消息，不复制正文
    @param [out] frame READY时借用到consume/reset/析构，其余状态清零
    @return READY或当前状态
    */
    int32_t peek(NetInterfaceFrame& frame) const;

    /** 结束当前借用，复用至多4KiB的小存储；更大正文立即释放
    @return OK、不完整时INVALID_STATE或已有持续错误
    */
    int32_t consume();

    /** 报告输入结束，完整待消费帧仍须由调用方消费
    @return 空闲为OK、待消费为READY，半帧为持续TRUNCATED
    */
    int32_t finish();

    /** 新连接时释放全部存储和错误，保留构造限额及显式压缩许可
    */
    void reset();

    /** 单执行者读取当前状态
    @return NEED_MORE、READY或持续错误
    */
    int32_t status() const;

    /** 单执行者读取创建时确定的单帧上限
    @return 正文字节上限，非法配置仍返回原值
    */
    uint32_t payloadLimit() const;

    /** 编码固定大端头和正文；多线程可使用互不重叠的输出独立调用
    @param [in] frame 借用元数据及完整正文，输出可覆盖元数据但不得与正文重叠
    @param [out] output 调用方可写区域，capacity为0时可空
    @param [in] capacity 可写字节数，实际只写required字节
    @param [out] required 所需字节数，参数错误为0；不得与输入或输出别名
    @return OK、BUFFER_TOO_SMALL、LIMIT_EXCEEDED或INVALID_ARGUMENT，失败不修改输出
    */
    static int32_t encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required);

    /** 由单执行者在应用握手共同确认扩展后设置，不改变已验证的当前帧
    @param [in] allowed 是否允许下一条压缩RPC帧，其他种类仍拒绝压缩
    */
    void allowCompression(bool allowed);

    /** 借用本模块编译内置的固定CA公开证书，不提供动态替换信任根的入口
    @param [out] size 可空，成功为包含末尾NUL的PEM字节数
    @return 生命周期为当前模块生命周期的只读字节，不得释放
    */
    static const uint8_t* trustAuthority(uint32_t* size);

    // 跨模块工作对象由本库创建与回收；辅助实现保持原协议、预算及错误边界
    /** 创建大文件增量摘要，不一次加载整个文件
    @param [out] handle 不可空，失败清零
    @return NI_OK或负数
    */
    static int32_t hashCreate(NetInterfaceHashHandle* handle);

    /** 按文件顺序提交字节，包括恢复前已经落盘的前缀
    @param [in] handle 当前未完成摘要
    @param [in] input size非零时不可空，只借用到返回
    @param [in] size 单次最大16MiB，总量不超过SHA256算法的2^61-1字节
    @return NI_OK或负数，错误保持至销毁
    */
    static int32_t hashUpdate(NetInterfaceHashHandle handle, const uint8_t* input, uint32_t size);

    /** 完成一次摘要；空间探测不足不改变状态
    @param [in] handle 未完成摘要
    @param [out] output 不可空，调用者拥有
    @param [in] capacity 至少32字节
    @return NI_OK、BUFFER_TOO_SMALL或负数；成功后不得再次update/finish
    */
    static int32_t hashFinish(NetInterfaceHashHandle handle, uint8_t* output, uint32_t capacity);

    /** 释放并擦除摘要，调用方不释放其内部资源
    @param [in] handle 有效上下文或null
    */
    static void hashDestroy(NetInterfaceHashHandle handle);

    /** 创建原生/Wasm共用的TLS和完整帧驱动
    @param [in] options 不可空，证书调用时借用，随机回调上下文借用至destroy
    @param [out] handle 不可空；失败清空
    @param [out] detail 可空，TLS底层诊断
    @return TLS_HANDSHAKE或负数错误，未代表账号已登录
    */
    static int32_t connectionCreate(const NetInterfaceTlsOptions* options, NetInterfaceConnectionHandle* handle,
                                                         int32_t* detail);

    /** 逻辑服务业务TLS客户端，固定内置CA及service.<目标号>域名，不能注入动态信任根
    @param [in] product 逻辑目标服务号；0的可达权限仍由管理路由授权
    @param [in] entropy 宿主强随机
    @param [in] context 随机上下文，借用至destroy
    @param [in] timeoutMs 100至60000毫秒
    @param [out] handle 成功创建的业务TLS连接，否则null
    @param [out] detail 可空，TLS诊断
    @return TLS_HANDSHAKE或负数，不代表已连接或账号认证
    */
    static int32_t connectionCreateServiceClient(uint32_t product, NetInterfaceEntropyFunction entropy,
        void* context, uint32_t timeoutMs, NetInterfaceConnectionHandle* handle, int32_t* detail);

    /** 释放连接和拥有的队列
    @param [in] handle 实例或null，禁止重复释放
    */
    static void connectionDestroy(NetInterfaceConnectionHandle handle);

    /** 提交密文，不接纳的后缀仍由宿主拥有
    @param [in] handle 有效连接
    @param [in] data size为0时可空
    @param [in] size 字节数
    @param [out] consumed 不可空，已复制长度
    @return TLS状态或负数错误
    */
    static int32_t connectionFeed(NetInterfaceConnectionHandle handle, const uint8_t* data, uint32_t size,
                                                       uint32_t* consumed);

    /** 有界驱动握手、加密和完整帧读取
    @param [in] handle 有效连接
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return TLS状态或持续错误；READY只表示TLS完成
    */
    static int32_t connectionProgress(NetInterfaceConnectionHandle handle, uint32_t timeLow, uint32_t timeHigh);

    /** 排队完整帧；普通最多2MiB/64项，控制独立64KiB/16项
    @param [in] handle 有效且TLS就绪的连接
    @param [in] fields 六项kind/product/request低位/高位/type/payloadSize
    @param [in] payload 字节，当fields[5]为0时可空
    @return NI_OK接纳、NI_LIMIT_EXCEEDED背压或其他错误
    */
    static int32_t connectionSend(NetInterfaceConnectionHandle handle, const uint32_t* fields, const uint8_t* payload);

    /** 借用一条完整、经过TLS验证的帧
    @param [in] handle 有效连接
    @param [out] fields 不可空，六项与Send相同
    @param [out] payload 不可空，借用至consume或destroy
    @return NI_READY、NI_NEED_MORE或负数错误
    */
    static int32_t connectionPeek(NetInterfaceConnectionHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 消费peek后的完整帧，允许下次progress读取后续帧
    @param [in] handle 有效连接
    @return NI_OK或错误
    */
    static int32_t connectionConsume(NetInterfaceConnectionHandle handle);

    /** 借用连续密文输出
    @param [in] handle 有效连接
    @param [out] data 不可空，消费前有效
    @param [out] size 不可空，连续长度
    @return TLS状态或错误
    */
    static int32_t connectionPeekOutput(NetInterfaceConnectionHandle handle, const uint8_t** data, uint32_t* size);

    /** 消费已由宿主复制或发送的密文
    @param [in] handle 有效连接
    @param [in] size 不超过peekOutput长度
    @return TLS状态或错误
    */
    static int32_t connectionConsumeOutput(NetInterfaceConnectionHandle handle, uint32_t size);

    /** 标记TCP结束，后续progress检查TLS关闭及帧是否完整
    @param [in] handle 有效连接
    @return TLS状态或错误
    */
    static int32_t connectionFinishInput(NetInterfaceConnectionHandle handle);

    /** 查询持续状态与诊断
    @param [in] handle 有效连接
    @param [out] detail 可空，TLS诊断
    @return TLS状态或错误
    */
    static int32_t connectionGetStatus(NetInterfaceConnectionHandle handle, int32_t* detail);

    /** 共用SHA256实现，输出由调用者拥有
    @param [in] input 借用字节，仅size0可空
    @param [in] size 最多16MiB
    @param [out] output 至少32字节，不与输入重叠
    @param [in] capacity 输出容量
    @return NI_OK或负数，失败输出不修改
    */
    static int32_t sha256(const uint8_t* input, uint32_t size, uint8_t* output, uint32_t capacity);

    /** 共用PBKDF2-HMAC-SHA256；只有服务端工作执行者用于密码校验，不在GUI/网络回调执行
    @param [in] password 借用密码
    @param [in] passwordSize 1至1024字节
    @param [in] salt 借用盐
    @param [in] saltSize 1至128字节
    @param [in] iterations 1至2000000，生产新账号由Account固定600000
    @param [out] output 至少32字节
    @param [in] capacity 输出容量
    @return NI_OK或负数，失败输出不修改
    */
    static int32_t pbkdf2Sha256(const uint8_t* password, uint32_t passwordSize, const uint8_t* salt, uint32_t saltSize,
                                                     uint32_t iterations, uint8_t* output, uint32_t capacity);

    /** 在所属模块中可靠擦除调用者明确拥有的秘密缓冲
    @param [in,out] bytes 可写字节，size0可空
    @param [in] size 字节数
    */
    static void wipe(uint8_t* bytes, uint32_t size);

    /** 借用宿主密码学随机，捕获回调失败并擦除有效输出，不提供系统或弱随机回退
    @param [in] entropy size非零时不可空，必须由宿主提供强随机
    @param [in] context 借用至回调返回
    @param [out] output size非零时不可空，失败擦除有效范围
    @param [in] size 0至8192，非法范围不访问输出
    @return OK、INVALID_ARGUMENT或INVALID_STATE
    */
    static int32_t entropyBytes(NetInterfaceEntropyFunction entropy, void* context, uint8_t* output, uint32_t size);

    /** 创建空protobuf消息
    @param [out] handle 不可空；失败清空
    @return NI_OK或负数错误
    */
    static int32_t messageCreate(NetInterfaceMessageHandle* handle);

    /** 握手共同确认位8后启用有界解压；仅白名单类型会自动尝试压缩，默认全部关闭。 */
    static int32_t connectionConfigureCompression(NetInterfaceConnectionHandle handle,
        const uint32_t* types, uint32_t count, int32_t negotiated);

    /** 销毁消息
    @param [in] handle 有效实例或null，禁止重复释放
    */
    static void messageDestroy(NetInterfaceMessageHandle handle);

    /** 清空消息及累计修改预算
    @param [in] handle 有效实例
    @return NI_OK或NI_INVALID_ARGUMENT
    */
    static int32_t messageClear(NetInterfaceMessageHandle handle);

    /** 有界解析，所有失败均先清除旧字段，禁止沿用旧账号等内容
    @param [in] handle 有效实例
    @param [in] input 完整protobuf，size为0时可空
    @param [in] size 最大1MiB；容器/值节点最大65536
    @return NI_OK或负数错误
    */
    static int32_t messageParse(NetInterfaceMessageHandle handle, const uint8_t* input, uint32_t size);

    /** 序列化同源protobuf消息，不在C++与宿主间转移内存所有权
    @param [in] handle 有效实例
    @param [out] output 输出，capacity为0时可空
    @param [in] capacity 容量
    @param [out] required 不可空，所需字节数；不足空间时缓冲保持不变
    @return NI_OK或负数错误，最大1MiB
    */
    static int32_t messageEncode(NetInterfaceMessageHandle handle, uint8_t* output, uint32_t capacity,
                                                      uint32_t* required);

    /** 设置map字段，保留既有Variant编号及二进制格式，字符串可以含NUL
    @param [in] handle 有效实例，单消息累计修改成本最多1MiB，clear后恢复预算
    @param [in] group 容器键
    @param [in] key 字段键
    @param [in] type INIT0/INT32=1/UINT64=2/DOUBLE=3/STRING=4/BLOB=5
    @param [in] value 字节，size为0时可空；数字为现有小端4/8字节
    @param [in] size 字节数，固定宽度会校验
    @return NI_OK或负数错误；非法输入不改旧值
    */
    static int32_t messageSetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t type,
                                                        const uint8_t* value, uint32_t size);

    /** 查询单个map字段
    @param [in] handle 有效实例
    @param [in] group 容器键
    @param [in] key 字段键
    @param [out] type 不可空，Variant类型
    @param [out] output 字节缓冲，capacity为0时可空
    @param [in] capacity 字节容量
    @param [out] required 不可空，实际字节数；不足空间不写缓冲
    @return NI_OK、NI_NOT_FOUND或负数错误
    */
    static int32_t messageGetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t* type,
                                                        uint8_t* output, uint32_t capacity, uint32_t* required);

    /** 设置状态码，序列化不会清除它
    @param [in] handle 有效实例
    @param [in] code 业务码
    @param [in] error UTF8错误文本，size为0时可空
    @param [in] size 最多4096字节
    @return NI_OK或负数错误
    */
    static int32_t messageSetCode(NetInterfaceMessageHandle handle, int32_t code, const uint8_t* error, uint32_t size);

    /** 查询状态码和错误文本
    @param [in] handle 有效实例
    @param [out] code 不可空
    @param [out] error 调用方缓冲，capacity为0时可空
    @param [in] capacity 缓冲容量
    @param [out] required 不可空，文本字节数，不添加NUL
    @return NI_OK或负数错误；不足空间时code及required仍有效
    */
    static int32_t messageGetCode(NetInterfaceMessageHandle handle, int32_t* code, uint8_t* error, uint32_t capacity,
                                                       uint32_t* required);

    /** 创建由库拥有的解码器
    @param [in] payloadLimit 最大消息体字节数，1至16MiB
    @param [out] handle 创建的实例，失败时清空
    @return OK、INVALID_ARGUMENT或NO_MEMORY
    */
    static int32_t create(uint32_t payloadLimit, NetInterfaceHandle* handle);

    /** 在分配模块内销毁实例
    @param [in] handle 有效实例或null，销毁后禁止再使用
    */
    static void destroy(NetInterfaceHandle handle);

    /** 输入流片段，停在第一条完整消息
    @param [in] handle 有效实例
    @param [in] data 本次调用借用字节，仅size为0时可空
    @param [in] size 输入字节数
    @param [out] consumed 已消费字节数，即使输入为空也必须提供
    @return NEED_MORE、READY或负数错误
    */
    static int32_t feed(NetInterfaceHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed);

    /** 借用完整消息而不消费
    @param [in] handle 有效实例
    @param [out] frame 无完整消息时清空
    @return READY或当前状态，空参数返回INVALID_ARGUMENT
    */
    static int32_t peek(NetInterfaceHandle handle, NetInterfaceFrame* frame);

    /** 消费完整消息，使所有借用视图失效
    @param [in] handle 有效实例
    @return OK、INVALID_STATE或持续错误
    */
    static int32_t consume(NetInterfaceHandle handle);

    /** 检查输入结束时是否截断
    @param [in] handle 有效实例
    @return OK、READY或负数错误
    */
    static int32_t finish(NetInterfaceHandle handle);

    /** 为新连接清空状态，使旧视图失效
    @param [in] handle 有效实例
    @return OK，空实例返回INVALID_ARGUMENT
    */
    static int32_t reset(NetInterfaceHandle handle);

    /** 编码至调用者缓冲
    @param [in] frame 待编码消息
    @param [out] output 与消息体不重叠，capacity为0时可空
    @param [in] capacity 可写字节数
    @param [out] required 所需字节数，参数不合法时为0
    @return OK、BUFFER_TOO_SMALL（输出不变）或负数错误
    */
    static int32_t encode(const NetInterfaceFrame* frame, uint8_t* output, uint32_t capacity, uint32_t* required);

    /** 使用固定uint32字段读取消息，避免跨语言依赖C结构体布局
    @param [in] handle 有效解码器
    @param [out] fields 不可空，6项依次为kind、product、request低位、request高位、type、payloadSize
    @param [out] payload 不可空，消息体借用至consume/reset/destroy
    @return READY或当前状态，非READY时输出清空
    */
    static int32_t peekFields(NetInterfaceHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 用固定字段编码帧，64位编号以两个uint32传递
    @param [in] fields 不可空，6项顺序与NetInterface::peekFields相同
    @param [in] payload 消息体，长度为0时可空
    @param [out] output 与消息体不重叠，capacity为0时可空
    @param [in] capacity 输出容量
    @param [out] required 不可空，所需字节数
    @return OK、BUFFER_TOO_SMALL或负数错误
    */
    static int32_t encodeFields(const uint32_t* fields, const uint8_t* payload, uint8_t* output, uint32_t capacity,
                                                     uint32_t* required);

    /** 创建客户端TLS，无需跨语言构建C结构体
    @param [in] authority 受信根PEM或DER，仅调用时借用
    @param [in] authoritySize 字节数，PEM含NUL，最大128KiB
    @param [in] hostname 必须验证的域名，1至253字节，仅调用时借用
    @param [in] entropy 操作系统密码学随机函数，借用至destroy
    @param [in] context 随机上下文，可空，借用至destroy
    @param [in] timeoutMs 握手时限，100至60000毫秒
    @param [out] handle 不可空，成功的TLS实例，失败清空
    @param [out] detail 可空，底层错误
    @return TLS_HANDSHAKE或负数错误
    */
    static int32_t tlsCreateClient(const uint8_t* authority, uint32_t authoritySize, const char* hostname,
                                                        NetInterfaceEntropyFunction entropy, void* context, uint32_t timeoutMs,
                                                        NetInterfaceTlsHandle* handle, int32_t* detail);

    /** 创建非阻塞TLS通道，验证配置并复制证书
    @param [in] options 证书调用时借用，随机回调上下文借用至destroy
    @param [out] handle 成功创建的实例；失败时清空
    @param [out] detail 可空，接收底层Mbed TLS错误
    @return TLS_HANDSHAKE或负数错误
    */
    static int32_t tlsCreate(const NetInterfaceTlsOptions* options, NetInterfaceTlsHandle* handle, int32_t* detail);

    /** 销毁TLS实例，释放并擦除私密数据
    @param [in] handle 有效实例或null，销毁后禁止再用
    */
    static void tlsDestroy(NetInterfaceTlsHandle handle);

    /** 输入网络密文，保留未消费后缀
    @param [in] handle 有效TLS实例
    @param [in] data size非0时不可空，本次调用复制
    @param [in] size 字节数
    @param [out] consumed 实际复制字节数，不可空
    @return TLS_OK、TLS_WANT_IO或负数错误
    */
    static int32_t tlsFeed(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed);

    /** 推进握手与待发明文；时间拆分避免JavaScript整数精度及i64 ABI差异
    @param [in] handle 有效TLS实例
    @param [in] timeLow 单调毫秒的低32位
    @param [in] timeHigh 单调毫秒的高32位
    @return TLS_HANDSHAKE、TLS_READY、TLS_CLOSED或负数错误
    */
    static int32_t tlsProgress(NetInterfaceTlsHandle handle, uint32_t timeLow, uint32_t timeHigh);

    /** 借用待发密文前缀，可在错误时排空TLS警报
    @param [in] handle 有效TLS实例
    @param [out] data 不可空，消费或销毁之前有效
    @param [out] size 不可空，连续字节数
    @return TLS_OK或负数错误
    */
    static int32_t tlsPeekOutput(NetInterfaceTlsHandle handle, const uint8_t** data, uint32_t* size);

    /** 消费已复制到宿主或已发送的密文
    @param [in] handle 有效TLS实例
    @param [in] size 不超过peek连续长度
    @return TLS_OK或负数错误
    */
    static int32_t tlsConsumeOutput(NetInterfaceTlsHandle handle, uint32_t size);

    /** 复制一条记录的待发明文；宿主随后调用progress
    @param [in] handle 有效TLS实例
    @param [in] data size非0时不可空，返回后不再借用
    @param [in] size 字节数，最多接受16384字节
    @param [out] accepted 不可空，已复制的前缀长度，不表示送达
    @return TLS_OK、TLS_WANT_IO或负数错误
    */
    static int32_t tlsWrite(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* accepted);

    /** 读取经过完整性验证的应用明文
    @param [in] handle 有效TLS实例
    @param [out] output capacity非0时不可空
    @param [in] capacity 可写字节数
    @param [out] received 不可空，实际输出字节数
    @return TLS_OK、TLS_WANT_IO、TLS_CLOSED或负数错误
    */
    static int32_t tlsRead(NetInterfaceTlsHandle handle, uint8_t* output, uint32_t capacity, uint32_t* received);

    /** 标记TCP已结束，后续read检查认证关闭和截断
    @param [in] handle 有效TLS实例
    @return TLS_OK或负数错误
    */
    static int32_t tlsFinishInput(NetInterfaceTlsHandle handle);

    /** 请求认证关闭，需继续排空密文输出
    @param [in] handle 有效TLS实例
    @return TLS_OK、TLS_WANT_IO或负数错误
    */
    static int32_t tlsClose(NetInterfaceTlsHandle handle);

    /** 查询TLS当前状态
    @param [in] handle 有效TLS实例
    @param [out] detail 可空，底层错误
    @return NetInterfaceTlsStatus
    */
    static int32_t tlsGetStatus(NetInterfaceTlsHandle handle, int32_t* detail);

    /** 在NI内部生成P256私钥和CSR，只复制固定字节，不转交STL分配或释放权
    @param [in] name ASCII逻辑身份，不必带NUL，不能自授服务注册权限
    @param [in] nameSize 1至96字节
    @param [in] entropy 宿主强随机，经受控入口调用，异常或失败不回退
    @param [in] context 熵上下文，借用至返回
    @param [out] privateKey 调用者缓冲，仅全部成功时写入；输出/输入/计数不得重叠
    @param [in] keyCapacity 1至4096字节，通常直接提供4096
    @param [out] keySize 含NUL的PEM字节数；空间不足为本次所需值，其他失败0；重叠拒绝时保留原字节
    @param [out] request 调用者CSR缓冲，仅全部成功时写入，不包含私钥
    @param [in] requestCapacity 1至4096字节，通常直接提供4096
    @param [out] requestSize 含NUL字节数；空间不足报告本次所需值，重试会生成不同身份
    @return OK、BUFFER_TOO_SMALL、INVALID_ARGUMENT、NO_MEMORY或PROTOCOL_ERROR；失败不输出部分密钥
    */
    static int32_t certificateCreateRequest(const uint8_t* name, uint32_t nameSize,
        NetInterfaceEntropyFunction entropy, void* context, uint8_t* privateKey, uint32_t keyCapacity,
        uint32_t* keySize, uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize);

    /** 创建固定64KiB双向窗口，失败清空输出
    @param [out] handle 不可空
    @return NI_OK或错误
    */
    static int32_t creditCreate(NetInterfaceCreditHandle* handle);

    /** 在本模块释放窗口
    @param [in] handle 有效句柄或null
    */
    static void creditDestroy(NetInterfaceCreditHandle handle);

    /** 在本窗口首次使用前配置已协商容量，失败不修改原值
    @param [in] handle 本模块创建的有效窗口句柄，不可空
    @param [in] bytes 65536至1048576字节，必须为16384的整数倍
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE
    */
    static int32_t creditConfigure(NetInterfaceCreditHandle handle, uint32_t bytes);

    /** 从已认证的管理OPEN回应读取窗口字段并应用，缺字段采用64KiB
    @param [in] handle 尚未使用的有效窗口句柄，不可空
    @param [in] input 仅在调用中借用的ProtoMessage字节，不可空；本函数不负责认证来源
    @param [in] size 输入1至16384字节
    @return NI_OK；参数、协议、窗口状态或内存不足返回对应负数错误
    */
    static int32_t creditConfigureMessage(NetInterfaceCreditHandle handle, const uint8_t* input, uint32_t size);

    /** 查询可发送和待归还字节数，持续错误时两项为0
    @param [in] handle 本模块创建的有效窗口句柄，不可空
    @param [out] fields 不可空，至少2个uint32，依次为可发送和待归还字节数
    @return NI_OK或持续错误；参数非法时返回NI_INVALID_ARGUMENT且不写输出
    */
    static int32_t creditGet(NetInterfaceCreditHandle handle, uint32_t* fields);

    /** 对应C++sent；LIMIT_EXCEEDED不扣额度
    @param [in] handle 窗口
    @param [in] size 已由下层拥有的1至16384字节密文
    @return NI_OK或错误
    */
    static int32_t creditSent(NetInterfaceCreditHandle handle, uint32_t size);

    /** 对应C++grant，拒绝凭空或重复扩大窗口
    @param [in] handle 窗口
    @param [in] size 对端实际归还字节数
    @return NI_OK或持续错误
    */
    static int32_t creditGrant(NetInterfaceCreditHandle handle, uint32_t size);

    /** 对应C++received，只在拥有完整密文记录后计入
    @param [in] handle 窗口
    @param [in] size 完整记录密文字节
    @return NI_OK或持续错误
    */
    static int32_t creditReceived(NetInterfaceCreditHandle handle, uint32_t size);

    /** 管理层拥有归还消息后消费，不能在背压时调用
    @param [in] handle 窗口
    @param [in] size 已交付管理层的额度
    @return NI_OK或错误
    */
    static int32_t creditConsume(NetInterfaceCreditHandle handle, uint32_t size);

    /** 从调用者值记录创建本模块拥有的候选句柄，统一校验及编码仍由NI负责。
    @param [in] nodes 借用至返回；每项structSize/version及有界NUL字符串必须正确
    @param [in] count 1至64，完整列表不得有重复节点名或地址端口
    @param [out] handle 不可空且不得与输入数组重叠；一般失败置null，重叠拒绝且不写输入；成功由本模块Destroy释放
    @return NI_OK、INVALID_ARGUMENT或NO_MEMORY，不授予身份
    */
    static int32_t directoryCreateNodes(const NetInterfaceDirectoryNodeData* nodes, uint32_t count,
        NetInterfaceDirectoryHandle* handle);

    /** 解析完整发现表，创建只读候选句柄；失败清空，不授予身份
    @param [in] data 非空protobuf
    @param [in] size 最大16384字节
    @param [out] handle 不可空且不得与data重叠；重叠拒绝且不修改输入
    @return NI_OK或错误
    */
    static int32_t directoryCreate(const uint8_t* data, uint32_t size, NetInterfaceDirectoryHandle* handle);

    /** 校验NGC3缓存并创建句柄，UTC拆为低/高32位避免JS精度损失
    @param [in] data 非空缓存
    @param [in] size 最大16432字节
    @param [in] utcLow 当前UTC毫秒低32位
    @param [in] utcHigh 当前UTC毫秒高32位
    @param [out] handle 不得与data重叠；一般失败清空，重叠拒绝且不修改输入
    @return NI_OK、NI_TIMEOUT或错误
    */
    static int32_t directoryCreateCache(const uint8_t* data, uint32_t size, uint32_t utcLow,
        uint32_t utcHigh, NetInterfaceDirectoryHandle* handle);

    /** 在创建模块销毁
    @param [in] handle 有效句柄或null
    */
    static void directoryDestroy(NetInterfaceDirectoryHandle handle);

    /** 查询已验证的完整候选数量
    @param [in] handle 有效句柄
    @param [out] count 成功1至64，失败0
    @return NI_OK或错误
    */
    static int32_t directoryCount(NetInterfaceDirectoryHandle handle, uint32_t* count);

    /** 向调用者已有的v1值数组复制完整快照，不转交STL或释放权。
    @param [in] handle 本模块创建的有效句柄
    @param [out] nodes 调用者数组，每项预填structSize/version；容量不足不改数组
    @param [in] capacity 0至64项，nodes为空且capacity为0可查询
    @param [out] count 必需项数；参数错误为0；不得与nodes重叠，重叠拒绝且保留原字节
    @return OK、BUFFER_TOO_SMALL或INVALID_ARGUMENT
    */
    static int32_t directoryCopyNodes(NetInterfaceDirectoryHandle handle,
        NetInterfaceDirectoryNodeData* nodes, uint32_t capacity, uint32_t* count);

    /** 复制一个公开节点，不暴露C++布局；输出缓冲不得重叠
    @param [in] handle 有效句柄
    @param [in] index 从0开始
    @param [out] name 节点ASCII名，包含NUL需38字节
    @param [in] nameCapacity 节点名缓冲容量
    @param [out] address 数值地址，包含NUL最多40字节
    @param [in] addressCapacity 地址缓冲容量
    @param [out] fields 四项uint32：端口、容量、boot低32、高32；一般失败清零，输出重叠时全部保持原值
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误；失败不修改字符串
    */
    static int32_t directoryNode(NetInterfaceDirectoryHandle handle, uint32_t index,
        char* name, uint32_t nameCapacity, char* address, uint32_t addressCapacity, uint32_t* fields);

    /** 导出只含公开候选的NGC3缓存，不重新实现JS协议
    @param [in] handle 有效句柄
    @param [in] utcLow 当前UTC毫秒低32位
    @param [in] utcHigh 当前UTC毫秒高32位
    @param [out] output 输出，允许空以查询required
    @param [in] capacity 输出容量
    @param [out] required 所需字节数，不得与output重叠；一般参数失败为0，重叠拒绝且保持原值
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误；空间不足不修改输出
    */
    static int32_t directoryCache(NetInterfaceDirectoryHandle handle, uint32_t utcLow, uint32_t utcHigh,
        uint8_t* output, uint32_t capacity, uint32_t* required);

    /** 同源构造文件控制请求，Wasm不手写二进制头
    @param [in] fields 5项：command/chunk/window/total低高
    @param [in] transfer 16字节非零幂等号或已有文件号
    @param [in] token 32字节；OPEN全零，QUERY/CANCEL服务秘密
    @param [in] hash 32字节；仅上传OPEN使用
    @param [in] key 有界不透明逻辑键，OPEN必需
    @param [in] keySize 0至512
    @param [in] authorization 不透明业务授权，可空
    @param [in] authorizationSize 0至2048
    @param [out] output 可空探测
    @param [in] capacity 容量
    @param [out] required 不可空
    @return NetInterfaceResult
    */
    static int32_t fileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token,
                                                          const uint8_t* hash, const uint8_t* key, uint32_t keySize,
                                                          const uint8_t* authorization, uint32_t authorizationSize, uint8_t* output,
                                                          uint32_t capacity, uint32_t* required);

    /** 解析文件成功响应
    @param [in] bytes 160字节状态体
    @param [in] size 精确长度
    @param [out] info 不可空，失败清零
    @return OK或负数
    */
    static int32_t fileParseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo* info);

    /** Wasm读取元数据，64位字段使用低/高32位
    @param [in] bytes 160字节状态体
    @param [in] size 长度
    @param [out] fields 12项：direction/state/chunk/window/total低高/offset低高/stream低高/expires低高
    @param [out] transfer 16字节
    @param [out] token 32字节秘密
    @param [out] hash 32字节整文件摘要
    @return OK或负数，失败输出全部清零
    */
    static int32_t fileInfoFields(const uint8_t* bytes, uint32_t size, uint32_t* fields, uint8_t* transfer,
                                                       uint8_t* token, uint8_t* hash);

    /** 创建独立纯协议流，不创建socket/文件/线程
    @param [in] info 已授权并映射的元数据
    @param [in] product 非零产品
    @param [in] sender 1发送DATA，0接收DATA
    @param [in] timeLow 单调毫秒低位
    @param [in] timeHigh 单调毫秒高位
    @param [in] timeoutMs 无进度时限1000至60000
    @param [out] handle 不可空，失败清零
    @return OK或负数
    */
    static int32_t fileFlowCreate(const NetInterfaceFileInfo* info, uint32_t product, uint32_t sender,
                                                       uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
                                                       NetInterfaceFileFlowHandle* handle);

    /** Wasm直接从状态体创建，宿主不维护C结构布局
    @param [in] bytes 文件状态体
    @param [in] size 固定160字节
    @param [in] product 产品
    @param [in] sender 1发端，0收端
    @param [in] timeLow 单调时间低位
    @param [in] timeHigh 高位
    @param [in] timeoutMs 无进度时限
    @param [out] handle 不可空，失败清零
    @return OK或负数
    */
    static int32_t fileFlowCreateWire(const uint8_t* bytes, uint32_t size, uint32_t product, uint32_t sender,
                                                           uint32_t timeLow, uint32_t timeHigh, uint32_t timeoutMs,
                                                           NetInterfaceFileFlowHandle* handle);

    /** 释放并擦除协议流，不操作宿主文件
    @param [in] handle 句柄或null
    */
    static void fileFlowDestroy(NetInterfaceFileFlowHandle handle);

    /** 可选接收端有界合并写，必须在第一块DATA之前设置，默认1
    @param [in] handle 收端流
    @param [in] chunks 不大于协商窗口，最多8；宿主支持最大chunks乘chunk字节单次写入
    @return 同Flow.setWriteBatchChunks；只减少宿主调用，不提前ACK，不改变线上协议
    */
    static int32_t fileFlowSetWriteBatchChunks(NetInterfaceFileFlowHandle handle, uint32_t chunks);

    /** 按顺序补恢复前缀
    @param [in] handle 流
    @param [in] offsetLow 前缀位置低位
    @param [in] offsetHigh 高位
    @param [in] bytes 文件前缀
    @param [in] size 不超过剩余量且最多16MiB
    @return 同Flow.prime
    */
    static int32_t fileFlowPrime(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                      const uint8_t* bytes, uint32_t size);

    /** 驱动时限
    @param [in] handle 流
    @param [in] timeLow 单调时间低位
    @param [in] timeHigh 高位
    @return 同Flow.progress
    */
    static int32_t fileFlowProgress(NetInterfaceFileFlowHandle handle, uint32_t timeLow, uint32_t timeHigh);

    /** 申请宿主读取
    @param [in] handle 流
    @param [out] fields 三项offset低/高位和精确字节数
    @return READY读取参数，否则输出0
    */
    static int32_t fileFlowRead(NetInterfaceFileFlowHandle handle, uint32_t* fields);

    /** 接纳异步读取结果并复制
    @param [in] handle 流
    @param [in] offsetLow 工作偏移低位
    @param [in] offsetHigh 高位
    @param [in] bytes 完整块
    @param [in] size 精确长度
    @return 同Flow.produced
    */
    static int32_t fileFlowProduced(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                         const uint8_t* bytes, uint32_t size);

    /** 接纳DATA/ACK帧
    @param [in] handle 流
    @param [in] fields 六项kind/product/id低高/type/size
    @param [in] payload fields[5]非零时不可空
    @return 同Flow.accept
    */
    static int32_t fileFlowAccept(NetInterfaceFileFlowHandle handle, const uint32_t* fields, const uint8_t* payload);

    /** 取一次持久写请求，宿主复制借用bytes后投递文件执行者
    @param [in] handle 流
    @param [out] fields 三项offset低/高位和长度
    @param [out] bytes 借用至written/cancel/destroy
    @return 同Flow.writeRequest，失败输出0/null
    */
    static int32_t fileFlowWrite(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** bytes);

    /** 回报对应实际持久写结果
    @param [in] handle 流
    @param [in] offsetLow 工作偏移低位
    @param [in] offsetHigh 高位
    @param [in] size 精确长度
    @param [in] result NI_OK持久化或负数
    @return 同Flow.written
    */
    static int32_t fileFlowWritten(NetInterfaceFileFlowHandle handle, uint32_t offsetLow, uint32_t offsetHigh,
                                                        uint32_t size, int32_t result);

    /** 摘要通过后取一次提交任务
    @param [in] handle 流
    @return 同Flow.takeCommit
    */
    static int32_t fileFlowTakeCommit(NetInterfaceFileFlowHandle handle);

    /** 实际文件提交结果，禁止入队时就报成功
    @param [in] handle 流
    @param [in] result NI_OK已提交，负数失败
    @return 同Flow.committed
    */
    static int32_t fileFlowCommitted(NetInterfaceFileFlowHandle handle, int32_t result);

    /** 借用输出；背压时不消费
    @param [in] handle 流
    @param [out] fields 六项kind/product/id低高/type/size
    @param [out] payload 借用至consume/cancel/destroy
    @return READY或其他结果，失败清输出
    */
    static int32_t fileFlowPeek(NetInterfaceFileFlowHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 下层完整接纳后消费
    @param [in] handle 流
    @return 同Flow.consumeOutput
    */
    static int32_t fileFlowConsume(NetInterfaceFileFlowHandle handle);

    /** 取消本地流，宿主另外发文件CANCEL RPC并收敛文件工作
    @param [in] handle 流
    @param [in] reason 负数错误
    @return OK或INVALID_ARGUMENT
    */
    static int32_t fileFlowCancel(NetInterfaceFileFlowHandle handle, int32_t reason);

    /** 当前显示快照
    @param [in] handle 流
    @param [out] info 固定C布局，失败清零
    @param [out] result 不可空
    @return OK或INVALID_ARGUMENT
    */
    static int32_t fileFlowSnapshot(NetInterfaceFileFlowHandle handle, NetInterfaceFileInfo* info, int32_t* result);

    /** Wasm从相同160字节状态读取进度，不依赖C结构体布局
    @param [in] handle 流
    @param [out] bytes 状态体，可空探测
    @param [in] capacity 至少160
    @param [out] result 流结果，不可空
    @return 编码结果；秘密只供恢复，不向展示/日志输出
    */
    static int32_t fileFlowSnapshotWire(NetInterfaceFileFlowHandle handle, uint8_t* bytes, uint32_t capacity,
                                                             int32_t* result);

    /** 创建固定字段握手，供上层适配器使用有界数组传参
    @param [in] options 六项：server/platform/capabilities/maxRequests/maxChunk/timeoutMs
    @param [in] optionCount 必须为6
    @param [in] identities 固定32字节，本连接随机数16字节和设备标识16字节
    @param [in] identitySize 必须为32
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @param [out] handle 必须非空，失败清空
    @return NEED_MORE或负数错误
    */
    static int32_t handshakeCreate(const uint32_t* options, uint32_t optionCount, const uint8_t* identities,
                                                        uint32_t identitySize, uint32_t timeLow, uint32_t timeHigh,
                                                        NetInterfaceHandshakeHandle* handle);

    /** 释放本实例
    @param [in] handle 实例或null；非null不得重复释放
    */
    static void handshakeDestroy(NetInterfaceHandshakeHandle handle);

    /** 检查固定时限和时钟回退
    @param [in] handle 有效实例
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return READY、NEED_MORE或负数错误
    */
    static int32_t handshakeProgress(NetInterfaceHandshakeHandle handle, uint32_t timeLow, uint32_t timeHigh);

    /** 接收经拆包的完整帧
    @param [in] handle 有效实例
    @param [in] fields 六项：kind/product/requestLow/requestHigh/type/payloadSize
    @param [in] payload 借用消息体，长度由fields指定
    @param [in] timeLow 单调毫秒低32位
    @param [in] timeHigh 单调毫秒高32位
    @return 握手READY/NEED_MORE、普通帧OK或负数错误
    */
    static int32_t handshakeAccept(NetInterfaceHandshakeHandle handle, const uint32_t* fields, const uint8_t* payload,
                                                        uint32_t timeLow, uint32_t timeHigh);

    /** 查看待发送握手，背压时不消费
    @param [in] handle 有效实例
    @param [out] fields 六项固定帧字段，未就绪时清空
    @param [out] payload 借用到consume/destroy，未就绪时清空
    @return READY有输出、NEED_MORE无输出或负数错误
    */
    static int32_t handshakePeekOutput(NetInterfaceHandshakeHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 下层接纳后消费待发帧
    @param [in] handle 有效实例
    @return READY/NEED_MORE或负数错误
    */
    static int32_t handshakeConsumeOutput(NetInterfaceHandshakeHandle handle);

    /** 读取已协商信息；失败清空有效的输出地址
    @param [in] handle 有效实例
    @param [out] fields 平台/能力/在途上限/块大小四项
    @param [out] identities 对端随机数16字节及客户端设备标识16字节
    @return READY、NEED_MORE或负数错误
    */
    static int32_t handshakePeerInfo(NetInterfaceHandshakeHandle handle, uint32_t* fields, uint8_t* identities);

    /** 创建管理连接；调用方显式提供身份、运输及熵源，不提供JS工厂
    @param [in] options v3配置，不可空
    @param [out] handle 不可空，失败清零
    @return NI_NEED_MORE或错误，管理认证完成后才READY
    */
    static int32_t linkCreate(const NetInterfaceLinkOptions* options, NetInterfaceLinkHandle* handle);

    /** 释放独占管理连接
    @param [in] handle 有效实例或null，不得重复释放
    */
    static void linkDestroy(NetInterfaceLinkHandle handle);

    /** 接纳布局无关的完整外层记录
    @param [in] handle 实例
    @param [in] fields 五项kind/product/channelLow/channelHigh/size
    @param [in] payload fields[4]非0时不可空，调用后不借用
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
    static int32_t linkAccept(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

    /** 推进认证/心跳
    @param [in] handle 实例
    @param [in] low 单调毫秒低32位
    @param [in] high 单调毫秒高32位
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t linkProgress(NetInterfaceLinkHandle handle, uint32_t low, uint32_t high);

    /** 借用下一待发记录；下层拥有之后才consume
    @param [in] handle 实例
    @param [out] fields 五项路由字段，不可空
    @param [out] payload 不可空，借用至consume/destroy
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t linkPeekOutput(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 消费已送出记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
    static int32_t linkConsumeOutput(NetInterfaceLinkHandle handle);

    /** 排队布局无关的业务密文，不在管理层解密
    @param [in] handle 已就绪实例
    @param [in] fields 五项路由字段
    @param [in] payload 调用后不借用
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
    static int32_t linkSendRecord(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

    /** 借用收到的业务密文
    @param [in] handle 实例
    @param [out] fields 五项字段
    @param [out] payload 借用至consumeRecord
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t linkPeekRecord(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 消费收到的业务记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
    static int32_t linkConsumeRecord(NetInterfaceLinkHandle handle);

    /** 排队认证管理消息
    @param [in] handle 实例
    @param [in] fields 六项kind/product/requestLow/requestHigh/type/size
    @param [in] payload 借用至返回
    @return NI_OK、NI_LIMIT_EXCEEDED或错误
    */
    static int32_t linkSendControl(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload);

    /** 借用完整管理消息
    @param [in] handle 实例
    @param [out] fields 六项帧字段
    @param [out] payload 借用至consumeControl
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t linkPeekControl(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 消费管理消息
    @param [in] handle 实例
    @return NI_OK或错误
    */
    static int32_t linkConsumeControl(NetInterfaceLinkHandle handle);

    /** 借用已验证管理身份，证书与声明不包含私钥
    @param [in] handle 实例
    @param [out] fields 四项role/nameSize/certificateSize/claimsSize
    @param [out] name 字符串字节借用至destroy
    @param [out] certificate PEM字节，不含末尾NUL，借用至destroy
    @param [out] claims 声明字节，借用至destroy
    @return NI_READY或状态，非READY清空有效输出
    */
    static int32_t linkPeerInfo(NetInterfaceLinkHandle handle, uint32_t* fields,
    const uint8_t** name, const uint8_t** certificate, const uint8_t** claims);

    /** 固定终止本次底层连接，不能把旧密文拼到新会话
    @param [in] handle 实例
    @return 负数终态
    */
    static int32_t linkFinishInput(NetInterfaceLinkHandle handle);

    /** 查询管理认证状态
    @param [in] handle 实例
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t linkStatus(NetInterfaceLinkHandle handle);

    /** 创建等待表
    @param [in] capacity 1至4096
    @param [out] handle 不可空，失败清空
    @return OK或负数错误
    */
    static int32_t requestsCreate(uint32_t capacity, NetInterfaceRequestsHandle* handle);

    /** 释放等待表，不执行宿主回调
    @param [in] handle 实例或null，非null不得重复释放
    */
    static void requestsDestroy(NetInterfaceRequestsHandle handle);

    /** 登记宿主验证过的一代端点
    @param [in] handle 有效实例
    @param [in] low 端点号低32位
    @param [in] high 端点号高32位
    @param [in] service 0外部端点，1本地可信服务
    @param [in] product 专用产品号，外部端点为0
    @param [in] limit 每端点入/出限制1至128
    @param [in] internalOnly 0普通服务，1只允许合法父请求访问
    @return OK或负数错误
    */
    static int32_t requestsAddEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high,
                                                            uint32_t service, uint32_t product, uint32_t limit, uint32_t internalOnly);

    /** 相关请求清理后删除端点
    @param [in] handle 有效实例
    @param [in] low 端点号低32位
    @param [in] high 端点号高32位
    @return OK或负数错误
    */
    static int32_t requestsRemoveEndpoint(NetInterfaceRequestsHandle handle, uint32_t low, uint32_t high);

    /** 登记完整请求，所有64位值用低/高uint32相邻表示
    @param [in] handle 有效实例
    @param [in] input 十三项：source低高、original低高、target低高、product、type、parent低高、now低高、timeoutMs
    @param [in] inputCount 必须13
    @param [in] autoId 0采用input原号；1由C++生成，input原号必须0
    @param [out] output 十六项：route/source/original/target/origin/parent/deadline各低高，product、type；失败清空
    @return OK、CANCELLED或负数错误
    */
    static int32_t requestsBegin(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                      uint32_t autoId, uint32_t* output);

    /** 目标完成请求，校验身份/元数据及当前期限
    @param [in] handle 有效实例
    @param [in] input 网关八项：target低高、route低高、product、type、now低高；客户端十项：source低高、original低高、target低高、product、type、now低高
    @param [in] inputCount 8按路由号完成，10按来源原请求号完成
    @param [out] output 十六项请求记录，READY/TIMEOUT/CANCELLED时填充，其他结果清空
    @return READY、TIMEOUT、CANCELLED或负数错误
    */
    static int32_t requestsComplete(NetInterfaceRequestsHandle handle, const uint32_t* input, uint32_t inputCount,
                                                         uint32_t* output);

    /** 取消来源请求或登记先到的取消控制
    @param [in] handle 有效实例
    @param [in] sourceLow 来源低32位
    @param [in] sourceHigh 来源高32位
    @param [in] originalLow 原请求低32位
    @param [in] originalHigh 原请求高32位
    @param [out] output 十六项请求记录，仅READY时填充
    @return READY、OK或负数错误
    */
    static int32_t requestsCancel(NetInterfaceRequestsHandle handle, uint32_t sourceLow, uint32_t sourceHigh,
                                                       uint32_t originalLow, uint32_t originalHigh, uint32_t* output);

    /** 每次仅取一个需终止的请求，宿主控制本轮次数
    @param [in] handle 有效实例
    @param [in] operation 0到期、1端点关闭、2父请求结束
    @param [in] keyLow 单调时间/端点号/父路由号的低32位
    @param [in] keyHigh 相应高32位
    @param [out] output 十六项请求记录，仅READY时填充
    @return READY、NOT_FOUND或负数错误
    */
    static int32_t requestsTake(NetInterfaceRequestsHandle handle, uint32_t operation, uint32_t keyLow,
                                                     uint32_t keyHigh, uint32_t* output);

    /** 查询在途数
    @param [in] handle 有效实例
    @return 0至4096，空句柄INVALID_ARGUMENT
    */
    static int32_t requestsCount(NetInterfaceRequestsHandle handle);

    /** 固定字段RPC信封编码，不传递C++布局或JS浮点请求号
    @param [in] kind REQUEST或RESPONSE
    @param [in] value 剩余毫秒或传输状态
    @param [in] parentLow 内部父请求号低32位
    @param [in] parentHigh 内部父请求号高32位
    @param [in] data 借用业务字节
    @param [in] size 字节数
    @param [out] output 不重叠输出空间
    @param [in] capacity 输出容量
    @param [out] required 不可空，所需空间
    @return NetInterfaceRpc::encode结果
    */
    static int32_t rpcEncode(uint32_t kind, uint32_t value, uint32_t parentLow, uint32_t parentHigh,
                                                  const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                                                  uint32_t* required);

    /** 解析已拆包的RPC帧；不复制业务体
    @param [in] frameFields kind/product/requestLow/requestHigh/type/payloadSize共6项
    @param [in] count 必须为6
    @param [in] data frameFields声明长度的消息体
    @param [out] fields value/parentLow/parentHigh/bodyOffset/bodySize共5项；失败清零，允许与输入字段重叠
    @return OK或负数错误
    */
    static int32_t rpcParse(const uint32_t* frameFields, uint32_t count, const uint8_t* data, uint32_t* fields);

    /** 共享控制帧验证/回复，避免微信重复协议逻辑
    @param [in] fields kind/product/requestLow/requestHigh/type/payloadSize共6项
    @param [in] count 必须为6
    @param [out] reply 六项；仅READY时为PONG，其余清零，允许同输入
    @return NetInterfaceRpc::control结果或INVALID_ARGUMENT
    */
    static int32_t rpcControl(const uint32_t* fields, uint32_t count, uint32_t* reply);

    /** 创建原生/Wasm共用的外层拆包器
    @param [out] handle 不可空，失败清零
    @return NI_OK或负数错误
    */
    static int32_t tunnelCreate(NetInterfaceTunnelHandle* handle);

    /** 在拥有缓冲的模块释放
    @param [in] handle 有效句柄或null，不得重复释放
    */
    static void tunnelDestroy(NetInterfaceTunnelHandle handle);

    /** 提交外层字节，只消费到一条完整记录
    @param [in] handle 实例
    @param [in] data size非零时不可空
    @param [in] size 字节数
    @param [out] consumed 不可空，实际接纳量
    @return NI_READY、NI_NEED_MORE或错误
    */
    static int32_t tunnelFeed(NetInterfaceTunnelHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed);

    /** 获取布局无关的路由头和借用密文
    @param [in] handle 实例
    @param [out] fields 五项kind/product/channelLow/channelHigh/size，不可空
    @param [out] payload 不可空，借用至consume/destroy
    @return NI_READY或当前状态，非READY输出清零
    */
    static int32_t tunnelPeek(NetInterfaceTunnelHandle handle, uint32_t* fields, const uint8_t** payload);

    /** 消费当前完整记录
    @param [in] handle 实例
    @return NI_OK或错误
    */
    static int32_t tunnelConsume(NetInterfaceTunnelHandle handle);

    /** 验证EOF
    @param [in] handle 实例
    @return NI_OK、NI_READY或截断错误
    */
    static int32_t tunnelFinish(NetInterfaceTunnelHandle handle);

    /** 编码固定外层头，64位编号由两项uint32传递，禁止JS浮点合并
    @param [in] fields 五项kind/product/channelLow/channelHigh/size
    @param [in] payload fields[4]非0时不可空
    @param [out] output 调用者输出，不允许与payload重叠
    @param [in] capacity 输出空间
    @param [out] required 不可空，合法输入所需字节数
    @return NI_OK、NI_BUFFER_TOO_SMALL或错误
    */
    static int32_t tunnelEncode(const uint32_t* fields, const uint8_t* payload,
    uint8_t* output, uint32_t capacity, uint32_t* required);

private:
    /** 完整固定头验证通过后，准备有界正文存储
    @return NEED_MORE或协议/容量/分配错误，不接纳任何正文
    */
    int32_t prepareHeader();

    /** 清除当前帧信息，按调用边界决定是否归还存储，不改变压缩许可
    @param [in] release true释放全部缓冲，false仅解除当前借用
    */
    void clear(bool release);

    /** 释放当前输入并记录第一个持续错误
    @param [in] error 负数协议或资源错误
    @return 原错误值
    */
    int32_t fail(int32_t error);

    /** 从已校验的固定头读取大端整数，不要求地址对齐
    @param [in] bytes 至少width字节的借用输入
    @param [in] width 1至8字节，仅由本类固定字段调用
    @return 对应无符号整数
    */
    static uint64_t readNumber(const uint8_t* bytes, uint32_t width);

    /** 将整数写成固定宽度大端字节，不要求地址对齐
    @param [out] bytes 至少width字节的可写空间
    @param [in] value 对应字段的值
    @param [in] width 1至8字节，仅由本类固定字段调用
    */
    static void writeNumber(uint8_t* bytes, uint64_t value, uint32_t width);

    /** 禁止复制独占缓冲
    @param [in] other 不可复制实例
    */
    NetInterface(const NetInterface& other);

    /** 禁止复制赋值独占缓冲
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetInterface& operator=(const NetInterface& other);

private:
    // 构造时固定的明文及线记录体上限
    uint32_t m_payloadLimit;
    // 下一帧能否接受已协商的RPC压缩扩展
    bool m_compressionAllowed;
    // 当前完整头已经验证的压缩标志，不随许可设置改变
    bool m_compressed;
    // 只有完整32字节收到后才读取内容，无须预先清零
    uint8_t m_header[NET_INTERFACE_HEADER_SIZE];
    // 已接纳固定头字节数，0至32
    uint32_t m_headerSize;
    // 当前线记录体字节数，解压后仍保留原接收边界
    uint32_t m_wireSize;
    // 本实例独占字节存储，READY前不向外暴露
    uint8_t* m_body;
    // 当前分配容量，consume最多保留4KiB
    uint32_t m_capacity;
    // 当前已接纳线记录体字节数，不超过m_wireSize
    uint32_t m_bodySize;
    // 当前已验证元数据，READY时才包含完整正文借用
    NetInterfaceFrame m_frame;
    // 唯一解码状态，负值持续到显式reset
    int32_t m_status;
};