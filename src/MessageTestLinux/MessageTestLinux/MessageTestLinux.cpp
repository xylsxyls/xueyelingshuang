#include "MessageTestLinux.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"
#include "QtControls/TextEdit.h"
#include "QtControls/Label.h"
#include "QtControls/PushButton.h"
#include "QtControls/LineEdit.h"
#include "CSystem/CSystemAPI.h"
#include "MsgLinux/MsgLinuxAPI.h"
#include "MessageReceiveTask.h"
#include "MessageRecoverTask.h"
#include <QTimer>
#include "QtControls/CheckBox.h"
#include "TypeManager.h"
#include <QtCore>
#include "SendToMessageTest.h"
#include <fstream>
#include <QTimer>

MessageTestLinux::MessageTestLinux(QWidget* parent):
	QDialog(parent),
	m_button(nullptr),
	m_textEdit(nullptr),
	m_area(nullptr),
	m_screen(nullptr),
	m_all(nullptr),
	m_areaCount(0),
	m_screenCount(0),
	m_allCount(0),
	m_id(0),
	m_change(nullptr),
	m_refresh(nullptr),
	m_clear(nullptr),
	m_toFile(nullptr),
	m_toAppointFile(nullptr),
	m_extern(nullptr),
	m_appointFilePath(nullptr),
	m_receiveThreadId(0),
	m_addStringThreadId(0),
	m_recoverThreadId(0),
	m_externWidth(0),
	m_msg(nullptr),
	m_isChange(false),
	m_update(false),
	m_textContentCount(32),
	m_isClear(false),
	m_isChangeState(false),
	m_processNameIdInit(64),
	m_processNameId(m_processNameIdInit),
	m_assignThreadId(0),
	m_showTextParam(nullptr),
	m_filterText(nullptr),
	m_filterTimer(nullptr)
{
	init();
}

MessageTestLinux::~MessageTestLinux()
{
	m_allPeopleIdCheckBox.clear();

	for (auto itPid = m_allPidCheckBox.begin(); itPid != m_allPidCheckBox.end(); ++itPid)
	{
		(*itPid)->blockSignals(true);
		(*itPid)->setParent(nullptr);
		(*itPid)->setUserData(Qt::UserRole, nullptr);
	}
	m_allPidCheckBox.clear();

	m_allThreadIdCheckBox.clear();
}

