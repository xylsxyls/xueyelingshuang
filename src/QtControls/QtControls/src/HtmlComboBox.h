#pragma once
#include "IdItemComboBox.h"
#include "QtControlsMacro.h"

class HtmlComboBoxStyle;
class QListWidgetItem;
/** 定做类，支持在addItem的时候输入html格式的文本
*/
class QtControlsAPI HtmlComboBox : public IdItemComboBox
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	HtmlComboBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~HtmlComboBox();

public:
	/** 设置下拉框文字颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListTextColor(const QColor& normalColor,
						  const QColor& hoverColor = QColor(0, 0, 0, 0),
						  const QColor& disabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);

	/** 设置下拉框文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListTextOrigin(qint32 origin, bool rePaint = false);

	/** 节点到下拉框窗口的四个外边距
	@param [in] leftOrigin 左侧偏移量
	@param [in] topOrigin 上侧偏移量
	@param [in] rightOrigin 右侧偏移量
	@param [in] bottomOrigin 下侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListItemAroundOrigin(qint32 leftOrigin,
								 qint32 topOrigin = -1,
								 qint32 rightOrigin = -1,
								 qint32 bottomOrigin = -1,
								 bool rePaint = false);

	/** 设置下拉框节点边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setListItemBorderWidth(qint32 width, bool rePaint = false);

	/** 设置提示框背景色
	@param [in] color 背景颜色
	*/
	void setToolTipBackgroundColor(const QColor& color);

protected:
	/** 初始化HTML下拉框代理、样式对象和默认文本绘制参数
	*/
	void init();

public:
	// 列表项常态文字颜色
	QColor m_normalColor;
	// 列表项选中文字颜色
	QColor m_selectedColor;
	// 列表项禁用文字颜色
	QColor m_disabledColor;
	// 列表项文字左侧偏移量
	qint32 m_origin;
	// 列表项左边距
	qint32 m_leftOrigin;
	// 列表项上边距
	qint32 m_topOrigin;
	// 列表项右边距
	qint32 m_rightOrigin;
	// 列表项下边距
	qint32 m_bottomOrigin;
	// 列表项边框宽度
	qint32 m_itemBorderWidth;
	// HTML下拉框自定义Style对象
	HtmlComboBoxStyle* m_style;
};