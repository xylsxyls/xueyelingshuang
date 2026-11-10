#pragma once
#include "LockFreeHashMapAPI.h"
#include <string>

/** 预先初始化的资源替身，实际映射、打开及 PID 身份属于 ProcessWork，本例不执行真实 IPC
*/
struct LockFreeHashMapMigrationChannel
{
public:
    // 示例进程编号
    int m_pid;
    // 示例进程创建身份
    uint64_t m_processIdentity;
    // 示例接收器代次
    uint64_t m_receiverGeneration;
    // 是否已经完成示例预映射
    bool m_mapped;
public:
    /** 在发布前完成示例初始化
    @param [in] pid 进程编号
    @param [in] identity 进程创建身份
    @param [in] generation 接收器代次
    */
    LockFreeHashMapMigrationChannel(int pid, uint64_t identity, uint64_t generation);
};

/** 两类缓存、按版本淘汰及在途注册表所有权的可编译迁移示例
*/
class LockFreeHashMapMigration
{
public:
    /** 执行迁移示例并核对独立预期
    @return 所有预期满足为 true，准备资源异常由用例入口捕获
    */
    static bool run();

    /** 普通函数形式的固定增长策略
    @param [in] context 调用期容量观测，不承诺可申请额度
    @return 期望新增槽位数，不是目标总容量
    */
    static size_t fixedGrowth(const LockFreeHashMapGrowContext& context);
};

/** 无状态函数对象策略，无进程全局可变状态
*/
struct LockFreeHashMapMigrationGrow
{
public:
    /** 按比例增长，至少请求一个槽位
    @param [in] context 调用期容量观测
    @return 期望新增槽位数，实际额度由库截断
    */
    size_t operator()(const LockFreeHashMapGrowContext& context) const;
};