void MessageTestLinux::init()
{
	if (!check())
	{
		return;
	}

	setFixedSize(852, 611);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(240, 240, 240, 255));
	setPalette(pattle);

	m_textEdit = new TextEdit(this);
	m_textEdit->move(12, 12);
	m_textEdit->setEnabled(false);
	m_textEdit->setBorderColor(QColor(204, 204, 204));
	m_textEdit->setBackgroundColor(QColor(240, 240, 240));
	m_textEdit->setFontSize(12);
	m_textEdit->setFontFamily(QStringLiteral("Calibri"));
	m_textEdit->ControlBorderForNormalHoverDisabled::setTextColor(QColor(100, 100, 100));

	m_area = new Label(this);
	m_area->setText(QStringLiteral("当前缓冲区剩余展示量：") + QString::fromStdString(std::to_string(m_areaCount - m_allCount)));
	m_area->resize(213, 18);
	m_area->setFontSize(12);
	m_area->setTextColor(QColor(0, 0, 0));

	m_screen = new Label(this);
	m_screen->setText(QStringLiteral("当前屏幕容量：") + QString::fromStdString(std::to_string(m_screenCount)));
	m_screen->resize(213, 18);
	m_screen->setFontSize(12);
	m_screen->setTextColor(QColor(0, 0, 0));

	m_all = new Label(this);
	m_all->setText(QStringLiteral("总容量：") + QString::fromStdString(std::to_string(m_allCount)));
	m_all->resize(213, 18);
	m_all->setFontSize(12);
	m_all->setTextColor(QColor(0, 0, 0));

	m_change = new PushButton(this);
	m_change->setText(QStringLiteral("更改屏幕"));
	m_change->resize(84, 33);
	m_change->setFontSize(12);
	m_change->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_change, &PushButton::clicked, this, &MessageTestLinux::onChangeClicked);

	m_refresh = new PushButton(this);
	m_refresh->setText(QStringLiteral("刷新"));
	m_refresh->resize(84, 33);
	m_refresh->setFontSize(12);
	m_refresh->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_change, &PushButton::clicked, this, &MessageTestLinux::onRefreshClicked);

	m_clear = new PushButton(this);
	m_clear->setText(QStringLiteral("清空"));
	m_clear->resize(182, 49);
	m_clear->setFontSize(12);
	m_clear->setFocus();
	m_clear->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_clear, &PushButton::clicked, this, &MessageTestLinux::onClearClicked);

	m_toFile = new PushButton(this);
	m_toFile->setText(QStringLiteral("转为写入工作路径下默认生成文\n件MessageTestFile.txt"));
	m_toFile->resize(182, 49);
	m_toFile->setFontSize(12);
	m_toFile->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_toFile, &PushButton::clicked, this, &MessageTestLinux::onToFileClicked);

	m_toAppointFile = new PushButton(this);
	m_toAppointFile->setText(QStringLiteral("将屏幕中的内容写入指定文件"));
	m_toAppointFile->resize(182, 30);
	m_toAppointFile->setFontSize(12);
	m_toAppointFile->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_toAppointFile, &PushButton::clicked, this, &MessageTestLinux::onToAppointFileClicked);

	m_extern = new PushButton(this);
	m_extern->setText(QStringLiteral("》"));
	m_extern->resize(13, 30);
	m_extern->setFontSize(12);
	m_extern->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_extern, &PushButton::clicked, this, &MessageTestLinux::onExternClicked);

	m_appointFilePath = new LineEdit(this);
	m_appointFilePath->setText(QStringLiteral("/tmp/123.txt"));
	m_appointFilePath->resize(180, 28);
	m_appointFilePath->setFontSize(12);

	m_showTextParam = new CheckBox(this);
	m_showTextParam->setText(QStringLiteral("显示参数"));
	m_showTextParam->resize(100, 20);
	m_showTextParam->setFontSize(12);
	m_showTextParam->setBackgroundColor(QColor(0, 0, 0, 0));
	QObject::connect(m_showTextParam, &CheckBox::stateChanged, this, &MessageTestLinux::onShowTextParamStateChanged);

	m_filterText = new LineEdit(this);
	m_filterText->resize(192, 20);
	m_filterText->setFontSize(12);
	m_filterText->setPlaceholderText(QStringLiteral("输入部分匹配文本"));
	QObject::connect(m_filterText, &LineEdit::textChanged, this, &MessageTestLinux::onFilterChanged);

	QObject::connect(&TypeManager::instance(), &TypeManager::receivePeopleId, this, &MessageTestLinux::onReceivePeopleId, Qt::QueuedConnection);
	QObject::connect(&TypeManager::instance(), &TypeManager::receivePid, this, &MessageTestLinux::onReceivePid, Qt::QueuedConnection);
	QObject::connect(&TypeManager::instance(), &TypeManager::receivePidThreadId, this, &MessageTestLinux::onReceivePidThreadId, Qt::QueuedConnection);
	QObject::connect(&TypeManager::instance(), &TypeManager::receiveLostPid, this, &MessageTestLinux::onReceiveLostPid, Qt::QueuedConnection);

	setWindowTitle("MessageTest1.1");

	QTimer* timer = new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onUpdateWindow()));
	timer->start(100);

	m_showStr.setFinite(m_textContentCount);

	CSystem::saveFile("123", "/tmp/MessageTestLinux.file");

	m_recoverThreadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<MessageRecoverTask> spMessageRecoverTask(new MessageRecoverTask);
	spMessageRecoverTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_recoverThreadId)->PostTask(spMessageRecoverTask);

	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<MessageReceiveTask> spMessageReceiveTask(new MessageReceiveTask);
	spMessageReceiveTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId)->PostTask(spMessageReceiveTask);

	//int iTitleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight); // 获取标题栏高度
	
	//m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	//QObject::connect(m_button, &COriginalButton::clicked, this, &MessageTestLinux::onButtonClicked);
}

bool MessageTestLinux::check()
{
	return true;
	return m_button != nullptr;
}

