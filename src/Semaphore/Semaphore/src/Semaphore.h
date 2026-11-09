#pragma once
#include "SemaphoreResult.h"
#include "SemaphoreWake.h"
#include "SemaphoreEventPool.h"
#include "SemaphoreMacro.h"
#include <atomic>
#include <string>
#include <stdint.h>
#include <memory>

class SemaphoreProcessMemory;
class SemaphoreDeadline;
class SemaphoreCount;

/** 本进程通知与原生跨进程计数通知。生命周期操作不与等待/通知并发。
    Windows使用内核信号量，Linux/macOS使用sem_open；移动端使用共享原子计数及有界睡眠，无文件锁。
    移动共享计数协议名称限30字节，不含斜杠/NUL；桌面长度由系统报告，手机沙箱须由宿主验证
    Wasm不支持跨进程命名通知，创建无资源、打开和等待返回失败；本地计数操作仍可用。
*/
class SemaphoreAPI Semaphore
{
// 普通分开调用的接口，以及可选的等待记录池管理和资源诊断
public:
    /** 初始化本地原子状态与系统等待后端，系统资源失败或原子类型不支持无锁时抛system_error
    */
    Semaphore();

    /** 外部静止期释放计数/事件等待池和系统资源，清理失败计入cleanupFailures
    */
    ~Semaphore();

    /** 信号量的通知，通知几次wait允许通过几次，wait之前通知一次则wait执行时允许通过，一对一，不允许一对多
    本地积压计数为64位；达到UINT64_MAX时抛出overflow_error，本次不增加计数
    */
    void signal();

    /** 发布一个64位许可，优先交付已登记等待者；普通积存和直接交付不分配记录
    超过2^63-1的极大积存使用池中不可变记录，可能报告分配/容量错误，64位上限不变。
    @return committed=true表示许可已积存或交付；即使提示失败也不可再次发布
    */
    SemaphoreResult trySignal();

    /** 信号量的等待
    */
    void wait();

    /** 信号量的等待
    @param [in] timeout 总等待时间，单位毫秒，0仅检查现有计数
    @return 取得信号返回true，超时返回false；无限等待使用无参数wait()
    */
    bool wait(uint32_t timeout);

    /** 有结果的无限计数等待
    @return 成功消费许可或返回系统错误/宿主不支持
    */
    SemaphoreResult waitResult();

    /** 有结果的有限计数等待，发布/获取可见性由计数消费或直接交付CAS建立
    普通阻塞先复用系统提示；持续未取得许可时进入内部优先交付链，已交付许可不被后来者抢走。
    @param [in] timeout 总预算毫秒，0仅探测
    @return OK且committed=true为取得许可；TIMEOUT未取得；其余为错误
    */
    SemaphoreResult waitResult(uint32_t timeout);

    /** 生命周期owner在生产者及消费者全部停止后清除本地积存通知。
    */
    void reset();

    /** 优先提示最早登记的合格事件等待者；没有等待者时不积存通知
    */
    void event();

    /** 提示扫描开始时已登记的全部事件等待者，后来登记者不取得这次旧广播
    */
    void eventAll();

    /** 发布非积存事件，不改变计数许可，不分配内存
    @param [in] all false提示一个合格登记者，true提示本次发布前登记者
    @return 提交后提示失败保留committed；无合格登记者时committed=false
    */
    SemaphoreResult tryEvent(bool all = false);

    /** 事件的等待
    */
    void eventWait();

    /** 事件的等待
    @param [in] timeout 超时时间，单位毫秒，非正值只探测，不代表无限
    @return 是否得到提示，调用方仍须检查实际条件或代次
    */
    bool eventWait(int32_t timeout);

    /** 有结果的无限事件等待；登记点为等待记录插入登记链的CAS
    @return OK只表示提示，业务方仍须检查条件/代次
    */
    SemaphoreResult eventWaitResult();

