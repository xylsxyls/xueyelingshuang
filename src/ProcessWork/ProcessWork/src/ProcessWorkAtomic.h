#pragma once
#include <stdint.h>

/** 原生共享映射的固定宽度原子操作；字段必须自然对齐，不构造进程内同步对象。
*/
class ProcessWorkAtomic
{
public:
    /** 读取32位共享字段，取得发布端之前写入的正文。
    @param [in] value 四字节对齐字段
    @return acquire快照
    */
    static int32_t load(const int32_t* value);

    /** 读取64位共享字段；x64使用对齐读取，32位Windows使用原生CAS防止撕裂。
    @param [in] value 八字节对齐字段
    @return acquire快照
    */
    static int64_t load(const int64_t* value);

    /** 独占字段的写入者发布64位共享值，竞争决定所有权的字段仍应使用replace。
    @param [in,out] value 八字节对齐且由调用方独占写入的字段
    @param [in] desired 发布值
    */
    static void store(int64_t* value, int64_t desired);

    /** 比较并发布32位共享字段。
    @param [in,out] value 四字节对齐字段
    @param [in] expected 预期值
    @param [in] desired 新值
    @return true已发布，false未修改
    */
    static bool replace(int32_t* value, int32_t expected, int32_t desired);

    /** 比较并发布64位共享字段。
    @param [in,out] value 八字节对齐字段
    @param [in] expected 预期值
    @param [in] desired 新值
    @return true已发布，false未修改
    */
    static bool replace(int64_t* value, int64_t expected, int64_t desired);

    /** 比较并交换64位字段，直接返回CAS观察到的原值，避免失败后再读同一缓存行。
    @param [in,out] value 八字节对齐共享字段
    @param [in] expected 预期值
    @param [in] desired 新值
    @return 等于expected表示提交成功，否则是导致本次CAS失败的实际原值
    */
    static int64_t compareExchange(int64_t* value, int64_t expected, int64_t desired);

    /** 原子加一并返回新值；登记及通知代次使用，不预留消息提交空洞
    @param [in,out] value 八字节对齐的非负计数器，调用方限制耗尽边界
    @return 加一后的值
    */
    static int64_t increment(int64_t* value);

    /** 竞争失败后执行一次处理器提示，不等待某个线程、不持有提交权。
    */
    static void relax();
};

#include "ProcessWorkAtomic.inl"