bool MessageTestLinux::textAvailable(const std::shared_ptr<TextMessage>& textMessage)
{
	{
		ReadLock lock(m_selectPeopleIdMutex);
		if (!m_selectPeopleId.empty())
		{
			bool isAvailable = false;
			for (auto itPeopleId = m_selectPeopleId.begin(); itPeopleId != m_selectPeopleId.end(); ++itPeopleId)
			{
				if (textMessage->m_peopleId == *itPeopleId)
				{
					isAvailable = true;
					break;
				}
			}
			if (!isAvailable)
			{
				return false;
			}
		}
	}
	
	{
		ReadLock lock(m_selectPidMutex);
		if (!m_selectPid.empty())
		{
			bool isAvailable = false;
			for (auto itPid = m_selectPid.begin(); itPid != m_selectPid.end(); ++itPid)
			{
				if (textMessage->m_pid == *itPid)
				{
					isAvailable = true;
					break;
				}
			}
			if (!isAvailable)
			{
				return false;
			}
		}
	}
	
	{
		ReadLock lock(m_selectThreadIdMutex);
		if (!m_selectThreadId.empty())
		{
			bool isAvailable = false;
			for (auto itThreadId = m_selectThreadId.begin(); itThreadId != m_selectThreadId.end(); ++itThreadId)
			{
				if (textMessage->m_threadId == *itThreadId)
				{
					isAvailable = true;
					break;
				}
			}
			if (!isAvailable)
			{
				return false;
			}
		}
	}

	{
		ReadLock lock(m_filterMutex);
		if (!m_filterStr.empty())
		{
			if (textMessage->m_text.find(m_filterStr) == std::string::npos)
			{
				return false;
			}
		}
	}

	return true;
}

int32_t MessageTestLinux::allToShow()
{
	return m_textStorage.allToShow(m_showStr, this);
}

std::string MessageTestLinux::textToString(const std::shared_ptr<TextMessage>& textMessage,
	bool isShowParam)
{
	return isShowParam ?
		("[" + std::to_string(textMessage->m_peopleId) +
		"][" + std::string(1, m_pidThreadIdMap[textMessage->m_pid].first) +
		"][" + std::to_string(m_pidThreadIdMap[textMessage->m_pid].second[textMessage->m_threadId]) +
		"]" + textMessage->m_text) : textMessage->m_text;
}

void MessageTestLinux::resizeEvent(QResizeEvent* eve)
{
	QDialog::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	//m_button->setGeometry(360, 120, 160, 80);
	int32_t windowWidth = width();
	int32_t windowHeight = height();
	
	m_textEdit->resize(windowWidth - 213 - 12 - m_externWidth, windowHeight - 12 * 2);
	m_area->move(windowWidth - 213 - m_externWidth, 28 + 16);
	m_screen->move(windowWidth - 213 - m_externWidth, 64 + 16);
	m_all->move(windowWidth - 213 - m_externWidth, 100 + 16);
	m_change->move(windowWidth - 213 + 16 - m_externWidth, 180);
	m_refresh->move(windowWidth - 213 + 114 - m_externWidth, 180);
	m_clear->move(windowWidth - 213 + 16 - m_externWidth, 244);
	m_extern->move(windowWidth - 15, 253);
	m_toFile->move(windowWidth - 213 + 16 - m_externWidth, 325);
	m_toAppointFile->move(windowWidth - 213 + 16 - m_externWidth, 405);
	m_appointFilePath->move(windowWidth - 213 + 17 - m_externWidth, 452);

	m_showTextParam->move(windowWidth - 313 + 16, 0 + 12);
	m_showTextParam->setVisible(m_externWidth != 0);
	m_filterText->move(windowWidth - 220 + 16, 0 + 12);
	m_filterText->setVisible(m_externWidth != 0);

	int32_t index = -1;
	for (auto itPeopleId = m_allPeopleIdCheckBox.begin(); itPeopleId != m_allPeopleIdCheckBox.end(); ++itPeopleId)
	{
		++index;
		(*itPeopleId)->move(windowWidth - 313 + 16, 28 + 16 + index * 36);
		(*itPeopleId)->setVisible(m_externWidth != 0);
	}

	index = -1;
	for (auto itThreadId = m_allThreadIdCheckBox.begin(); itThreadId != m_allThreadIdCheckBox.end(); ++itThreadId)
	{
		++index;
		(*itThreadId)->move(windowWidth - 313 + 16 + 50, 28 + 16 + index * 36);
		(*itThreadId)->setVisible(m_externWidth != 0);
	}

	index = -1;
	for (auto itPid = m_allPidCheckBox.begin(); itPid != m_allPidCheckBox.end(); ++itPid)
	{
		++index;
		(*itPid)->move(windowWidth - 313 + 16 + 100, 28 + 16 + index * 36);
		(*itPid)->setVisible(m_externWidth != 0);
	}
}

