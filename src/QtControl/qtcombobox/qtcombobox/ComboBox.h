#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <stdint.h>
#include <string>
#include "ListWidget.h"

class ComboBox : public QComboBox
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ComboBox(QWidget* parent = NULL);

public:
	/** 将点击框设为圆角，输入圆角半径，只有加入边框才会有圆角
	@param [in] radius 圆角半径
	@param [in] rePaint 是否立即重画
	*/
	void setBorderRadius(int32_t radius, bool rePaint = false);

	/** 设置点击框背景颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& normalColor,
							const QColor& hoverColor,
							const QColor& pressedColor,
							const QColor& disabledColor,
							bool rePaint = false);

	/** 设置边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setBorderWidth(int32_t width, bool rePaint = false);

	/** 设置点击框边框颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& normalColor,
						const QColor& hoverColor,
						const QColor& pressedColor,
						const QColor& disabledColor,
						bool rePaint = false);

	/** 设置点击框边框图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] borderImgHover 非选中悬停图片
	@param [in] borderImgPressed 非选中按下图片
	@param [in] borderImgDisabled 非选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBorderImage(const QString& borderImgPath,
						int32_t borderImgStateCount = 4,
						int32_t borderImgNormal = 1,
						int32_t borderImgHover = 2,
						int32_t borderImgPressed = 3,
						int32_t borderImgDisabled = 4,
						bool rePaint = false);

	/** 设置下拉箭头的宽度
	@param [in] width 下拉箭头宽度
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownWidth(int32_t width, bool rePaint = false);

	/** 设置下拉箭头的高度
	@param [in] height 下拉箭头高度
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownHeight(int32_t height, bool rePaint = false);

	/** 设置下拉箭头的边框宽度
	@param [in] width 下拉箭头宽度
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownBorderWidth(int32_t width, bool rePaint = false);

	/** 设置下拉箭头的图片
	@param [in] dropDownImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownImage(const QString& dropDownImgPath, bool rePaint = false);

	/** 设置文字颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] pressedColor 按下颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& normalColor,
					  const QColor& hoverColor,
					  const QColor& pressedColor,
					  const QColor& disabledColor,
					  bool rePaint = false);

	/** 设置字体
	@param [in] fontName 字体名
	@param [in] rePaint 是否立即重画
	*/
	void setFontFace(const QString& fontName, bool rePaint = false);

	/** 设置字体大小
	@param [in] fontSize 字体大小
	@param [in] rePaint 是否立即重画
	*/
	void setFontSize(int32_t fontSize, bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 下拉框距离点击框的纵向偏移量
	@param [in] origin 纵向偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListOrigin(int32_t origin, bool rePaint = false);

	/** 增加一行
	@param [in] text 文本
	*/
	void addItem(const QString& text);

	/** 重画
	*/
	void repaint();

private:
	void updateStyle();
	void showEvent(QShowEvent* eve);

public:
	ListWidget* m_listWidget;

private:
	bool m_hasSetBorderRadius;
	int32_t m_borderRadius;

	bool m_hasSetBackgroundColor;
	std::map<int32_t, QColor> m_backgroundColorMap;

	bool m_hasSetBorderWidth;
	int32_t m_borderWidth;

	bool m_hasSetBorderColor;
	std::map<int32_t, QColor> m_borderColorMap;

	bool m_hasSetBorderImg;
	bool m_loadBorderImgSuccess;
	int32_t m_borderImgStateCount;
	std::wstring m_borderImgPath;
	std::vector<int32_t> m_vecBorderImgHeight;
	std::map<int32_t, int32_t> m_borderImgMap;

	bool m_hasSetDropDownWidth;
	int32_t m_dropDownWidth;

	bool m_hasSetDropDownHeight;
	int32_t m_dropDownHeight;

	bool m_hasSetDropDownBorderWidth;
	int32_t m_dropDownBorderWidth;

	bool m_hasSetDropDownImg;
	std::wstring m_dropDownImgPath;

	bool m_hasSetTextColor;
	std::map<int32_t, QColor> m_itemTextColorMap;

	bool m_hasSetFontName;
	std::wstring m_fontName;

	bool m_hasSetFontSize;
	int32_t m_fontSize;

	bool m_hasSetTextOrigin;
	int32_t m_textOrigin;

	bool m_hasSetListOrigin;
	int32_t m_listOrigin;
};

#endif