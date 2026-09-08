#include "PushButton.h"
#include "CStringManager/CStringManagerAPI.h"
#include "QssString/QssStringAPI.h"
#include <QEvent>
#include <QHelpEvent>
#include <QPixmap>
#include <limits>

PushButton::PushButton(QWidget* parent) :
ControlShow(parent),
m_bkgImageStateCount(4),
m_bkgImageNormal(1),
m_bkgImageHover(2),
m_bkgImagePressed(3),
m_bkgImageDisabled(4),
m_bkgImageCKNormal(1),
m_bkgImageCKHover(2),
m_bkgImageCKPressed(3),
m_bkgImageCKDisabled(4),
m_fontColorNormal(255, 255, 255),
m_fontColorHover(255, 255, 255),
m_fontColorPressed(255, 255, 255),
m_fontColorDisabled(255, 255, 255),
m_fontColorCKNormal(255, 255, 255),
m_fontColorCKHover(255, 255, 255),
m_fontColorCKPressed(255, 255, 255),
m_fontColorCKDisabled(255, 255, 255),
m_fontFace(QStringLiteral("微软雅黑")),
m_fontSize(14),
m_bold(false),
m_italic(false),
m_underline(false),
m_borderWidth(0),
m_borderRadius(0),
m_borderStyle("solid"),
m_textAlign("center"),
m_leftMargin(0),
m_topMargin(0),
m_rightMargin(0),
m_bottomMargin(0),
m_bkgImageHMargin(0),
m_bkgImageVMargin(0),
m_toolTipOffset(0, 0),
m_clickBreathTime(0),
m_savedEnabled(true)
{
	ControlBase<QPushButton>::setControlShow(this);
	setText("");
	setToolTipDuration(2500);
	m_clickBreathTimer.setSingleShot(true);
	connect(this, &PushButton::clicked, this, &PushButton::onClicked);
	connect(&m_clickBreathTimer, &QTimer::timeout, this, &PushButton::onClickBreathTimerTimeout);
	updateCompatibleStyle();
}

PushButton::~PushButton()
{

}

void PushButton::setEnabled(bool enabled)
{
	m_savedEnabled = enabled;
	if (!m_clickBreathTimer.isActive())
	{
		QPushButton::setEnabled(enabled);
	}
}

void PushButton::setClickBreathTime(quint64 time)
{
	m_clickBreathTime = time;
	if (m_clickBreathTime == 0)
	{
		if (m_clickBreathTimer.isActive())
		{
			m_clickBreathTimer.stop();
			QPushButton::setEnabled(m_savedEnabled);
		}
		return;
	}

	quint64 maxInterval = static_cast<quint64>((std::numeric_limits<int>::max)());
	m_clickBreathTimer.setInterval(static_cast<int>(m_clickBreathTime > maxInterval ? maxInterval : m_clickBreathTime));
}

void PushButton::setBkgImage(const QString& bkgImage,
							 const quint64 bkgImageStateCount,
							 const quint64 bkgImageNormal,
							 const quint64 bkgImageHover,
							 const quint64 bkgImagePressed,
							 const quint64 bkgImageDisabled,
							 const quint64 bkgImageCKNormal,
							 const quint64 bkgImageCKHover,
							 const quint64 bkgImageCKPressed,
							 const quint64 bkgImageCKDisabled)
{
	m_bkgImage = bkgImage;
	// clampImageValue入参：value是外部传入的图片状态序号。
	// clampImageValue出参：无。
	// clampImageValue返回值：返回限制到qint32范围内的安全状态值。
	const quint64 maxImageValue = static_cast<quint64>((std::numeric_limits<qint32>::max)());
	auto clampImageValue = [maxImageValue](quint64 value) -> quint32
	{
		return static_cast<quint32>(value > maxImageValue ? maxImageValue : value);
	};
	m_bkgImageStateCount = clampImageValue(bkgImageStateCount);
	m_bkgImageNormal = clampImageValue(bkgImageNormal);
	m_bkgImageHover = clampImageValue(bkgImageHover);
	m_bkgImagePressed = clampImageValue(bkgImagePressed);
	m_bkgImageDisabled = clampImageValue(bkgImageDisabled);
	m_bkgImageCKNormal = clampImageValue(bkgImageCKNormal);
	m_bkgImageCKHover = clampImageValue(bkgImageCKHover);
	m_bkgImageCKPressed = clampImageValue(bkgImageCKPressed);
	m_bkgImageCKDisabled = clampImageValue(bkgImageCKDisabled);
	updateCompatibleStyle();
}