    /** 有结果的有限事件等待
    @param [in] timeout 总预算毫秒，非正值立即返回TIMEOUT，不表示无限
    @return OK提示、TIMEOUT或系统错误/宿主不支持
    */
    SemaphoreResult eventWaitResult(int32_t timeout);

    /** 可选地配置eventWait/eventWaitResult使用的事件等待记录池，失败保留原池
    不调用时按默认策略按需建立池；仅需限制容量或自定义扩缩容策略时配置
    与组合计数接口无依赖，也不影响普通signal/wait及跨进程通知；只允许外部静止期调用
    @param [in] config 初始/阈值/最大容量及策略，不影响计数wait
    @return 配置结果；不能与任何等待/通知并发
    */
    SemaphoreResult configureEventPool(const SemaphorePoolConfig& config);

    /** 可选地维护事件等待记录池，在工作预算内尝试安全回收
    普通事件等待可直接使用，无需配套调用；本函数不是组合接口的准备步骤
    默认不自动缩容；配置缩容策略后，owner可主动调用以在空闲期触发维护，无后台线程
    @param [out] released 本次实际释放记录数
    @return OK或策略/容量错误
    */
    SemaphoreResult maintainEventPool(size_t& released);

    /** 可选地查询事件等待记录池容量，包含不可复用退役记录及扩容预留
    只用于观测，不配置或创建池，不是普通接口或组合接口的调用前提
    未显式配置时，首次事件等待建立默认池之前各项记录统计为0
    @return 观测快照
    */
    SemaphorePoolStats eventPoolStatistics() const;

    /** 可选配置计数等待的内部记录池，首次需要优先交付登记时默认创建；与事件池配置独立
    仅在外部静止且积存数为0时调用；不影响原有signal/wait的正常调用流程。
    @param [in] config 计数等待记录的容量及扩缩容策略
    @return OK或配置/资源错误，失败保留原池
    */
    SemaphoreResult configureCountPool(const SemaphorePoolConfig& config);

    /** 按工作预算维护计数等待池，无需在每次等待前后调用
    @param [out] released 物理释放记录数
    @return 维护结果；对象生命周期由调用方保持
    */
    SemaphoreResult maintainCountPool(size_t& released);

    /** 单独观测计数等待池，包含退役和预留记录；未创建时均为0
    @return 计数池观测值，不含事件池，不是登记完成屏障
    */
    SemaphorePoolStats countPoolStatistics() const;

    /** 移动端宿主在首次跨进程通知前配置共享目录，各参与进程须传入同一路径。
    Android使用应用内部专用目录，iOS使用App Group容器中的专用目录；必须已存在且属于当前UID。
    配置期间不能并发创建/打开通知；首次使用后仅接受原路径。桌面原生后端只接受空路径。
    @param [in] directory 各参与进程约定的共享目录
    @return 配置成功返回true；移动端空路径、非法目录或使用后改路径返回false
    */
    static bool setProcessDirectory(const std::string& directory);

    /** 进程信号量的创建
    @param [in] name 名字
    @param [in] signalCount 最大通知计数，Windows及移动共享计数后端使用；不是初始计数
    */
    void createProcessSemaphore(const std::string& name, int32_t signalCount = 800000000);

    /** 有结果的创建/加入；初始化异常转为NO_MEMORY，局部资源完整清理
    @param [in] name 名称，沿平台命名限制，禁止NUL和/；桌面原生命名空间沿旧接口处理
    @param [in] signalCount 最大计数，非初始许可；POSIX仍使用系统上限
    @return OK已打开；失败未取得对象，允许重试
    */
    SemaphoreResult createProcessResult(const std::string& name, int32_t signalCount = 800000000);

    /** 打开信号量
    @param [in] name 名字
    */
    void openProcessSemaphore(const std::string& name);

    /** 打开已有进程信号量，不创建不存在对象
    @param [in] name 现有对象名称，不创建新信号量
    @return true打开成功；不存在或错误为false
    */
    bool tryOpenProcessSemaphore(const std::string& name);

