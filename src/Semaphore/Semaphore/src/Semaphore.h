#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <stdint.h>
#include "SemaphoreMacro.h"
#ifdef _MSC_VER
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <string>
#endif
#if defined(__ANDROID__) || defined(__APPLE__)
#include <array>
#include <sys/stat.h>
#include <sys/types.h>
#endif

/** 信号类，包含信号量和事件
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
	/** 信号量的通知，通知几次wait允许通过几次，wait之前通知一次则wait执行时允许通过，一对一，不允许一对多
	*/
	void signal();

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
	@param [in] signalCount 同时接受的最大信号量个数，该参数只在windows下使用
	*/
	void createProcessSemaphore(const std::string& name, int32_t signalCount = 800000000);

	/** 打开信号量
	@param [in] name 名字
	*/
	void openProcessSemaphore(const std::string& name);

	/** 关闭信号量，在析构中也会执行，重复执行不会崩溃
	*/
	void closeProcessSemaphore();

	/** 进程信号，linux下在进程关掉之后，之前的通知会失效，Semaphore的析构不受影响
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

#if defined(__ANDROID__) || defined(__APPLE__)
private:
    /** 校验只供当前用户使用的普通文件，拒绝链接及外部可写对象
    @param [in] descriptor 已打开的文件
    @return 文件属性；不安全或读取失败时抛出异常
    */
    struct stat processSemaphoreFileInfo(int32_t descriptor) const;

    /** 读取并校验固定v2记录，不解释旧平台布局或重置不兼容对象
    @param [in] descriptor 共享文件，调用方已持有目录锁
    @param [in] allowUnpublished 仅无使用者的create允许恢复完成标记尚未写入的v2记录
    @return 完成标记、版本、字节数、计数及计数反码
    */
    std::array<uint32_t, 5> readProcessSemaphoreState(int32_t descriptor, bool allowUnpublished = false) const;

    /** 完整写入记录或计数，短写及EINTR不会被当作完成
    @param [in] descriptor 共享文件，调用方已持有目录锁
    @param [in] bytes 待写数据，调用期间有效
    @param [in] size 字节数
    @param [in] offset 写入位置
    */
    void writeProcessSemaphoreBytes(int32_t descriptor, const void* bytes, size_t size, off_t offset) const;

    /** 仅在路径仍对应本实例的inode时删除名称，不删除后来创建的代次
    @param [in] descriptor 本次打开的共享文件
    @param [in] name 名称路径，调用方已持有目录锁
    */
    void unlinkProcessSemaphore(int32_t descriptor, const std::string& name) const;

    /** 取得可访问的统一共享目录，启动配置只在未打开时读取
    可选SEMAPHORE_PROCESS_SCOPE覆盖目录，否则使用系统临时目录；运行中不得改环境
    */
    void initProcessSemaphoreDirectory();

    /** 校验逻辑名称并生成共享文件路径，不加入调用者PID，不截断名称
    @param [in] name 逻辑名称，不含路径分隔符或NUL
    @return 共享目录中的完整文件路径
    */
    std::string mobileProcessSemaphoreName(const std::string& name) const;

    /** 在目录锁内创建或打开同名对象，持有共享文件锁表示本实例仍在使用
    @param [in] name 逻辑名称
    @param [in] create true允许创建或回收无使用者的旧对象，false只打开活跃对象
    */
    void openPortableProcessSemaphore(const std::string& name, bool create);

    /** 获取目录级文件锁，串行化创建、初始化、打开及删除
    @return 持有独占锁的文件描述符，失败抛出异常
    */
    int32_t lockProcessSemaphoreDirectory() const;

    /** 释放本次目录锁及文件描述符，不抛出异常
    @param [in] descriptor 本次获取的描述符，负值表示未获取
    */
    void unlockProcessSemaphoreDirectory(int32_t descriptor) const;

    /** 为每次等待创建独立内核观察者，防止多个等待者互相消费唤醒事件，无工作线程
    @param [in] name 本次共享文件路径
    @param [in] descriptor 已打开的共享文件描述符，仅借用
    @return 观察者描述符，调用方负责close；失败抛出异常
    */
    int32_t createProcessSemaphoreObserver(const std::string& name, int32_t descriptor) const;

    /** 检查并原子消费一个通知，读写由目录锁串行化
    @return 取得通知时true，无通知时false；内部错误抛出异常
    */
    bool tryConsumeProcessSemaphore();

    /** 用固定单调截止时间等待内核事件，唤醒后重新检查真实计数
    @param [in] timeout 毫秒，-1无限等待，0只检查，低于-1返回false
    @return 取得通知true，超时或非法超时false；内部错误抛出异常
    */
    bool waitPortableProcessSemaphore(int32_t timeout);
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_mtx;
	std::condition_variable m_cv;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_count;

#ifdef _MSC_VER
	HANDLE m_processSemaphore;
#elif defined(__linux__) && !defined(__ANDROID__)
	int m_processSemaphoreId;
	std::string m_name;
#endif

#if defined(__ANDROID__) || defined(__APPLE__)
    // 捕获的共享目录；通信双方须有访问权限，不随运行中的环境变化切换
    std::string m_processSemaphoreScope;
    // 本次打开的完整原生名称
    std::string m_mobileProcessName;
    // 共享文件及存活锁；所有等待者退出后才能关闭
    int32_t m_mobileProcessFile;
#endif
};