#pragma once
#include "ReadWriteMutexBase.h"
#include <string>

#ifdef _MSC_VER
typedef void* HANDLE;
#endif

class ReadWriteMutexAPI FileReadWriteMutex : public ReadWriteMutexBase
{
public:
	/** ���캯��
	@param [in] filePath �ļ�·������������ֻ�����ʱ·���´����ļ���windows�»�ɾ����ʱ�ļ�������������·����ɾ��
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