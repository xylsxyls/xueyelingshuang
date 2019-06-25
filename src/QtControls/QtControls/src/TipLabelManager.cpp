#include "TipLabelManager.h"
#include "TipLabel.h"
#include "core/coreAPI.h"

TipLabelManager& TipLabelManager::instance()
{
	static TipLabelManager tipLabelManager;
	return tipLabelManager;
}

void TipLabelManager::popTip(const QPoint& pos, const QString& text, qint32 maxWidth, qint32 timeOut, bool hasFocus)
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
	m_tipLabel->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_tipLabel->show();
}

void TipLabelManager::close()
{
	emit closeTip();
}

TipLabelManager::TipLabelManager()
{
	init();
}

TipLabelManager::~TipLabelManager()
{
	SafeDelete(m_tipLabel);
}

void TipLabelManager::init()
{
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