void MessageTestLinux::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	m_receiveThreadId = 0;

	{
		WriteLock lock(m_pidMutex);
    	for (auto it = m_pid.begin(); it != m_pid.end();)
    	{
    	    if (!CSystem::processName(*it).empty())
    	    {
    	        printf("end pid = %d\n", (int32_t)*it);
    	        CTaskThreadManager::Instance().Uninit(m_pidThreadId[*it]);
				m_pidThreadId.erase(m_pidThreadId.find(*it));
    	        it = m_pid.erase(it);
    	        continue;
    	    }
    	    ++it;
    	}
	}

	CTaskThreadManager::Instance().Uninit(m_addStringThreadId);
	m_addStringThreadId = 0;

	CTaskThreadManager::Instance().Uninit(m_recoverThreadId);
	m_recoverThreadId = 0;

	CSystem::deleteFile("/tmp/MessageTestLinux.file");

	//CTaskThreadManager::Instance().UninitAll();
	
	QDialog::closeEvent(eve);
}

void MessageTestLinux::onChangeClicked()
{
	m_isChange = !m_isChange;
	m_change->setText(m_isChange ? QStringLiteral("更改完毕") : QStringLiteral("更改屏幕"));
	m_textEdit->setEnabled(m_isChange);
	m_textEdit->setBorderColor(m_isChange ? QColor(23, 23, 23) : QColor(204, 204, 204));
	m_textEdit->setBackgroundColor(m_isChange ? QColor(255, 255, 255) : QColor(240, 240, 240));
	m_textEdit->ControlBorderForNormalHoverDisabled::setTextColor(m_isChange ? QColor(12, 12, 12) : QColor(100, 100, 100));
	m_textEdit->repaint();
	m_update = true;
}

void MessageTestLinux::onRefreshClicked()
{
	m_update = true;
}

void MessageTestLinux::onClearClicked()
{
	{
		WriteLock lock(m_selectPeopleIdMutex);
		m_selectPeopleId.clear();
	}
	
	{
		WriteLock lock(m_selectPidMutex);
		m_selectPid.clear();
	}

	{
		WriteLock lock(m_selectThreadIdMutex);
		m_selectThreadId.clear();
	}

	{
		WriteLock lock1(m_paramMutex);
		WriteLock lock2(m_showMutex);
		WriteLock lock3(m_allStringMutex);
		
		m_allPeopleId.clear();
		m_pidThreadIdReceiveMap.clear();

		for (auto itPeopleId = m_allPeopleIdCheckBox.begin(); itPeopleId != m_allPeopleIdCheckBox.end(); ++itPeopleId)
		{
			(*itPeopleId)->blockSignals(true);
			(*itPeopleId)->setParent(nullptr);
			delete *itPeopleId;
		}
		m_allPeopleIdCheckBox.clear();
	
		for (auto itPid = m_allPidCheckBox.begin(); itPid != m_allPidCheckBox.end(); ++itPid)
		{
			(*itPid)->blockSignals(true);
			(*itPid)->setParent(nullptr);
			(*itPid)->setUserData(Qt::UserRole, nullptr);
			delete *itPid;
		}
		m_allPidCheckBox.clear();
	
		for (auto itThreadId = m_allThreadIdCheckBox.begin(); itThreadId != m_allThreadIdCheckBox.end(); ++itThreadId)
		{
			(*itThreadId)->blockSignals(true);
			(*itThreadId)->setParent(nullptr);
			delete *itThreadId;
		}
		m_allThreadIdCheckBox.clear();

		m_showStr.clear();
		m_screenCount = 0;
		m_textStorage.clear();
		int32_t subCount = m_allCount;
		m_areaCount.fetch_sub(subCount);
		m_allCount.fetch_sub(subCount);
		m_processNameId = m_processNameIdInit;
		m_assignThreadId = 0;
		m_pidThreadIdMap.clear();
		m_isClear = true;
	}
	
	m_update = true;
}

void MessageTestLinux::onToFileClicked()
{

}

void MessageTestLinux::onToAppointFileClicked()
{
	std::string appointFile = m_appointFilePath->text().toStdString();
	std::ofstream txt(appointFile, std::ios::trunc);
	{
		ReadLock lock(m_allStringMutex);
		std::vector<std::string> vecString = m_textStorage.allToString(true, this);
		int32_t index = vecString.size();
		while (index-- != 0)
		{
			txt << vecString[index];
		}
	}
	
	int32_t result = system(("gedit " + appointFile).c_str());
	result = result;
}

