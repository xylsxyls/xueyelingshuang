#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <stdint.h>
#include "SemaphoreMacro.h"
#include <memory>

/** 本进程通知与原生跨进程计数通知。生命周期操作不与等待/通知并发。
    Windows使用内核信号量，Linux/macOS使用sem_open；移动端使用共享原子计数及有界睡眠，无文件锁。
    可移植逻辑名称限30字节，不含斜杠/NUL；手机沙箱是否允许须由宿主验证。
    Wasm不支持跨进程命名通知，创建无资源、打开和等待返回失败；本地计数操作仍可用。
*/
class SemaphoreAPI Semaphore
{
public:
    /** 构造函数
    */
    Semaphore();

    /** 析构函数
    */
    ~Semaphore();

public:
    /** 移动端宿主在首次跨进程通知前配置共享目录，各参与进程须传入同一路径。
    Android使用应用内部专用目录，iOS使用App Group容器中的专用目录；必须已存在且属于当前UID。
    配置期间不能并发创建/打开通知；首次使用后仅接受原路径。桌面原生后端只接受空路径。
    @return 配置成功返回true；移动端空路径、非法目录或使用后改路径返回false
    */
    static bool setProcessDirectory(const std::string& directory);

    /** 信号量的通知，通知几次wait允许通过几次，wait之前通知一次则wait执行时允许通过，一对一，不允许一对多
    本地积压计数为64位；达到UINT64_MAX时抛出overflow_error，本次不增加计数
    */
    void signal();

    /** 生命周期owner在生产者及消费者全部停止后清除本地积存通知。 */
    void reset();

    /** 信号量的等待
    */
    void wait();

    /** 信号量的等待
    @param [in] timeout 总等待时间，单位毫秒，0仅检查现有计数
    @return 取得信号返回true，超时返回false；无限等待使用无参数wait()
    */
    bool wait(uint32_t timeout);

    /** 进程信号量的创建
    @param [in] name 名字
    @param [in] signalCount 最大通知计数，Windows及移动共享计数后端使用；不是初始计数
    */
    void createProcessSemaphore(const std::string& name, int32_t signalCount = 800000000);

    /** 打开信号量
    @param [in] name 名字
    */
    void openProcessSemaphore(const std::string& name);

    /** 打开已有进程信号量，不创建不存在对象
    @param [in] name 现有对象名称，不创建新信号量
    @return true打开成功；不存在或错误为false
    */
    bool tryOpenProcessSemaphore(const std::string& name);

    /** 以明确结果发布一次通知，不等待其他进程释放锁
    @return 0已发布，1计数满，-1未打开或其他错误
    */
    int32_t tryProcessSignal();

    /** 关闭信号量，在析构中也会执行，重复执行不会崩溃
    */
    void closeProcessSemaphore();

    /** POSIX名称owner撤销本代名称，已打开的等待者仍可使用；不关闭本实例。
    @return true已撤销/不存在；Windows由内核引用管理名称，返回false
    */
    bool unlinkProcessName();

    /** 进程信号，通知仅作提示；POSIX创建者关闭移除名称，已打开对端仍可使用该对象
    */
    void processSignal();

    /** 进程等待
    */
    void processWait();

    /** 进程等待
    @param [in] timeout 超时时间，单位毫秒
    @return 等到信号返回true，超时或未初始化返回false
    */
    bool processWait(int32_t timeout);

    /** 事件通知，多个事件同时等待只会通知一个，必须进入了eventWait，通知才有效果，一对一，允许一对多
    */
    void event();

    /** 事件通知，同时通知多个事件，必须进入了eventWait，通知才有效果，一对多
    */
    void eventAll();

    /** 事件的等待
    */
    void eventWait();

    /** 事件的等待
    @param [in] timeout 超时时间，单位毫秒
    */
    bool eventWait(int32_t timeout);

private:
    // 本进程条件变量的短锁；原生命名通知的发送和等待不取得此锁
    std::mutex m_mtx;
    std::condition_variable m_cv;
    // 只由原有本进程短锁保护，避免大量积压通知跨过32位有符号上限
    uint64_t m_count;
    // 移动端后备由本库直接通过系统映射管理，不依赖其他工程；桌面原生路径保持为空。
    struct ProcessSemaphoreMemory;
    std::unique_ptr<ProcessSemaphoreMemory> m_processMemory;
    // 原生HANDLE或sem_t*，仅实现文件解释；所有平台公共接口相同
    void* m_processSemaphore;
    // POSIX仅新建者拥有名称移除权；打开者只释放自己的引用
    bool m_processCreated;
    std::string m_processName;
};