void PushButton::setBkgColor(const QColor& bkgColorNormal,
							 const QColor& bkgColorHover,
							 const QColor& bkgColorPressed,
							 const QColor& bkgColorDisabled,
							 const QColor& bkgColorCKNormal,
							 const QColor& bkgColorCKHover,
							 const QColor& bkgColorCKPressed,
							 const QColor& bkgColorCKDisabled)
{
	m_bkgColorNormal = bkgColorNormal;
	m_bkgColorHover = bkgColorHover;
	m_bkgColorPressed = bkgColorPressed;
	m_bkgColorDisabled = bkgColorDisabled;
	m_bkgColorCKNormal = bkgColorCKNormal;
	m_bkgColorCKHover = bkgColorCKHover;
	m_bkgColorCKPressed = bkgColorCKPressed;
	m_bkgColorCKDisabled = bkgColorCKDisabled;
	updateCompatibleStyle();
}

void PushButton::setFontColor(const QColor& fontColorNormal,
							  const QColor& fontColorHover,
							  const QColor& fontColorPressed,
							  const QColor& fontColorDisabled,
							  const QColor& fontColorCKNormal,
							  const QColor& fontColorCKHover,
							  const QColor& fontColorCKPressed,
							  const QColor& fontColorCKDisabled)
{
	m_fontColorNormal = fontColorNormal;
	m_fontColorHover = fontColorHover;
	m_fontColorPressed = fontColorPressed;
	m_fontColorDisabled = fontColorDisabled;
	m_fontColorCKNormal = fontColorCKNormal;
	m_fontColorCKHover = fontColorCKHover;
	m_fontColorCKPressed = fontColorCKPressed;
	m_fontColorCKDisabled = fontColorCKDisabled;
	updateCompatibleStyle();
}

void PushButton::setFontFace(const QString& value)
{
	m_fontFace = value;
	updateCompatibleStyle();
}

void PushButton::setFontSize(const quint32& value)
{
	m_fontSize = value;
	updateCompatibleStyle();
}

void PushButton::setBold(const bool& value)
{
	m_bold = value;
	updateCompatibleStyle();
}

void PushButton::setItalic(const bool& value)
{
	m_italic = value;
	updateCompatibleStyle();
}

void PushButton::setUnderline(const bool& value)
{
	m_underline = value;
	updateCompatibleStyle();
}

void PushButton::setBorderWidth(const quint32& value)
{
	m_borderWidth = value;
	updateCompatibleStyle();
}

void PushButton::setBorderRadius(const quint32& value)
{
	m_borderRadius = value;
	updateCompatibleStyle();
}

void PushButton::setBorderStyle(const QString& value)
{
	m_borderStyle = value;
	updateCompatibleStyle();
}

void PushButton::setBorderColor(const QColor& borderColorNormal,
								const QColor& borderColorHover,
								const QColor& borderColorPressed,
								const QColor& borderColorDisabled,
								const QColor& borderColorCKNormal,
								const QColor& borderColorCKHover,
								const QColor& borderColorCKPressed,
								const QColor& borderColorCKDisabled)
{
	m_borderColorNormal = borderColorNormal;
	m_borderColorHover = borderColorHover;
	m_borderColorPressed = borderColorPressed;
	m_borderColorDisabled = borderColorDisabled;
	m_borderColorCKNormal = borderColorCKNormal;
	m_borderColorCKHover = borderColorCKHover;
	m_borderColorCKPressed = borderColorCKPressed;
	m_borderColorCKDisabled = borderColorCKDisabled;
	updateCompatibleStyle();
}

