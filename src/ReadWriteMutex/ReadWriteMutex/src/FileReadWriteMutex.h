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

	~FileReadWriteMutex();

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
	HANDLE m_file;
#elif __unix__
	int m_fd;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	static std::string s_tempDir;
	std::string m_filePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	bool m_isName;
};