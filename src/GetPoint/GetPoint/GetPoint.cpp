#include "GetPoint.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/TextEdit.h"
#include "QtControls/LineEdit.h"
#include "CHook/CHookAPI.h"
#include <windows.h>
#include <QCursor>
#include <atomic>
#include "CSystem/CSystemAPI.h"

std::atomic<bool> g_get1 = false;
std::atomic<bool> g_get2 = false;
std::atomic<int32_t> g_x = 0;
std::atomic<int32_t> g_y = 0;
std::atomic<int32_t> g_x1 = 0;
std::atomic<int32_t> g_y1 = 0;
std::atomic<int32_t> g_x2 = 0;
std::atomic<int32_t> g_y2 = 0;
HWND g_hWnd = nullptr;

LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	g_x = QCursor::pos().x();
	g_y = QCursor::pos().y();
	if (wParam == 513 || wParam == 516)
	{
		if (g_get1)
		{
			g_get1 = false;
			g_x1 = (int32_t)g_x;
			g_y1 = (int32_t)g_y;
			CSystem::setClipboardData(g_hWnd, std::to_string(g_x1) + ", " + std::to_string(g_y1));
		}
		if (g_get2)
		{
			g_get2 = false;
			g_x2 = (int32_t)g_x;
			g_y2 = (int32_t)g_y;
			CSystem::setClipboardData(g_hWnd, std::to_string(g_x1) + ", " +
				std::to_string(g_y1) + ", " +
				std::to_string(g_x2 - g_x1) + ", " +
				std::to_string(g_y2 - g_y1));
		}
	}

	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

GetPoint::GetPoint(QWidget* parent):
	m_point1(nullptr),
	m_text1(nullptr),
	m_text2(nullptr),
	m_point2(nullptr)
{
	//ui.setupUi(this);
	//m_button = new COriginalButton(this);
	init();
}

GetPoint::~GetPoint()
{

}

void GetPoint::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	//m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	//QObject::connect(m_button, &COriginalButton::clicked, this, &GetPoint::onButtonClicked);

	m_point1 = new COriginalButton(this);
	m_point2 = new COriginalButton(this);
	m_text1 = new LineEdit(this);
	m_text2 = new LineEdit(this);

	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	move(20, 20);
	int32_t windowWidth = 20 + 70 + 10 + 150 + 20;
	int32_t windowHeight = 20 + 30 + 10 + 30 + 20;
	setFixedSize(windowWidth, windowHeight);
	setCustomerTitleBarHeight(windowHeight);

	m_point1->setText(QStringLiteral("左上"));
	m_point1->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_point1, &COriginalButton::clicked, this, &GetPoint::onPoint1ButtonClicked);

	m_point2->setText(QStringLiteral("宽高"));
	m_point2->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_point2, &COriginalButton::clicked, this, &GetPoint::onPoint2ButtonClicked);

	g_hWnd = (HWND)winId();

	CHook::Init(WH_MOUSE_LL, HookFun);

	QTimer *timer = new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(updateModelWindow()));
	timer->start(1);
}

bool GetPoint::check()
{
	return true;
}

void GetPoint::resizeEvent(QResizeEvent* eve)
{
	DialogShow::resizeEvent(eve);

	if (!check())
	{
		return;
	}

	m_point1->setGeometry(20, 20, 70, 30);
	m_point2->setGeometry(20, 60, 70, 30);

	m_text1->setGeometry(100, 20, 150, 30);
	m_text2->setGeometry(100, 60, 150, 30);
}

void GetPoint::closeEvent(QCloseEvent* eve)
{
	CHook::Uninit();
	DialogShow::closeEvent(eve);
}

void GetPoint::onPoint1ButtonClicked()
{
	g_get1 = true;
	g_x1 = 0;
	g_y1 = 0;
}

void GetPoint::onPoint2ButtonClicked()
{
	g_get2 = true;
	g_x2 = 0;
	g_y2 = 0;
}

void GetPoint::updateModelWindow()
{
	if (g_get1)
	{
		m_text1->setText((std::to_string(g_x) + ", " + std::to_string(g_y)).c_str());
	}
	else if (g_x1 != 0 && g_y1 != 0)
	{
		m_text1->setText((std::to_string(g_x1) + ", " + std::to_string(g_y1)).c_str());
	}
	if (g_get2)
	{
		m_text2->setText((std::to_string(g_x1) + ", " +
			std::to_string(g_y1) + ", " +
			std::to_string(g_x - g_x1) + ", " +
			std::to_string(g_y - g_y1)).c_str());
	}
	else if (g_x2 != 0 && g_y2 != 0)
	{
		m_text2->setText((std::to_string(g_x1) + ", " +
			std::to_string(g_y1) + ", " +
			std::to_string(g_x2 - g_x1) + ", " +
			std::to_string(g_y2 - g_y1)).c_str());
	}
}