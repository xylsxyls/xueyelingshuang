#include "NotifyDialogManager.h"
#include "DialogBase.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"
#include "TipShowDialog.h"
#include "AskShowDialog.h"
#include "LoginShowDialog.h"
#include <QTime>

NotifyDialogManager& NotifyDialogManager::instance()
{
	static NotifyDialogManager showDialogHelper;
	return showDialogHelper;
}

void NotifyDialogManager::showTipDialog(int32_t& dialogId,
									  int32_t userType,
									  const QString& tip,
									  const QString& title,
									  int32_t done,
									  const QString& buttonText,
									  int32_t timeOut,
									  bool isCountDownVisible)
{
	TipShowDialog::showTipDialog(dialogId, userType, title, tip, buttonText, done, timeOut, isCountDownVisible);
}

void NotifyDialogManager::showAskDialog(int32_t& dialogId,
									  int32_t userType,
									  const QString& tip,
									  const QString& title,
									  int32_t acceptDone,
									  int32_t ignoreDone,
									  const QString& acceptText,
									  const QString& ignoreText,
									  int32_t timeOut,
									  bool isCountDownVisible)
{
	AskShowDialog::showAskDialog(dialogId,
								 userType,
								 title,
								 tip,
								 acceptText,
								 acceptDone,
								 ignoreText,
								 ignoreDone,
								 timeOut,
								 isCountDownVisible);
}

void NotifyDialogManager::showLoginDialog(int32_t& dialogId,
										int32_t userType,
										const QString& tip,
										const QString& linkUrl,
										const QString& title,
										int32_t timeOut,
										bool isUrlButtonVisible,
										bool isCountDownVisible)
{
	QString greeting;
	QTime tm = QTime::currentTime();
	int hour = tm.hour();
	if ((hour > 0) && (hour <= 9))
	{
		greeting = QString::fromStdWString(L"Hi~早晨好！");
	}
	else if ((hour > 9) && (hour <= 12))
	{
		greeting = QString::fromStdWString(L"Hi~早上好！");
	}
	else if ((hour > 12) && (hour <= 18))
	{
		greeting = QString::fromStdWString(L"Hi~下午好！");
	}
	else
	{
		greeting = QString::fromStdWString(L"Hi~晚上好！");
	}

	LoginShowDialog::showLoginDialog(dialogId,
									 userType,
									 title,
									 greeting,
									 tip,
									 QString::fromStdWString(L">>了解更多"),
									 linkUrl,
									 timeOut,
									 isUrlButtonVisible,
									 isCountDownVisible);
}

int32_t NotifyDialogManager::popAskDialog(int32_t& dialogId,
										const QString& title,
										const QString& tip,
										const QString& acceptText,
										int32_t acceptDone,
										const QString& ignoreText,
										int32_t ignoreDone,
										QWidget* parent,
										int32_t timeOut,
										bool isCountDownVisible)
{
	return -1;
}

int32_t NotifyDialogManager::popTipDialog(int32_t& dialogId,
										const QString& title,
										const QString& tip,
										const QString& buttonText,
										int32_t done,
										QWidget* parent,
										int32_t timeOut,
										bool isCountDownVisible)
{
	return -1;
}

int32_t NotifyDialogManager::popInputDialog(int32_t& dialogId,
										  const QString& title,
										  const QString& editTip,
										  const QString& buttonText,
										  int32_t done,
										  QString& editText,
										  QWidget* parent,
										  int32_t timeOut,
										  bool isCountDownVisible)
{
	return -1;
}

int32_t NotifyDialogManager::popWaitDialog(int32_t& dialogId,
										 const QString& title,
										 const QString& tip,
										 QWidget* parent,
										 int32_t timeOut,
										 bool isCountDownVisible)
{
	return -1;
}