void PushButton::setTextAlign(const QString& align)
{
	m_textAlign = align;
	updateCompatibleStyle();
}

void PushButton::setMargins(const quint32& left,
							const quint32& top,
							const quint32& right,
							const quint32& bottom)
{
	m_leftMargin = left;
	m_topMargin = top;
	m_rightMargin = right;
	m_bottomMargin = bottom;
	updateCompatibleStyle();
}

void PushButton::setBkgMargins(const quint32& hMargin, const quint32& vMargin)
{
	m_bkgImageHMargin = hMargin;
	m_bkgImageVMargin = vMargin;
	updateCompatibleStyle();
}

void PushButton::setToolTipOffset(int x, int y)
{
	m_toolTipOffset = QPoint(x, y);
}

bool PushButton::event(QEvent* eve)
{
	if (eve == nullptr)
	{
		return false;
	}
	if (eve->type() == QEvent::ToolTip)
	{
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(eve);
		QHelpEvent offsetHelpEvent(QEvent::ToolTip,
								   helpEvent->pos() + m_toolTipOffset,
								   helpEvent->globalPos() + m_toolTipOffset);
		return QPushButton::event(&offsetHelpEvent);
	}
	return QPushButton::event(eve);
}

void PushButton::onClicked()
{
	if (m_clickBreathTime == 0)
	{
		return;
	}

	QPushButton::setEnabled(false);
	if (m_clickBreathTimer.isActive())
	{
		m_clickBreathTimer.stop();
	}
	m_clickBreathTimer.start();
}

void PushButton::onClickBreathTimerTimeout()
{
	m_clickBreathTimer.stop();
	QPushButton::setEnabled(m_savedEnabled);
}

void PushButton::updateCompatibleStyle()
{
	initClassName();

	m_controlStyle.addClassName().AddKeyValue(L"font-family", CStringManager::Format(L"'%s'", m_fontFace.toStdWString().c_str()));
	m_controlStyle.addClassName().AddKeyValue(L"font-size", CStringManager::Format(L"%dpx", m_fontSize));
	m_controlStyle.addClassName().AddKeyValue(L"font-weight", m_bold ? L"bold" : L"normal");
	m_controlStyle.addClassName().AddKeyValue(L"font-style", m_italic ? L"italic" : L"normal");
	m_controlStyle.addClassName().AddKeyValue(L"text-decoration", m_underline ? L"underline" : L"none");
	m_controlStyle.addClassName().AddKeyValue(L"text-align", m_textAlign.toStdWString());
	m_controlStyle.addClassName().AddKeyValue(L"padding-left", CStringManager::Format(L"%dpx", m_leftMargin));
	m_controlStyle.addClassName().AddKeyValue(L"padding-top", CStringManager::Format(L"%dpx", m_topMargin));
	m_controlStyle.addClassName().AddKeyValue(L"padding-right", CStringManager::Format(L"%dpx", m_rightMargin));
	m_controlStyle.addClassName().AddKeyValue(L"padding-bottom", CStringManager::Format(L"%dpx", m_bottomMargin));
	m_controlStyle.addClassName().AddKeyValue(L"border-radius", CStringManager::Format(L"%dpx", m_borderRadius));
	m_controlStyle.addClassName()(L"focus").AddKeyValue(L"outline", L"none");

	updateStateStyle(L"enabled", L"", m_bkgImageNormal, m_bkgColorNormal, m_fontColorNormal, m_borderColorNormal);
	updateStateStyle(L"hover", L"", m_bkgImageHover, m_bkgColorHover, m_fontColorHover, m_borderColorHover);
	updateStateStyle(L"pressed", L"", m_bkgImagePressed, m_bkgColorPressed, m_fontColorPressed, m_borderColorPressed);
	updateStateStyle(L"disabled", L"", m_bkgImageDisabled, m_bkgColorDisabled, m_fontColorDisabled, m_borderColorDisabled);
	updateStateStyle(L"checked", L"enabled", m_bkgImageCKNormal, m_bkgColorCKNormal, m_fontColorCKNormal, m_borderColorCKNormal);
	updateStateStyle(L"checked", L"hover", m_bkgImageCKHover, m_bkgColorCKHover, m_fontColorCKHover, m_borderColorCKHover);
	updateStateStyle(L"checked", L"pressed", m_bkgImageCKPressed, m_bkgColorCKPressed, m_fontColorCKPressed, m_borderColorCKPressed);
	updateStateStyle(L"checked", L"disabled", m_bkgImageCKDisabled, m_bkgColorCKDisabled, m_fontColorCKDisabled, m_borderColorCKDisabled);

	ControlShow<QPushButton>::repaint();
}

