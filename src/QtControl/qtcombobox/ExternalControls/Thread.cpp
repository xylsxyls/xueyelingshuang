#include "Thread.h"
#include "DialogManager.h"
#include "DownloadOperateDialog.h"

Thread::Thread(QObject *parent)
{
	number = 0;
}

Thread::~Thread()
{

}

void Thread::run()
{
	auto ptr = DialogManager::instance().downloadOperatePtr(m_dialogId);
	if (ptr == nullptr)
	{
		return;
	}

	ptr->setRate(20);
	ptr->setDownloadSpeed("100kb/s");
	ptr->setDownloaded("1MB/14MB");
	ptr->setDownloadTime("11:00:00");

	Sleep(1000);

	ptr = DialogManager::instance().downloadOperatePtr(m_dialogId);
	if (ptr == nullptr)
	{
		return;
	}

	ptr->setRate(40);
	ptr->setDownloadSpeed("200kb/s");
	ptr->setDownloaded("3MB/14MB");
	ptr->setDownloadTime("11:00:01");

	Sleep(1000);

	ptr = DialogManager::instance().downloadOperatePtr(m_dialogId);
	if (ptr == nullptr)
	{
		return;
	}

	ptr->setRate(60);
	ptr->setDownloadSpeed("300kb/s");
	ptr->setDownloaded("7MB/14MB");
	ptr->setDownloadTime("11:00:02");

	Sleep(1000);

	ptr = DialogManager::instance().downloadOperatePtr(m_dialogId);
	if (ptr == nullptr)
	{
		return;
	}

	ptr->setRate(80);
	ptr->setDownloadSpeed("400kb/s");
	ptr->setDownloaded("11MB/14MB");
	ptr->setDownloadTime("11:00:03");

	Sleep(1000);

	ptr = DialogManager::instance().downloadOperatePtr(m_dialogId);
	if (ptr == nullptr)
	{
		return;
	}

	ptr->setRate(100);
	ptr->setDownloadSpeed("500kb/s");
	ptr->setDownloaded("13MB/14MB");
	ptr->setDownloadTime("11:00:04");

	ptr->setEditDownloadAddr("1121212");
	ptr->setEditPath(QStringLiteral("44水电费水电费"));

	ptr->error();

	return;
	//emit error(1);
	DialogManager::instance().downloadError(1);
	//while (1)
	//{
	//	//
	//	emit UpdateSignal(number);
	//	number++;
	//	sleep(1);
	//}
}

void Thread::ResetSlot()
{
	number = 0;
	emit UpdateSignal(number);
}