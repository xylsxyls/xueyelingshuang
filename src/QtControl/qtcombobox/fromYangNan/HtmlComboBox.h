#pragma once
#include "IdItemComboBox.h"

class QListWidgetItem;
/** 定做类，支持在addItem的时候输入html格式的文本
*/
class HtmlComboBox : public IdItemComboBox
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

	/** 设置点击框文字偏移量
	@param [in] origin 点击框文字偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 设置下拉框文字颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListTextColor(const QColor& normalColor,
						  const QColor& hoverColor,
						  const QColor& disabledColor,
						  bool rePaint = false);

	/** 设置下拉框文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListTextOrigin(int32_t origin, bool rePaint = false);

	/** 节点到下拉框窗口的四个外边距
	@param [in] leftOrigin 左侧偏移量
	@param [in] topOrigin 上侧偏移量
	@param [in] rightOrigin 右侧偏移量
	@param [in] bottomOrigin 下侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListItemAroundOrigin(int32_t leftOrigin,
								 int32_t topOrigin,
								 int32_t rightOrigin,
								 int32_t bottomOrigin,
								 bool rePaint = false);

	/** 设置下拉框节点边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setListItemBorderWidth(int32_t width, bool rePaint = false);

public:
	QColor m_normalColor;
	QColor m_selectedColor;
	QColor m_disabledColor;
	int32_t m_comboBoxOrigin;
	int32_t m_origin;
	int32_t m_leftOrigin;
	int32_t m_topOrigin;
	int32_t m_rightOrigin;
	int32_t m_bottomOrigin;
	int32_t m_itemBorderWidth;
};