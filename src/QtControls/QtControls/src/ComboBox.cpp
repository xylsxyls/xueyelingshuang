#include "ComboBox.h"
#include "NoFocusFrameDelegate.h"
#include "ListWidget.h"
#include <QtWidgets/QScrollBar>
#include <QKeyEvent>
#include <QEvent>

ComboBox::ComboBox(QWidget* parent) :
ControlShow(parent),
m_listOrigin(0),
m_listWidget(nullptr),
m_dropDownImgNormal(1),
m_dropDownImgDisabled(4),
m_dropDownImgExpandNormal(5),
m_dropDownImgExpandDisabled(8),
m_dropDownImgStateCount(8),
m_hoverIndex(-1),
m_dropDownWidth(-1),
m_dropDownHeight(-1),
m_dropDownVisible(true),
m_dropDownBorderWidth(-1)
{
	ControlBase::setControlShow(this);
	setItemName(L"drop-down");
	m_listWidget = new ListWidget(this);
	init();
}

ComboBox::~ComboBox()
{
	
}

void ComboBox::setDefault()
{
	//下拉边框粗度设为0，因为QListWidget已有边框，此属性px无效
	m_controlStyle.addClassName()(SPACE, L"QAbstractItemView").AddKeyValue(L"border", L"none");
	setBorderWidth(1);
	setTextOrigin(0);
}

void ComboBox::setDropDownSize(qint32 width, qint32 height, bool rePaint)
{
	m_dropDownWidth = width;
	m_dropDownHeight = GetInt(height, width);
	if (m_dropDownVisible)
	{
		ControlBase::setPxValue(L"width", width, true, false);
		ControlBase::setPxValue(L"height", GetInt(height, width), true, rePaint);
	}
}

void ComboBox::setDropDownBorderWidth(qint32 width, bool rePaint)
{
	m_dropDownBorderWidth = width;
	if (m_dropDownVisible)
	{
		ControlBase::setPxSolidValue(L"border", width, true, rePaint);
	}
}

void ComboBox::setDropDownImage(const QString& dropDownImgPath,
								qint32 dropDownImgStateCount,
								qint32 dropDownImgNormal,
								qint32 dropDownImgHover,
								qint32 dropDownImgDisabled,
								qint32 dropDownImgExpandNormal,
								qint32 dropDownImgExpandDisabled,
								bool rePaint)
{
	std::map<qint32, std::map<qint32, qint32>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = dropDownImgNormal;
	imageStateMap[NORMAL][HOVER] = dropDownImgHover;
	imageStateMap[NORMAL][PRESSED] = dropDownImgExpandNormal;
	imageStateMap[NORMAL][DISABLED] = dropDownImgDisabled;
	std::wstring wstrImgPath = dropDownImgPath.toStdWString();

	m_imagePath = wstrImgPath;
	m_dropDownImgStateCount = dropDownImgStateCount;
	m_dropDownImgNormal = dropDownImgNormal;
	m_dropDownImgDisabled = dropDownImgDisabled;
	m_dropDownImgExpandNormal = dropDownImgExpandNormal;
	m_dropDownImgExpandDisabled = dropDownImgExpandDisabled;
	
	ControlBase::setImageStateMap(imageStateMap, wstrImgPath, dropDownImgStateCount, L"border-image", L"down-arrow", rePaint);
}

void ComboBox::setDropDownTopRightOrigin(qint32 topOrigin, qint32 rightOrigin, bool rePaint)
{
	ControlBase::setPxValue(L"margin-top", topOrigin, true, false);
	ControlBase::setPxValue(L"margin-right", GetInt(rightOrigin, topOrigin), true, rePaint);
}

void ComboBox::setListOrigin(qint32 origin)
{
	m_listOrigin = origin;
}

void ComboBox::addItem(const QString& text)
{
	if (!check())
	{
		return;
	}
	QListWidgetItem* widgetItem = new QListWidgetItem(m_listWidget);
	if (widgetItem == nullptr)
	{
		return;
	}
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
}

void ComboBox::addItems(const QStringList& textList)
{
	if (!check())
	{
		return;
	}
	qint32 index = -1;
	qint32 size = textList.size();
	while (index++ != size - 1)
	{
		QListWidgetItem* widgetItem = new QListWidgetItem(m_listWidget);
		if (widgetItem != nullptr)
		{
			widgetItem->setText(textList[index]);
			widgetItem->setToolTip(textList[index]);
		}
	}
}

void ComboBox::setItemText(qint32 index, const QString& text)
{
	QComboBox::setItemText(index, text);
	if (!check())
	{
		return;
	}
	QListWidgetItem* widgetItem = m_listWidget->item(index);
	if (widgetItem == nullptr)
	{
		return;
	}
	widgetItem->setToolTip(text);
}