void PushButton::updateStateStyle(const std::wstring& firstState,
								  const std::wstring& secondState,
								  quint32 imageIndex,
								  const QColor& backgroundColor,
								  const QColor& fontColor,
								  const QColor& borderColor)
{
	addStateStyleValue(firstState, secondState, L"border-image", borderImageValue(imageIndex));
	addStateStyleValue(firstState, secondState, L"background-color", QssHelper::QColorToWString(backgroundColor));
	addStateStyleValue(firstState, secondState, L"color", QssHelper::QColorToWString(fontColor));
	addStateStyleValue(firstState, secondState, L"border", borderValue(borderColor));
	addStateStyleValue(firstState, secondState, L"border-radius", CStringManager::Format(L"%dpx", m_borderRadius));
	addStateStyleValue(firstState, secondState, L"text-align", m_textAlign.toStdWString());
	addStateStyleValue(firstState, secondState, L"padding-left", CStringManager::Format(L"%dpx", m_leftMargin));
	addStateStyleValue(firstState, secondState, L"padding-top", CStringManager::Format(L"%dpx", m_topMargin));
	addStateStyleValue(firstState, secondState, L"padding-right", CStringManager::Format(L"%dpx", m_rightMargin));
	addStateStyleValue(firstState, secondState, L"padding-bottom", CStringManager::Format(L"%dpx", m_bottomMargin));
}

void PushButton::addStateStyleValue(const std::wstring& firstState,
									const std::wstring& secondState,
									const std::wstring& key,
									const std::wstring& value)
{
	QssString& style = m_controlStyle.addClassName();
	if (!firstState.empty())
	{
		style(firstState);
	}
	if (!secondState.empty())
	{
		style(secondState);
	}
	style.AddKeyValue(key, value);
}

std::wstring PushButton::borderImageValue(quint32 imageIndex) const
{
	if (m_bkgImage.isEmpty() || m_bkgImageStateCount == 0 || imageIndex == 0 || imageIndex > m_bkgImageStateCount)
	{
		return L"none";
	}

	QPixmap pixmap;
	if (!pixmap.load(m_bkgImage))
	{
		return L"none";
	}

	qint32 imageHeight = pixmap.height();
	if (imageHeight <= 0 || (imageHeight % m_bkgImageStateCount) != 0)
	{
		return L"none";
	}

	qint32 stepHeight = imageHeight / m_bkgImageStateCount;
	qint32 top = stepHeight * (imageIndex - 1) + m_bkgImageVMargin;
	qint32 bottom = imageHeight - stepHeight * imageIndex + m_bkgImageVMargin;
	QString imagePath = m_bkgImage;
	imagePath.replace("\\", "/");
	return CStringManager::Format(L"url(%s) %d %d %d %d stretch stretch",
								  imagePath.toStdWString().c_str(),
								  top,
								  m_bkgImageHMargin,
								  bottom,
								  m_bkgImageHMargin);
}

std::wstring PushButton::borderValue(const QColor& borderColor) const
{
	return CStringManager::Format(L"%dpx %s %s",
								  m_borderWidth,
								  m_borderStyle.toStdWString().c_str(),
								  QssHelper::QColorToWString(borderColor).c_str());
}