    /** 有结果的打开既有对象，不创建名称
    @param [in] name 逻辑名称
    @return OK已打开；失败未取得对象
    */
    SemaphoreResult openProcessResult(const std::string& name);

    /** 关闭信号量，在析构中也会执行，重复执行不会崩溃
    */
    void closeProcessSemaphore();

    /** 静止期关闭本实例，检查撤名和系统关闭结果
    @return OK释放完成；系统失败仍清理其他资源，未释放映射/引用保留供重试
    */
    SemaphoreResult closeProcessResult();

    /** POSIX/移动后端仅独占创建者可撤销本代名称，打开者及同名加入者拒绝撤名
    成功撤名即消耗本实例的撤名权，已有引用继续使用；同名重建由新创建者负责。
    各参与方须遵守唯一owner协议，不可绕过本库直接撤名/重建正在使用的名称。
    @return true已撤销/不存在；Windows由内核引用管理名称，返回false
    */
    bool unlinkProcessName();

    /** 对命名对象积存一个许可；POSIX创建者关闭移除名称，已打开对端仍可使用旧对象
    */
    void processSignal();

    /** 以明确结果发布一次通知，不等待其他进程释放锁
    @return 0已发布，1计数满，-1未打开或其他错误
    */
    int32_t tryProcessSignal();

    /** 有结果的命名发布
    @return OK已提交、OVERFLOW未提交、NOT_OPEN或系统错误
    */
    SemaphoreResult processSignalResult();

    /** 进程等待
    */
    void processWait();

    /** 进程等待
    @param [in] timeout 超时时间，单位毫秒
    @return 等到信号返回true，超时或未初始化返回false
    */
    bool processWait(int32_t timeout);

    /** 有结果的命名等待
    @param [in] timeout -1无限，0探测，正数总预算毫秒，低于-1非法
    @return OK消费、TIMEOUT或具体失败
    */
    SemaphoreResult processWaitResult(int32_t timeout);

    /** 可选的资源清理诊断，查询当前模块累计失败数，饱和不回绕
    普通接口和组合接口均可查询，无需配套调用，也不是每个Semaphore实例的错误状态
    本函数只读统计，不执行清理；需要逐次关闭错误时优先用closeProcessResult
    @return 当前模块累计清理失败数，不是逐调用错误码
    */
    static uint32_t cleanupFailures();

// 通知后立即等待回执时使用的性能组合接口
public:
    /** 外部静止期为频繁请求/回执准备组合等待，两个参与对象分别调用；失败保留原后端
    冷唤醒的请求/回执往返优先准备后使用signalAndWaitResult，减少系统调用及调度开销
    Windows准备成功会切换该对象以后所有等待的后端；普通计数热路径可能变慢，应单独建对象
    Windows准备一个系统事件并使用组合系统调用，其余平台保留原等待机制；不启动线程。
    计数仍为64位，事件仍非积存；既有signal/wait/event接口继续可用。
    @return OK或系统错误/宿主不能阻塞；与等待/通知并发调用属于生命周期错误
    */
    SemaphoreResult prepareSignalAndWait();

    /** 本地发布一个64位许可后无限等待另一对象的计数许可，不承诺两阶段原子完成
    通知后立即等待回执的冷唤醒场景优先使用本组合接口，以降低往返延迟
    两端prepareSignalAndWait成功时可采用组合系统调用；未准备仍按先发布后等待执行。
    只适用于发送后紧接等待回执的调用链；回执与请求的代次由上层关联。
    @param [in,out] waitOn 目标本地对象，两端须活到调用结束，可为当前对象
    @return 分别报告发布和消费；发送已提交后等待失败/超时不可重新发送
    */
    SemaphoreSignalWaitResult signalAndWaitResult(Semaphore& waitOn);

