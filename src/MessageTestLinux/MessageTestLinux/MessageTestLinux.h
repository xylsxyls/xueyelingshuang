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
#include "TextMessage.h"
#include "TextStorage.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

class Label;
class TextEdit;
class COriginalButton;
class LineEdit;
class QFocusEvent;
class MsgLinux; 
class CheckBox;
class QTimer;

class MessageTestLinux : public QDialog
{
	Q_OBJECT
public:
	friend class MessageReceiveTask;
	friend class MessagePidTask;
	friend class MessageRecoverTask;
	friend class AddStringTask;
	friend class AvailableTask;
	friend class StringTask;
	friend class TextStorage;
	
public:
	MessageTestLinux(QWidget* parent = nullptr);
	~MessageTestLinux();

protected:
	void init();
	bool check();

	bool textAvailable(const std::shared_ptr<TextMessage>& textMessage);
	int32_t allToShow();
	std::string textToString(const std::shared_ptr<TextMessage>& textMessage,
		bool isShowParam);

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
	void onExternClicked();
	void onUpdateWindow();
	void onReceivePeopleId(int32_t peopleId);
	void onReceivePid(int32_t pid);
	void onReceivePidThreadId(int32_t pid, int32_t threadId);
	void onReceiveLostPid(int32_t pid);
	void onStateChanged(int32_t state);
	void onShowTextParamStateChanged(int32_t state);
	void onFilterChanged(const QString& text);
	void onFilterTimer();

private:
	COriginalButton* m_button;
	TextEdit* m_textEdit;
	Label* m_area;
	Label* m_screen;
	Label* m_all;
	std::atomic<int32_t> m_areaCount;
	std::atomic<int32_t> m_screenCount;
	std::atomic<int32_t> m_allCount;
	std::atomic<int32_t> m_id;
	PushButton* m_change;
	PushButton* m_refresh;
	PushButton* m_clear;
	PushButton* m_toFile;
	PushButton* m_toAppointFile;
	PushButton* m_extern;
	LineEdit* m_appointFilePath;
	uint32_t m_receiveThreadId;
	uint32_t m_addStringThreadId;
	uint32_t m_recoverThreadId;
	int32_t m_externWidth;
	MsgLinux* m_msg;
	ReadWriteMutex m_allStringMutex;
	TextStorage m_textStorage;
	LockFreeQueue<std::shared_ptr<TextMessage>> m_listReceiveStr;
	ReadWriteMutex m_selectPeopleIdMutex;
	std::set<int32_t> m_selectPeopleId;
	ReadWriteMutex m_selectPidMutex;
	std::set<int32_t> m_selectPid;
	ReadWriteMutex m_selectThreadIdMutex;
	std::set<int32_t> m_selectThreadId;

	ReadWriteMutex m_paramMutex;
	std::set<int32_t> m_allPeopleId;
	//pid, threadId
	std::map<int32_t, std::set<int32_t>> m_pidThreadIdReceiveMap;

	std::vector<CheckBox*> m_allPeopleIdCheckBox;
	std::vector<CheckBox*> m_allPidCheckBox;
	std::vector<CheckBox*> m_allThreadIdCheckBox;
	ReadWriteMutex m_showMutex;
	FiniteDeque<std::shared_ptr<TextMessage>> m_showStr;
	ReadWriteMutex m_pidMutex;
	std::set<uint32_t> m_pid;
	std::map<uint32_t, uint32_t> m_pidThreadId;
	std::atomic<bool> m_isChange;
	std::atomic<bool> m_update;
	int32_t m_textContentCount;
	QString m_lineHeight;
	std::atomic<bool> m_isClear;
	std::atomic<bool> m_isChangeState;
	char m_processNameIdInit;
	char m_processNameId;
	int32_t m_assignThreadId;
	//pid, threadId, assignThreadId
	std::map<int32_t, std::pair<char, std::map<int32_t, int32_t>>> m_pidThreadIdMap;
	CheckBox* m_showTextParam;
	LineEdit* m_filterText;
	std::string m_filterStr;
	ReadWriteMutex m_filterMutex;
	QTimer* m_filterTimer;
};

#endif // QTTEST_H