void MessageTestLinux::onExternClicked()
{
	if (m_externWidth == 0)
	{
		m_externWidth = 300;
		setFixedSize(QSize(width() + 300, height()));
		m_extern->setText(QStringLiteral("《"));
	}
	else
	{
		m_externWidth = 0;
		setFixedSize(QSize(width() - 300, height()));
		m_extern->setText(QStringLiteral("》"));
	}
}

void MessageTestLinux::onUpdateWindow()
{
	if (!m_update)
	{
		return;
	}

	bool isShowParam = m_showTextParam->isChecked();
	if (m_isChange)
	{
		QString allText;
		{
			ReadLock lock(m_allStringMutex);
			int32_t allSize = 0;
			std::vector<std::string> vecString = m_textStorage.allToString(isShowParam, this);
			int32_t index = -1;
			while (index++ != vecString.size() - 1)
			{
				allSize += vecString[index].size();
			}
			allText.reserve(allSize);
			index = vecString.size();
			while (index-- != 0)
			{
				allText.append(QString::fromStdString(std::move(vecString[index])));
			}
			m_update = false;
		}
		if (!allText.isEmpty())
		{
			allText.chop(1);
		}
		m_textEdit->setText(allText);
	}
	else
	{
		QString showText;
		{
			ReadLock lock(m_showMutex);
			for (auto it = m_showStr.begin(); it != m_showStr.end(); ++it)
			{
				showText.append(textToString(*it, isShowParam).c_str());
			}
			m_update = false;
		}

		if (!showText.isEmpty())
		{
			showText.chop(1);
		}
		m_textEdit->setText(showText);
	}
	m_area->setText(QStringLiteral("当前缓冲区剩余展示量：") + QString::fromStdString(std::to_string(m_areaCount - m_allCount)));
	m_screen->setText(QStringLiteral("当前屏幕容量：") + QString::fromStdString(std::to_string(m_screenCount)));
	m_all->setText(QStringLiteral("总容量：") + QString::fromStdString(std::to_string(m_allCount)));
}

void MessageTestLinux::onReceivePeopleId(int32_t peopleId)
{
	CheckBox* box = new CheckBox(this);
	box->setText(std::to_string(peopleId).c_str());
	box->setFontSize(12);
	box->resize(45, 20);
	box->setBackgroundColor(QColor(0, 0, 0, 0));
	box->repaint();
	QObject::connect(box, &CheckBox::stateChanged, this, &MessageTestLinux::onStateChanged);
	m_allPeopleIdCheckBox.push_back(box);
	resizeEvent(nullptr);
}

void MessageTestLinux::onReceivePid(int32_t pid)
{
	CheckBox* box = new CheckBox(this);
	std::string processName = CSystem::processName(pid);
	box->setText((std::string(1, ++m_processNameId) + "*  | " + processName).c_str());
	box->setUserData(Qt::UserRole, (QObjectUserData*)pid);
	box->setToolTip(std::to_string(pid).c_str());
	box->setFontSize(12);
	box->resize(180, 20);
	box->setBackgroundColor(QColor(0, 0, 0, 0));
	box->repaint();
	QObject::connect(box, &CheckBox::stateChanged, this, &MessageTestLinux::onStateChanged);
	m_allPidCheckBox.push_back(box);
	m_pidThreadIdMap[pid].first = m_processNameId;
	resizeEvent(nullptr);
}

void MessageTestLinux::onReceivePidThreadId(int32_t pid, int32_t threadId)
{
	CheckBox* box = new CheckBox(this);
	box->setText((std::to_string(++m_assignThreadId) + "*").c_str());
	box->setFontSize(12);
	box->setToolTip(std::to_string(threadId).c_str());
	box->resize(50, 20);
	box->setBackgroundColor(QColor(0, 0, 0, 0));
	box->repaint();
	QObject::connect(box, &CheckBox::stateChanged, this, &MessageTestLinux::onStateChanged);
	m_allThreadIdCheckBox.push_back(box);
	resizeEvent(nullptr);

	m_pidThreadIdMap[pid].second[threadId] = m_assignThreadId;
}

void MessageTestLinux::onReceiveLostPid(int32_t pid)
{
	char processName = m_pidThreadIdMap[pid].first;
	for (auto it = m_allPidCheckBox.begin(); it != m_allPidCheckBox.end(); ++it)
	{
		if ((*it)->text().toStdString().substr(0, 2) == (std::string(1, processName) + "*"))
		{
			std::string text = (*it)->text().toStdString();
			text.erase(++text.begin());
			(*it)->setText(text.c_str());
		}
	}

	auto& threadIdMap = m_pidThreadIdMap[pid].second;
	for (auto it = m_allThreadIdCheckBox.begin(); it != m_allThreadIdCheckBox.end(); ++it)
	{
		for (auto itAssignThreadId = threadIdMap.begin(); itAssignThreadId != threadIdMap.end(); ++itAssignThreadId)
		{
			if ((*it)->text().toStdString() == (std::to_string(itAssignThreadId->second) + "*"))
			{
				(*it)->setText(std::to_string(itAssignThreadId->second).c_str());
				break;
			}
		}
	}
}

