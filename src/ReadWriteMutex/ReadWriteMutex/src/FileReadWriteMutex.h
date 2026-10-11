#pragma once
#include "ReadWriteMutexBase.h"
#include <string>

#ifdef _MSC_VER
typedef void* HANDLE;
#endif

/** 锁文件实现的跨进程独占访问，读写共用同一锁；对象内状态不提供并发调用同步
*/
class ReadWriteMutexAPI FileReadWriteMutex : public ReadWriteMutexBase
{
public:

	/** 保存锁文件路径并准备POSIX描述符；Windows加锁时打开文件
	@param [in] filePath 含斜杠或反斜杠时原样作路径，否则用临时目录加名字及.lock；不是仅按绝对路径判断
	*/
	FileReadWriteMutex(const std::string& filePath);

	/** 释放仍持有的锁并关闭POSIX描述符，不允许与其他方法并发
	*/
	virtual ~FileReadWriteMutex();

public:
	/** 按write语义取得文件独占锁，不是共享读锁
	*/
	virtual void read();

	/** 同步取得文件独占锁，已持有时不重复加锁；失败输出诊断但无结果返回
	*/
	virtual void write();

	/** 按unwrite语义释放本对象持有的文件锁
	*/
	virtual void unread();

	/** 释放文件锁；Windows同时关闭句柄并删除以名字生成的临时锁文件
	*/
	virtual void unwrite();

#if defined(__unix__) || defined(__APPLE__)
	/** POSIX不等待地尝试独占文件锁，失败输出诊断；接口无结果返回
	*/
	void trywrite();

#endif

	/** 取得平台临时目录，POSIX优先TMPDIR并确保末尾分隔符
	@return 带平台路径分隔符的临时目录字符串
	*/
	static std::string tempDir();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	FileReadWriteMutex(const FileReadWriteMutex& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	FileReadWriteMutex& operator=(const FileReadWriteMutex& other);

private:
#ifdef _MSC_VER
	// Windows文件锁句柄，未加锁时为nullptr。
	// Windows锁文件句柄，未加锁时为nullptr
	HANDLE m_file;
#elif defined(__unix__) || defined(__APPLE__)
	// Linux文件锁fd，打开失败时为-1。
	// POSIX文件描述符，打开失败为负一
	int m_fd;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 临时锁文件目录，传入名字时会拼到该目录下。
	// 静态初始化的临时目录，用于没有路径分隔符的锁名字
	static std::string s_tempDir;
	// 实际使用的锁文件路径。
	// 实际锁文件路径，由输入路径或临时目录加名字生成
	std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 是否由名字生成临时锁文件路径。
	// 输入没有斜杠或反斜杠，使用临时目录生成锁文件路径
	bool m_isName;
	// 当前对象是否已经成功持有文件锁。
	// 本对象是否成功持有文件锁，不是跨线程同步标志
	bool m_isLocked;
};