    /** 本地发布后有限等待，沿用一个单调总预算
    通知后立即等待回执时优先使用本接口；准备方法及热路径代价见prepareSignalAndWait
    @param [in,out] waitOn 目标本地对象，生命周期与无限版本相同
    @param [in] timeout 总预算毫秒，0只在发布后探测目标许可
    @return 发布/消费状态及错误，不能根据等待失败重发已经发布的许可
    */
    SemaphoreSignalWaitResult signalAndWaitResult(Semaphore& waitOn, uint32_t timeout);

    /** 对当前命名对象发布一个许可，然后等待目标对象；两个对象均须已打开并保持存活
    跨进程通知后立即等待回复时优先使用本组合接口，Windows可减少系统调用及往返延迟
    Windows原生后端合并系统调用，其余原生后端按同一总预算顺序执行；不承诺两个阶段原子完成。
    无锁语义与processWait一致；不会改变名称或共享协议，不用于本地signal/wait。
    TIMEOUT且signalState=DONE时只可继续等待，不能再次发布；UNKNOWN须用业务代次恢复。
    close/configure/destruct不与本次调用并发；双方不能把对方回复当作本次请求自动关联。
    @param [in,out] waitOn 等待对象，允许与当前对象相同；发送失败时不开始等待
    @param [in] timeout 总预算毫秒，-1无限，0发送后立即探测，小于-1拒绝且不发送
    @return 分别报告发送/消费状态；原生组合调用的未知失败保守报告UNKNOWN，Wasm为UNSUPPORTED
    */
    SemaphoreSignalWaitResult processSignalAndWaitResult(Semaphore& waitOn, int32_t timeout = -1);

private:
    /** 发布计数并可选延迟系统提示，延迟仅供本地组合系统调用使用
    @param [in] notify 是否立即发出系统提示
    @return 许可提交及提示结果
    */
    SemaphoreResult publishCount(bool notify);

    /** 仅尝试普通积存计数加一，已登记等待者或大计数必须由调用方进入慢路径
    @return true表示已增加许可且尚未系统提示，false表示没有修改状态
    */
    bool incrementCount();

    /** 按需发布独立计数记录池，失败不改变许可
    @param [out] pool 当前实例的计数池
    @return 初始化结果
    */
    SemaphoreResult prepareCountPool(SemaphoreCount*& pool);

    /** 等待已登记记录，退出时与交付竞争取消并归还记录
    @param [in] record 本次登记记录
    @param [in] deadline 整次调用共用的截止预算
    @param [in] entered 是否已登记公共系统等待，函数负责撤销
    @return 交付状态及系统错误
    */
    SemaphoreResult waitCountRecord(SemaphoreEventRecord* record, const SemaphoreDeadline& deadline, bool entered = false);

    /** 取得命名资源，只有成功的独占创建取得撤名权；失败由局部候选回收
    @param [in] name 已有命名规则中的逻辑名称
    @param [in] signalCount 正数创建，0仅打开
    @return 已有残留资源时拒绝覆盖，其他失败保留原生错误
    */
    SemaphoreResult connectProcess(const std::string& name, int32_t signalCount);

    /** 按原调用的预算等待共享计数或POSIX信号量，不在组合发送后重启计时
    @param [in] deadline 原调用总预算
    @param [in] infinite 是否允许使用POSIX无限系统等待
    @return 本次许可消费状态和原生错误
    */
    SemaphoreResult waitProcess(const SemaphoreDeadline& deadline, bool infinite);

    /** 按需取得默认事件池；并发首次申请以CAS发布完整候选，不等待初始化持有者
    @param [out] pool 本实例的稳定池引用，外部静止才允许更换/销毁
    @return OK或未发布失败
    */
    SemaphoreResult prepareEventPool(SemaphoreEventPool*& pool);

