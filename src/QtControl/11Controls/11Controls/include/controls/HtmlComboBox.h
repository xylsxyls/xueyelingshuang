#pragma once
#include "IdItemComboBox.h"
#include "ControlsMacro.h"

class QListWidgetItem;
/** 定做类，支持在addItem的时候输入html格式的文本
*/
class ControlsAPI HtmlComboBox : public IdItemComboBox
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

protected:
	void init();

public:
	QColor m_normalColor;
	QColor m_selectedColor;
	QColor m_disabledColor;
	qint32 m_origin;
	qint32 m_leftOrigin;
	qint32 m_topOrigin;
	qint32 m_rightOrigin;
	qint32 m_bottomOrigin;
	qint32 m_itemBorderWidth;
};