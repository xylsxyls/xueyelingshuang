#include "PerformanceTest.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/Label.h"
#include "DialogManager/DialogManagerAPI.h"
#include <QProcess>
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "PerformanceManager.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CpuTask.h"
#include "MemoryTask.h"
#include <QTimer>
#include "Ctxt/CtxtAPI.h"

PerformanceTest::PerformanceTest(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	m_process = new Label(this);
	m_timer = new QTimer(this);
	m_writeTimer = new QTimer(this);
	m_writeLogButton = new COriginalButton(this);
	init();
}

PerformanceTest::~PerformanceTest()
{

}

void PerformanceTest::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &PerformanceTest::onButtonClicked);
	m_button->setText(QStringLiteral("添加进程"));

	m_writeLogButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_writeLogButton, &COriginalButton::clicked, this, &PerformanceTest::onWriteLogButtonClicked);
	m_writeLogButton->setText(QStringLiteral("写入文件"));

	//m_process->setBackgroundColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255));
	m_process->setTextColor(QColor(0, 255, 0, 255));
	m_process->setFontSize(15);
	m_process->setWordWrap(true);

	m_currentPath = CSystem::GetCurrentExePath();

	QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(onUpdatePerformanceWindow()));
	m_timer->start(100);

	QObject::connect(m_writeTimer, SIGNAL(timeout()), this, SLOT(onWriteLog()));
}

bool PerformanceTest::check()
{
	return true;
	return m_button != nullptr;
}

void PerformanceTest::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_button->setGeometry(485, 157, 100, 50);
	m_writeLogButton->setGeometry(485, 237, 100, 50);
	m_process->setGeometry(20, 20, 450, 370);
}

void PerformanceTest::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().UninitAll();
	QMainWindow::closeEvent(eve);
}

void PerformanceTest::onButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("请输入要检测的进程");
	inputDialogParam.m_parent = windowHandle();
	InputEx input;
	input.m_tip = QStringLiteral("进程Pid");
	inputDialogParam.m_vecInputEx.push_back(input);
	input.m_tip = QStringLiteral("是否检测CPU");
	input.m_defaultText = "1";
	inputDialogParam.m_vecInputEx.push_back(input);
	input.m_tip = QStringLiteral("是否检测内存");
	input.m_defaultText = "1";
	inputDialogParam.m_vecInputEx.push_back(input);
	DialogManager::instance().makeDialog(inputDialogParam);

	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	uint32_t pid = atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str());
	bool checkCpu = (inputDialogParam.m_vecInputEx[1].m_editText == "1");
	bool checkMemory = (inputDialogParam.m_vecInputEx[2].m_editText == "1");

	PerformanceManager::instance().addPerformance(pid, checkCpu, checkMemory);

	if (checkCpu)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		m_vecThread.push_back(threadId);
		std::shared_ptr<CpuTask> spCpuTask(new CpuTask);
		spCpuTask->setParam(pid);
		CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spCpuTask);
	}
	
	if (checkMemory)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		m_vecThread.push_back(threadId);
		std::shared_ptr<MemoryTask> spMemoryTask(new MemoryTask);
		spMemoryTask->setParam(pid);
		CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spMemoryTask);
	}
}

void PerformanceTest::onUpdatePerformanceWindow()
{
	m_process->setText(QString::fromLocal8Bit(PerformanceManager::instance().performanceWindowString().c_str()));
}

void PerformanceTest::onWriteLog()
{
	auto performanceMap = PerformanceManager::instance().performance();
	for (auto it = performanceMap.begin(); it != performanceMap.end(); ++it)
	{
		if (it->second[0] != "nil")
		{
			Ctxt cpuTxt(m_currentPath + std::to_string(it->first) + "Cpu.txt");
			cpuTxt.AddLineWithoutOpenFile("%s\t%s", PerformanceManager::instance().currentTime().c_str(), it->second[0].c_str());
		}
		if (it->second[1] != "nil")
		{
			Ctxt memoryTxt(m_currentPath + std::to_string(it->first) + "Memory.txt");
			memoryTxt.AddLineWithoutOpenFile("%s\t%s", PerformanceManager::instance().currentTime().c_str(), it->second[1].c_str());
		}
	}
}

void PerformanceTest::onWriteLogButtonClicked()
{
	m_writeTimer->start(1000);

	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("写入成功");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}