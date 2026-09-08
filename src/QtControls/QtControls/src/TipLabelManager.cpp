#include "TipLabelManager.h"
#include "TipLabel.h"
#include "CSystem/CSystemAPI.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>

TipLabelManager& TipLabelManager::instance()
{
	static TipLabelManager tipLabelManager;
	return tipLabelManager;
}

void TipLabelManager::popTip(const QPoint& pos, const QString& text, qint32 maxWidth, qint32 timeOut, bool hasFocus)
{
	QCoreApplication* app = QCoreApplication::instance();
	if (app == nullptr)
	{
		return;
	}
	if (app != nullptr && QThread::currentThread() != app->thread())
	{
		QMetaObject::invokeMethod(this,
								  "popTipInGuiThread",
								  Qt::QueuedConnection,
								  Q_ARG(QPoint, pos),
								  Q_ARG(QString, text),
								  Q_ARG(int, static_cast<int>(maxWidth)),
								  Q_ARG(int, static_cast<int>(timeOut)),
								  Q_ARG(bool, hasFocus));
		return;
	}
	popTipInGuiThread(pos, text, maxWidth, timeOut, hasFocus);
}

void TipLabelManager::close()
{
	emit closeTip();
}

void TipLabelManager::popTipInGuiThread(const QPoint& pos, const QString& text, int maxWidth, int timeOut, bool hasFocus)
{
	if (m_tipLabel == nullptr)
	{
		init();
		if (m_tipLabel == nullptr)
		{
			return;
		}
	}
	m_tipLabel->close();
	m_tipLabel->setBottomRight(pos);
	m_tipLabel->setText(text);
	m_tipLabel->setMaxWidth(maxWidth);
	m_tipLabel->setExistTime(timeOut);
	m_tipLabel->setHasFocus(hasFocus);
	m_tipLabel->setFontFace(QStringLiteral("微软雅黑"));
	m_tipLabel->show();
}

TipLabelManager::TipLabelManager():
m_tipLabel(nullptr)
{
	QCoreApplication* app = QCoreApplication::instance();
	if (app != nullptr && QThread::currentThread() != app->thread())
	{
		moveToThread(app->thread());
	}
	qAddPostRoutine(TipLabelManager::destroyTipLabelForApplicationExit);
}

TipLabelManager::~TipLabelManager()
{
	destroyTipLabel();
}

void TipLabelManager::init()
{
	QCoreApplication* app = QCoreApplication::instance();
	if (app == nullptr || QThread::currentThread() != app->thread())
	{
		return;
	}
	m_tipLabel = new TipLabel(nullptr);
	if (m_tipLabel == nullptr)
	{
		return;
	}
	m_tipLabel->setBorderRadius(2);
	m_tipLabel->setBorderWidth(1);
	m_tipLabel->setBorderColor(QColor(118, 118, 118, 255));
	m_tipLabel->setTextColor(QColor(87, 87, 87, 255));
	m_tipLabel->setBackgroundColor(QColor(243, 244, 248, 255));
	QObject::connect(this, &TipLabelManager::closeTip, m_tipLabel, &TipLabel::close);
}

void TipLabelManager::destroyTipLabel()
{
	if (m_tipLabel == nullptr)
	{
		return;
	}

	QCoreApplication* app = QCoreApplication::instance();
	if (app != nullptr && QThread::currentThread() != app->thread())
	{
		TipLabel* tipLabel = m_tipLabel;
		m_tipLabel = nullptr;
		tipLabel->deleteLater();
		return;
	}

	delete m_tipLabel;
	m_tipLabel = nullptr;
}

void TipLabelManager::destroyTipLabelForApplicationExit()
{
	TipLabelManager::instance().destroyTipLabel();
}