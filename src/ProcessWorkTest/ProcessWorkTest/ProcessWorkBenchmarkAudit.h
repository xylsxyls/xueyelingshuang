#pragma once
#include <string>
#include <stdint.h>

/** 仅测试工程及隔离诊断副本使用的容量审计，不加入正式库的代码或布局
*/
class ProcessWorkBenchmarkAudit
{
public:
    /** 开始本进程计时区间的事件统计，先清空旧值再开放记录
    */
    static void begin();

    /** 停止记录，调用方随后完成线程停止或读取原子统计
    */
    static void finish();

    /** 记录实际发生的分配、扩容或容量等待事件
    @param [in] event 0未取到槽，1容量等待，2接收正文申请，3Post正文申请，4队列增长，5Map增长，6共享页创建/映射，7完整探测容量不足，8Post正文预算等待，9接收本地预算不足
    */
    static void hit(uint32_t event);

    /** 保存各事件计数，文件写入在性能计时之外
    @param [in] path 本次独立进程的结果路径
    */
    static void write(const std::string& path);
};
