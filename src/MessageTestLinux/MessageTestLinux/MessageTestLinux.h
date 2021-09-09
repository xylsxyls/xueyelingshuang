#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_MessageTestLinux.h"
#include <QtWidgets/QDialog>
#include "QtControls/PushButton.h"
#include <set>
#include <mutex>
#include <atomic>
#include <memory>
#include <QString>
#include <list>
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "FiniteDeque/FiniteDequeAPI.h"

class Label;
class TextEdit;
class COriginalButton;
class LineEdit;
class QFocusEvent;
class MsgLinux;

class MessageTestLinux : public QDialog
{
	Q_OBJECT
public:
	friend class MessageReceiveTask;
	friend class MessagePidTask;
	friend class MessageRecoverTask;
	friend class AddStringTask;
	
public:
	MessageTestLinux(QWidget* parent = nullptr);
	~MessageTestLinux();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onButtonClicked();
	void onChangeClicked();
	void onRefreshClicked();
	void onClearClicked();
	void onToFileClicked();
	void onToAppointFileClicked();
	void updateWindow();

private:
	COriginalButton* m_button;
	TextEdit* m_textEdit;
	Label* m_area;
	Label* m_screen;
	std::atomic<int32_t> m_areaCount;
	std::atomic<int32_t> m_screenCount;
	PushButton* m_change;
	PushButton* m_refresh;
	PushButton* m_clear;
	PushButton* m_toFile;
	PushButton* m_toAppointFile;
	LineEdit* m_appointFilePath;
	uint32_t m_receiveThreadId;
	uint32_t m_addStringThreadId;
	uint32_t m_recoverThreadId;
	MsgLinux* m_msg;
	std::mutex m_addStringMutex;
	std::list<QString> m_listAllStr;
	LockFreeQueue<std::string> m_listReceiveStr;
	std::mutex m_showMutex;
	FiniteDeque<std::string> m_showStr;
	std::mutex m_pidMutex;
	std::set<uint32_t> m_pid;
	std::map<uint32_t, uint32_t> m_pidThreadId;
	std::atomic<bool> m_isChange;
	std::atomic<bool> m_update;
};

#endif // QTTEST_H