void ComboBox::setListBackgroundColor(const QColor& color, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setBackgroundColor(color, rePaint);
}

void ComboBox::setListBorderWidth(qint32 width, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setBorderWidth(width, rePaint);
}

void ComboBox::setListBorderColor(const QColor& color, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setBorderColor(color, rePaint);
}

void ComboBox::setListItemBorderColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemBorderColor(normalColor, hoverColor, disabledColor, rePaint);
}

void ComboBox::setListItemBackgroundColor(const QColor& normalColor,
										  const QColor& hoverColor,
										  const QColor& disabledColor,
										  bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemBackgroundColor(normalColor, hoverColor, disabledColor, rePaint);
}

void ComboBox::setListItemBorderWidth(qint32 width, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemBorderWidth(width, rePaint);
}

void ComboBox::setListItemBorderImage(const QString& borderImgPath,
									qint32 borderImgStateCount,
									qint32 borderImgNormal,
									qint32 borderImgHover,
									qint32 borderImgDisabled,
									bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemBorderImage(borderImgPath, borderImgStateCount, borderImgNormal, borderImgHover, borderImgDisabled, rePaint);
}

void ComboBox::setListItemHeight(qint32 height, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemHeight(height, rePaint);
}

void ComboBox::setListTextColor(const QColor& normalColor,
								const QColor& hoverColor,
								const QColor& disabledColor,
								bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemTextColor(normalColor, hoverColor, disabledColor, rePaint);
}

void ComboBox::setListFontFace(const QString& fontName, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setFontFace(fontName, rePaint);
}

void ComboBox::setListFontSize(qint32 fontSize, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setFontSize(fontSize, rePaint);
}

void ComboBox::setListTextOrigin(qint32 origin, bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemTextOrigin(origin, rePaint);
}

void ComboBox::setListItemAroundOrigin(qint32 leftOrigin,
								 qint32 topOrigin,
								 qint32 rightOrigin,
								 qint32 bottomOrigin,
								 bool rePaint)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setItemAroundOrigin(leftOrigin, topOrigin, rightOrigin, bottomOrigin, rePaint);
}

void ComboBox::setListMaxHeight(qint32 maxHeight)
{
	QAbstractItemView* viewPtr = view();
	if (viewPtr == nullptr)
	{
		return;
	}
	QWidget* widget = viewPtr->parentWidget();
	if (widget == nullptr)
	{
		return;
	}
	widget->setStyleSheet(QString("max-height:%1px").arg(maxHeight));
}

void ComboBox::setDropDownVisible(bool enable, bool rePaint)
{
	m_dropDownVisible = enable;
	if (m_dropDownVisible == false)
	{
		ControlBase::setPxValue(L"min-width", 0, true, false);
		ControlBase::setPxValue(L"min-height", 0, true, false);
		ControlBase::setPxValue(L"max-width", 0, true, false);
		ControlBase::setPxValue(L"max-height", 0, true, false);
		ControlBase::setPxValue(L"width", 0, true, false);
		ControlBase::setPxValue(L"height", 0, true, false);
		ControlBase::setPxSolidValue(L"border", 1, true, rePaint);
	}
	else if (m_dropDownWidth != -1 && m_dropDownHeight != -1)
	{
		ControlBase::setPxValue(L"min-width", m_dropDownWidth, true, false);
		ControlBase::setPxValue(L"min-height", m_dropDownHeight, true, false);
		ControlBase::setPxValue(L"max-width", m_dropDownWidth, true, false);
		ControlBase::setPxValue(L"max-height", m_dropDownHeight, true, false);
		ControlBase::setPxValue(L"width", m_dropDownWidth, true, false);
		ControlBase::setPxValue(L"height", m_dropDownHeight, true, false);
		if (m_dropDownBorderWidth != -1)
		{
			ControlBase::setPxSolidValue(L"border", m_dropDownBorderWidth, true, rePaint);
		}
	}
}

void ComboBox::setSelectEnable(bool enable)
{
	if (!check())
	{
		return;
	}
	m_listWidget->setClickEnable(enable);
}

void ComboBox::showEvent(QShowEvent* eve)
{
	//这里重写repaint为了刷新下拉列表控件
	repaint();
	QComboBox::showEvent(eve);
}

void ComboBox::mouseMoveEvent(QMouseEvent* eve)
{
	setToolTip(currentText());
	QComboBox::mouseMoveEvent(eve);
}

