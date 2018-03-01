#include "Thread.h"
#include "DialogManager.h"

Thread::Thread(QObject *parent)
{
	number = 0;
}

Thread::~Thread()
{

}

void Thread::run()
{
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