    /** 计数等待入口，普通积存许可直接消费，不分配记录或读取时钟
    @param [in] infinite 是否无限等待
    @param [in] timeout 有限等待毫秒
    @return 真实谓词结果或系统错误
    */
    SemaphoreResult waitCount(bool infinite, uint32_t timeout);

    /** 登记计数等待者，再次核对许可后进入系统等待
    @param [in] deadline 原调用的总预算，组合调用也不重建预算
    @return 是否消费许可；退出前撤销系统等待登记
    */
    SemaphoreResult waitCountRegistered(const SemaphoreDeadline& deadline);

    /** 短时竞争沿用系统阻塞，持续32ms未取得许可则请求转入优先交付登记
    @param [in] deadline 原调用总预算，阶段切换不重新开始计时
    @param [out] priority true表示仍未消费，应进入计数池；false表示返回最终结果
    @return 本阶段消费、超时或系统错误，退出前归还公共系统等待登记
    */
    SemaphoreResult waitCountCompetitive(const SemaphoreDeadline& deadline, bool& priority);

    /** 登记一次非积存事件等待，退出时与通知竞争最终结果
    @param [in] infinite 是否无限等待
    @param [in] timeout 有限等待毫秒
    @return 事件结果或资源错误
    */
    SemaphoreResult waitEvent(bool infinite, uint32_t timeout);

    /** 实现本地组合调用，已知的许可CAS提交状态不依赖系统提示是否成功
    @param [in,out] waitOn 消费目标，两对象生命周期外部保持
    @param [in] infinite 是否无限等待
    @param [in] timeout 有限总预算毫秒
    @return 发布/消费的独立结果
    */
    SemaphoreSignalWaitResult signalAndWaitLocal(Semaphore& waitOn, bool infinite, uint32_t timeout);

    /** 尝试消费一次许可
    @return true取得一个许可
    */
    bool consumeCount();

    /** 兼容接口将错误明确传播，超时正常返回
    @param [in] result 本次操作结果
    */
    static void checkLocalResult(const SemaphoreResult& result);

    /** 核对各端共同名称约束
    @param [in] name 逻辑名称
    @return 是否可传给系统后端；移动协议限30字节，桌面额外限制由系统报告
    */
    static bool validProcessName(const std::string& name);

private:
#if defined(_MSC_VER) && !defined(STATIC_LIB)
    // 这些私有成员由DLL内方法构造、访问和销毁，调用方仍须使用匹配的头和CRT
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    // 直接积存数或带标记的登记链/大计数记录；CAS提交许可，对象析构/reset须外部静止
    // MSVC x86不会为自然8字节成员自动重对齐栈，必须显式要求，避免跨缓存行原子访问
#ifdef _MSC_VER
    __declspec(align(8))
#endif
    std::atomic<uint64_t> m_count;
    // 系统睡眠提示，独立于业务许可/事件，不维护自有锁
    SemaphoreWake m_wake;
    // 默认按首次事件等待创建；configure静止期显式预建；计数/通知不分配池
    std::atomic<SemaphoreEventPool*> m_eventPool;
    // 计数等待单独登记并直接接收许可；普通积存热路径不创建此池。
    std::atomic<SemaphoreCount*> m_countPool;
    // 移动端后备由本库直接通过系统映射管理，不依赖其他工程；桌面原生路径保持为空。
    std::unique_ptr<SemaphoreProcessMemory> m_processMemory;
#if defined(_MSC_VER) && !defined(STATIC_LIB)
#pragma warning(pop)
#endif
    // 原生HANDLE或sem_t*，仅实现文件解释；所有平台公共接口相同
    void* m_processSemaphore;
    // POSIX仅新建者拥有名称移除权；打开者只释放自己的引用
    bool m_processCreated;
#if defined(_MSC_VER) && !defined(STATIC_LIB)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    // POSIX名称撤销责任，成功撤名后清空，禁止旧代重复删除
    std::string m_processName;
#if defined(_MSC_VER) && !defined(STATIC_LIB)
#pragma warning(pop)
#endif
};