void ComboBox::showPopup()
{
	if (count() == 0)
	{
		hidePopup();
		return;
	}
	m_imageStateMap[NORMAL][NORMAL] = m_dropDownImgExpandNormal;
	m_imageStateMap[NORMAL][DISABLED] = m_dropDownImgExpandDisabled;
	ControlBase::setImageStateMap(m_imageStateMap, m_imagePath, m_dropDownImgStateCount, L"border-image", L"down-arrow", true);
	if (m_listOrigin == 0)
	{
		QComboBox::showPopup();
		return;
	}
	QRect rect = geometry();
	QRect moveRect = rect;
	moveRect.setTop(rect.top() + m_listOrigin);
	move(moveRect.topLeft());
	QComboBox::showPopup();
	move(rect.topLeft());
	return;
}

void ComboBox::hidePopup()
{
	m_imageStateMap[NORMAL][NORMAL] = m_dropDownImgNormal;
	m_imageStateMap[NORMAL][DISABLED] = m_dropDownImgDisabled;
	ControlBase::setImageStateMap(m_imageStateMap, m_imagePath, m_dropDownImgStateCount, L"border-image", L"down-arrow", true);
	QComboBox::hidePopup();
	QEvent eve(QEvent::HoverLeave);
	event(&eve);
}

void ComboBox::keyPressEvent(QKeyEvent* eve)
{
	if (eve == nullptr)
	{
		QComboBox::keyPressEvent(eve);
		return;
	}
	if (eve->key() == Qt::Key_Enter || eve->key() == Qt::Key_Return)
	{
		return;
	}
	QComboBox::keyPressEvent(eve);
}

bool ComboBox::check()
{
	return m_listWidget != nullptr;
}

void ComboBox::init()
{
	if (!check())
	{
		return;
	}
	QAbstractItemModel* model = m_listWidget->model();
	if (model != nullptr)
	{
		setModel(model);
	}
	setView(m_listWidget);
	NoFocusFrameDelegate* frameDelegate = new NoFocusFrameDelegate(this);
	if (frameDelegate != nullptr)
	{
		setItemDelegate(frameDelegate);
	}
	setMouseTracking(true);
	QObject::connect(m_listWidget, &QListWidget::itemEntered, this, &ComboBox::listItemEntered);
	QObject::connect(m_listWidget, &ListWidget::itemPressed, this, &ComboBox::listItemPressed);
	setDefault();

	QWidget* parentWidget = m_listWidget->parentWidget();
	if (parentWidget != nullptr)
	{
		parentWidget->setWindowFlags((parentWidget->windowFlags() | Qt::FramelessWindowHint));
		parentWidget->setAttribute(Qt::WA_TranslucentBackground);
	}

	QScrollBar* scrollBar = m_listWidget->verticalScrollBar();
	if (scrollBar == nullptr)
	{
		return;
	}
	scrollBar->setStyleSheet(
		"QScrollBar:vertical"
		"{"
		"width:3px;"
		"background:rgba(0,0,0,0%);"
		"margin:0px,0px,0px,0px;"
		"padding-top:0px;"
		"padding-bottom:0px;"
		"}"
		"QScrollBar::handle:vertical"
		"{"
		"width:3px;"
		"background:rgba(89,109,170,100%);"
		"border-radius:1px;"
		"min-height:10;"
		"}"
		"QScrollBar::handle:vertical:hover"
		"{"
		"width:3px;"
		"background:rgba(110,139,229,100%);"
		"border-radius:1px;"
		"min-height:10;"
		"}"
		"QScrollBar::add-line:vertical"
		"{"
		"height:0px;"
		"width:3px;"
		"border-image:none;"
		"subcontrol-position:bottom;"
		"}"
		"QScrollBar::sub-line:vertical"
		"{"
		"height:0px;"
		"width:3px;"
		"border-image:none;"
		"subcontrol-position:top;"
		"}"
		"QScrollBar::add-line:vertical:hover"
		"{"
		"height:0px;"
		"width:3px;"
		"border-image:none;"
		"subcontrol-position:bottom;"
		"}"
		"QScrollBar::sub-line:vertical:hover"
		"{"
		"height:0px;"
		"width:3px;"
		"border-image:none;"
		"subcontrol-position:top;"
		"}"
		"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical"
		"{"
		"background:rgba(54,68,111,100%);"
		"border-radius:1px;"
		"}"
		);
}

void ComboBox::listItemEntered(QListWidgetItem* item)
{
	if (!check())
	{
		return;
	}
	qint32 index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		if (m_listWidget->item(index) == item)
		{
			m_hoverIndex = index;
		}
	}
}

void ComboBox::listItemPressed(QListWidgetItem* item)
{
	if (!check())
	{
		return;
	}
	qint32 index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		if (m_listWidget->item(index) == item)
		{
			emit itemPressed(index);
			break;
		}
	}
}

void ComboBox::repaint()
{
	ControlShow::repaint();
	if (!check())
	{
		return;
	}
	m_listWidget->repaint();
}