void MessageTestLinux::onStateChanged(int32_t state)
{
	auto beginTime = CSystem::GetHighTickCount();
	CheckBox* box = qobject_cast<CheckBox*>(sender());
	if (box == nullptr)
	{
		return;
	}

	int32_t peopleId = 0;
	bool isPeopleId = false;
	{
		if (std::find(m_allPeopleIdCheckBox.begin(), m_allPeopleIdCheckBox.end(), box) != m_allPeopleIdCheckBox.end())
		{
			peopleId = atoi(box->text().toStdString().c_str());
			isPeopleId = true;
		}
	}
	
	if (isPeopleId)
	{
		WriteLock lock(m_selectPeopleIdMutex);
		if (state == Qt::CheckState::Checked)
		{
			m_selectPeopleId.insert(peopleId);
		}
		else if (state == Qt::CheckState::Unchecked)
		{
			auto itPeopleId = m_selectPeopleId.find(peopleId);
			if (itPeopleId != m_selectPeopleId.end())
			{
				m_selectPeopleId.erase(itPeopleId);
			}
		}
	}

	int32_t pid = 0;
	bool isPid = false;
	{
		if (std::find(m_allPidCheckBox.begin(), m_allPidCheckBox.end(), box) != m_allPidCheckBox.end())
		{
			pid = (int32_t)box->userData(Qt::UserRole);
			isPid = true;
		}
	}
	
	if (isPid)
	{
		WriteLock lock(m_selectPidMutex);
		if (state == Qt::CheckState::Checked)
		{
			m_selectPid.insert(pid);
		}
		else if (state == Qt::CheckState::Unchecked)
		{
			auto itPid = m_selectPid.find(pid);
			if (itPid != m_selectPid.end())
			{
				m_selectPid.erase(itPid);
			}
		}
	}

	int32_t threadId = 0;
	bool isThreadId = false;
	{
		if (std::find(m_allThreadIdCheckBox.begin(), m_allThreadIdCheckBox.end(), box) != m_allThreadIdCheckBox.end())
		{
			threadId = atoi(box->toolTip().toStdString().c_str());
			isThreadId = true;
		}
	}
	
	if (isThreadId)
	{
		WriteLock lock(m_selectThreadIdMutex);
		if (state == Qt::CheckState::Checked)
		{
			m_selectThreadId.insert(threadId);
		}
		else if (state == Qt::CheckState::Unchecked)
		{
			auto itThreadId = m_selectThreadId.find(threadId);
			if (itThreadId != m_selectThreadId.end())
			{
				m_selectThreadId.erase(itThreadId);
			}
		}
	}

	{
		ReadLock lock1(m_showMutex);
		ReadLock lock2(m_allStringMutex);
		//auto beginTime = CSystem::GetHighTickCount();
		m_screenCount = allToShow();
		//printf("time = %d\n", CSystem::GetHighTickCountMilliRunTime(beginTime));
		m_isChangeState = true;
		m_update = true;
	}
}

void MessageTestLinux::onShowTextParamStateChanged(int32_t state)
{
	m_update = true;
}

void MessageTestLinux::onFilterChanged(const QString& text)
{
	if (m_filterTimer != nullptr)
	{
		delete m_filterTimer;
	}
	m_filterTimer = new QTimer(this);
	QObject::connect(m_filterTimer, SIGNAL(timeout()), this, SLOT(onFilterTimer()));
	m_filterTimer->start(500);
}

void MessageTestLinux::onFilterTimer()
{
	{
		WriteLock lock(m_filterMutex);
		m_filterStr = m_filterText->text().toStdString();
	}

	{
		ReadLock lock1(m_showMutex);
		ReadLock lock2(m_allStringMutex);
		m_screenCount = allToShow();
		m_isChangeState = true;
		m_update = true;
	}

	delete m_filterTimer;
	m_filterTimer = nullptr;
}

void MessageTestLinux::onButtonClicked()
{
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = QStringLiteral("点击");
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}