#pragma once

#include <stdint.h>

/** 验证普通/try接纳错误分类、内存保护回滚及成功消息FIFO
*/
void testProcessWorkResultContracts();

/** 回调线程请求关闭不释放仍在借用的资源，owner随后完成关闭。
*/
void testProcessWorkSelfClose();

/** 验证共享字段发布顺序、完整64位读取以及失败CAS不修改目标。
*/
void testProcessWorkAtomics();

/** 验证内存保护参数、溢出边界和保留量拒绝结果。
*/
void testProcessWorkMemoryGuard();

/** 验证分页寻址、并发映射安装及正文缩容后的代次隔离。
*/
void testProcessWorkStorage();

/** 验证预留不阻挡已提交节点、尾指针帮助以及源节点代次拒绝旧CAS。
*/
void testProcessWorkSharedQueue();

/** 在旧轮次末尾新增就绪流，首次front即应找到它，覆盖旧流空闲及死亡两种情况
*/
void testProcessWorkNewStreamRound();

/** 验证共享队列8生产者连续复用节点，无丢失、重复或线程内乱序。
*/
void testProcessWorkSharedQueueStress();

/** 验证无配置上限能跨过旧1024边界，且零阈值不提前扩容。
*/
void testProcessWorkUnlimited();

/** 验证固定扩容计划按字节预算分批完成，内存保护拒绝时完整回滚。
*/
void testProcessWorkGrowthBudget();

/** 白盒子进程在共享提交的不同阶段退出，不插入生产库测试钩子。
@param [in] argc 参数个数
@param [in] argv 参数列表
@return 子进程退出状态
*/
int runProcessWorkFaultChild(int argc, char** argv);

/** 验证死亡生产者未提交占位回收、已提交正文保留和暂停生产者不被误回收。
@param [in] phase 0正文占位、1节点占位、2正常提交、3只链接未推进尾、4存活暂停
@param [in] abrupt true强制终止实际子进程，false正常退出但留下占位
*/
void testProcessWorkDeadProducer(int32_t phase, bool abrupt = false);

/** 测试子进程作为接收者，按参数阻塞回调或验证重启后的4000包
@param [in] argc 参数数量
@param [in] argv 参数列表
@return 子进程退出状态
*/
int runProcessWorkCrashReceiver(int argc, char** argv);

/** 满载时杀死接收进程，全部发送线程结束等待，同名重启后完整交付
*/
void testProcessWorkReceiverCrash();

/** 本地预算耗尽时杀死已提交发送者，维护不能回收尚未复制的正文。
*/
void testProcessWorkDeadSenderBackpressure();

/** 验证批次复制、每包计费、慢消费者预算、块复用和静止清理。
*/
void testProcessWorkReceiveBatches();

/** 在保护页边界验证0..96字节正文的接收复制，不得越界读写或借用已变化的源数据
*/
void testProcessWorkReceiveCopyBoundary();

/** 验证接收批次在独立生产和消费线程之间的可变包长FIFO及预算归还。
*/
void testProcessWorkReceiveBatchStress();

/** 验证单槽在36字节内联边界两侧复用，不读取另一种存储位置的旧正文。
*/
void testProcessWorkInlineBoundary();

/** 验证通道缓存借用不复制shared_ptr，重入清空仍保护原值且回调异常不重复调用。
*/
void testProcessWorkBorrowedCache();

/** 快缓存并发清理不释放在途借用，重入回退且过期查找不能重新命中
*/
void testProcessWorkSendCache();

/** 接收批次预分配独立于消息额度，循环复用及配置失败重试不遗失正文
*/
void testProcessWorkReceiveReserve();

/** 验证Post小包预留、池满及大包回退、并发归还和计费FIFO
*/
void testProcessWorkPostPacketPool();

/** 验证等待请求不封锁其他空闲资源，归还正文仍按等待链直接授予且不能被抢走
*/
void testProcessWorkFreeSlotsWithWaiters();

/** 同一物理线程交替使用两个通道，在跨页、跨段、复用及重新打开后核对完整FIFO
*/
void testProcessWorkSenderAddressCache();

/** 接收器跨页消费并在同一对象上重新打开不同容量，旧映射不得被缓存继续使用
*/
void testProcessWorkReceiverPageReuse();

/** 扫描提示不能成为私有容量边界，验证跨窗口满载、完全排空及重复使用的FIFO
*/
void testProcessWorkReservationWindow();

/** 验证本地正文计费、单条大消息、失败归还与处理期间保留预算。
*/
void testProcessWorkLocalBudget();

/** 验证合并通知在高并发满载和空闲切换中不丢失消息。
*/
void testProcessWorkLocalQueueStress();

/** 验证tryPost预算失败不会提交，未启动发送线程时也不会无限等待。
*/
void testProcessWorkPostBudget();

/** 32个生产者普通post发送123456；调用端不重试，预算足够时不得因竞争抛异常
*/
void testProcessWorkPostContention();

/** 阻塞回调时本地计费不增长，共享满载产生背压，恢复后消息完整有序。
*/
void testProcessWorkSlowCallback();

/** 篡改已提交节点的正文下标，验证撤销通道且不重复交付
*/
void testProcessWorkCorruption();

/** 正文映射不可用时普通发送返回资源错误且归还占位，恢复映射后可以继续发送
*/
void testProcessWorkMappingFailure();

/** 未批准登记与满载分别返回资源/容量错误，不误报CAS竞争Busy；恢复后精确交付
*/
void testProcessWorkSendFailureKinds();

/** 预热20轮后再执行100轮真实初始化、发送、关闭，验证句柄及私有内存平台
*/
void testProcessWorkResourcePlateau();

/** 诊断对照只使用系统运行库，不调用ProcessWork或基础库，输出线程/互斥句柄高水位
*/
void runProcessWorkRuntimeControl();