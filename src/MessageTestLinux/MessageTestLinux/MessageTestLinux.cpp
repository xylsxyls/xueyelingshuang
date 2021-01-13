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

MessageTestLinux::MessageTestLinux(QWidget* parent):
	QDialog(parent),
	m_button(nullptr),
	m_textEdit(nullptr),
	m_area(nullptr),
	m_screen(nullptr),
	m_areaCount(0),
	m_screenCount(0),
	m_change(nullptr),
	m_refresh(nullptr),
	m_clear(nullptr),
	m_toFile(nullptr),
	m_toAppointFile(nullptr),
	m_appointFilePath(nullptr),
	m_receiveThreadId(0),
	m_addStringThreadId(0),
	m_recoverThreadId(0),
	m_msg(nullptr),
	m_isChange(false),
	m_update(false)
{
	init();
}

MessageTestLinux::~MessageTestLinux()
{

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
	QTextBlockFormat blockFormat;
	blockFormat.setLineHeight(1, QTextBlockFormat::LineDistanceHeight);
	auto textCursor = m_textEdit->textCursor();
	textCursor.setBlockFormat(blockFormat);
	m_textEdit->setTextCursor(textCursor);

	m_area = new Label(this);
	m_area->setText(QStringLiteral("当前缓冲区剩余展示量：") + QString::fromStdString(std::to_string(m_areaCount)));
	m_area->resize(213, 18);
	m_area->setFontSize(12);
	m_area->setTextColor(QColor(0, 0, 0));

	m_screen = new Label(this);
	m_screen->setText(QStringLiteral("当前屏幕容量：") + QString::fromStdString(std::to_string(m_screenCount)));
	m_screen->resize(213, 18);
	m_screen->setFontSize(12);
	m_screen->setTextColor(QColor(0, 0, 0));

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
	QObject::connect(m_clear, &PushButton::clicked, this, &MessageTestLinux::onToFileClicked);

	m_toAppointFile = new PushButton(this);
	m_toAppointFile->setText(QStringLiteral("将屏幕中的内容写入指定文件"));
	m_toAppointFile->resize(182, 30);
	m_toAppointFile->setFontSize(12);
	m_toAppointFile->setFocusPolicy(Qt::NoFocus);
	QObject::connect(m_clear, &PushButton::clicked, this, &MessageTestLinux::onToAppointFileClicked);

	m_appointFilePath = new LineEdit(this);
	m_appointFilePath->setText(QStringLiteral("/tmp/123.txt"));
	m_appointFilePath->resize(180, 28);
	m_appointFilePath->setFontSize(12);

	setWindowTitle("MessageTest1.0");

	QTimer *timer = new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(updateWindow()));
	timer->start(100);

	m_showStr.setFinite(32);

	m_listAllStr.push_front(QString());

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
	m_textEdit->resize(windowWidth - 213 - 12, windowHeight - 12 * 2);
	m_area->move(windowWidth - 213, 28 + 12);
	m_screen->move(windowWidth - 213, 60 + 12);
	m_change->move(windowWidth - 213 + 16, 180);
	m_refresh->move(windowWidth - 213 + 114, 180);
	m_clear->move(windowWidth - 213 + 16, 244);
	m_toFile->move(windowWidth - 213 + 16, 325);
	m_toAppointFile->move(windowWidth - 213 + 16, 405);
	m_appointFilePath->move(windowWidth - 213 + 17, 452);
}

void MessageTestLinux::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	m_receiveThreadId = 0;

	{
		std::unique_lock<std::mutex> lock(m_pidMutex);
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
	m_textEdit->repaint();
	m_update = true;
}

void MessageTestLinux::onRefreshClicked()
{

}

void MessageTestLinux::onClearClicked()
{
	{
		std::unique_lock<std::mutex> lock(m_showMutex);
		m_showStr.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_addStringMutex);
		m_listAllStr.clear();
		m_listAllStr.push_front(QString());
		m_screenCount = 0;
	}
	m_update = true;
}

void MessageTestLinux::onToFileClicked()
{

}

void MessageTestLinux::onToAppointFileClicked()
{

}

void MessageTestLinux::updateWindow()
{
	if (!m_update)
	{
		return;
	}
	if (m_isChange)
	{
		std::unique_lock<std::mutex> lock(m_addStringMutex);
		QString allText;
		for (auto it = m_listAllStr.begin(); it != m_listAllStr.end(); ++it)
		{
			allText += *it;
		}
		m_textEdit->setText(allText);
	}
	else
	{
		std::unique_lock<std::mutex> lock(m_showMutex);
		std::string show;
		for (auto it = m_showStr.begin(); it != m_showStr.end(); ++it)
		{
			show += *it + "\n";
		}
		if (!show.empty())
		{
			show.pop_back();
		}
		m_textEdit->setText(QString::fromStdString(show));
	}
	m_screen->setText(QStringLiteral("当前屏幕容量：") + QString::fromStdString(std::to_string(m_screenCount)));
	m_update = false;
}

void MessageTestLinux::onButtonClicked()
{
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = QStringLiteral("点击");
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}