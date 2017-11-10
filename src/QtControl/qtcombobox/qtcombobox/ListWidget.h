#pragma once
#include <QtWidgets/QListWidget>
#include <stdint.h>

class ListWidget : public QListWidget
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ListWidget(QWidget* parent = NULL);

public:
	/** 设置背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setBorderWidth(int32_t width, bool rePaint = false);

	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setBorderColor(const QColor& color, bool rePaint = false);

	/** 设置节点背景颜色（list控件没有按下属性）
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& hoverColor,
							const QColor& disabledColor,
							bool rePaint = false);

	/** 设置节点边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderWidth(int32_t width, bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] borderImgHover 非选中悬停图片
	@param [in] borderImgDisabled 非选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setItemBorderImage(const QString& borderImgPath,
							int32_t borderImgStateCount = 4,
							int32_t borderImgNormal = 1,
							int32_t borderImgHover = 2,
							int32_t borderImgDisabled = 4,
							bool rePaint = false);

	/** 设置节点高度
	@param [in] height 节点高度
	@param [in] rePaint 是否立即重画
	*/
	void setItemHeight(int32_t height, bool rePaint = false);

	/** 设置文字颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& normalColor,
					  const QColor& hoverColor,
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

	/** 节点到窗口的左侧外边距
	@param [in] origin 左侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemLeftOrigin(int32_t origin, bool rePaint = false);

	/** 节点到窗口的上侧外边距
	@param [in] origin 上侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemTopOrigin(int32_t origin, bool rePaint = false);

	/** 节点到窗口的右侧外边距
	@param [in] origin 右侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemRightOrigin(int32_t origin, bool rePaint = false);

	/** 节点到窗口的下侧外边距
	@param [in] origin 下侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setItemBottomOrigin(int32_t origin, bool rePaint = false);

	/** 重画
	*/
	void repaint();

private:
	void updateStyle();
	void showEvent(QShowEvent* eve);

private:
	bool m_hasSetBackgroundColor;
	QColor m_backgroundColor;

	bool m_hasSetBorderWidth;
	int32_t m_borderWidth;

	bool m_hasSetBorderColor;
	QColor m_borderColor;

	bool m_hasSetItemBorderColor;
	std::map<int32_t, QColor> m_itemBorderColorMap;

	bool m_hasSetItemBorderWidth;
	int32_t m_itemBorderWidth;

	bool m_hasSetItemBorderImg;
	bool m_loadItemBorderImgSuccess;
	int32_t m_itemBorderImgStateCount;
	std::wstring m_itemBorderImgPath;
	std::vector<int32_t> m_vecItemBorderImgHeight;
	std::map<int32_t, int32_t> m_itemBorderImgMap;

	bool m_hasSetItemHeight;
	int32_t m_itemHeight;

	bool m_hasSetTextColor;
	std::map<int32_t, QColor> m_itemTextColorMap;

	bool m_hasSetFontName;
	std::wstring m_fontName;

	bool m_hasSetFontSize;
	int32_t m_fontSize;

	bool m_hasSetTextOrigin;
	int32_t m_textOrigin;

	bool m_hasSetItemLeftOrigin;
	int32_t m_itemLeftOrgin;

	bool m_hasSetItemTopOrigin;
	int32_t m_itemTopOrgin;

	bool m_hasSetItemRightOrigin;
	int32_t m_itemRightOrgin;

	bool m_hasSetItemBottomOrigin;
	int32_t m_itemBottomOrgin;
};