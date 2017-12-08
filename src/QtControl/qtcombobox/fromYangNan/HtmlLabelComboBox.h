#pragma once
#include "ComboBox.h"
#include <QColor>

class Label;
class QListWidgetItem;
class HtmlLabelComboBox : public ComboBox
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	HtmlLabelComboBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~HtmlLabelComboBox();

public:
	/** 增加一行html格式文本
	@param [in] text 文本
	*/
	void addItem(const QString& text);

	/** 增加整串html格式文本
	@param [in] textList 文本列表
	*/
	void addItems(const QStringList& textList);

	/** 设置下拉框字体
	@param [in] fontName 字体名
	@param [in] rePaint 是否立即重画
	*/
	void setListFontFace(const QString& fontName, bool rePaint = false);

	/** 设置下拉框字体大小
	@param [in] fontSize 字体大小
	@param [in] rePaint 是否立即重画
	*/
	void setListFontSize(int32_t fontSize, bool rePaint = false);

	/** 设置下拉框文字颜色
	@param [in] normalColor 常态颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListTextColor(const QColor& normalColor,
						  const QColor& disabledColor,
						  bool rePaint = false);

	/** 设置文字偏移量
	@param [in] origin 文字偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

private:
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

public:
	int32_t m_textOrigin;

private:
	std::map<QListWidgetItem*, Label*> m_mapLabel;
};