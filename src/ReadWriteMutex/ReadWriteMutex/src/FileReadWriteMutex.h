#pragma once
#include "ReadWriteMutexBase.h"
#include <string>

#ifdef _MSC_VER
typedef void* HANDLE;
#endif

class ReadWriteMutexAPI FileReadWriteMutex : public ReadWriteMutexBase
{
public:
	/** 构造函数
	@param [in] filePath 文件路径，如果是名字会在临时路径下创建文件，windows下会删除临时文件，如果传入绝对路径则不删除
	*/
	FileReadWriteMutex(const std::string& filePath);

	/** 析构函数
	*/
	virtual ~FileReadWriteMutex();

public:
	virtual void read();
	virtual void write();
	virtual void unread();
	virtual void unwrite();

#ifdef __unix__
	void trywrite();
#endif

	static std::string tempDir();

private:
#ifdef _MSC_VER
	// Windows文件锁句柄，未加锁时为nullptr。
	HANDLE m_file;
#elif __unix__
	// Linux文件锁fd，打开失败时为-1。
	int m_fd;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 临时锁文件目录，传入名字时会拼到该目录下。
	static std::string s_tempDir;
	// 实际使用的锁文件路径。
	std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 是否由名字生成临时锁文件路径。
	bool m_isName;
	// 当前对象是否已经成功持有文件锁。
	bool m_isLocked;
};