#ifndef COMBOBOX_H__
#define COMBOBOX_H__

#include <QtWidgets/QComboBox>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"

class ListWidget;
class ComboBox : 
	public ControlFont < QComboBox >,
	public ControlSelf < QComboBox >,
	public ControlBorderForNormalHoverPressedDisabled < QComboBox >
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ComboBox(QWidget* parent = NULL);

public:
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

	/** 设置下拉箭头的尺寸
	@param [in] width 下拉箭头宽度
	@param [in] height 下拉箭头高度
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownSize(int32_t width, int32_t height, bool rePaint = false);

	/** 设置下拉箭头的边框宽度
	@param [in] width 下拉箭头宽度
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownBorderWidth(int32_t width, bool rePaint = false);

	/** 设置下拉箭头的图片
	@param [in] dropDownImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] dropDownImgStateCount 上下平分几份
	@param [in] dropDownImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] dropDownImgHover 非选中悬停图片
	@param [in] dropDownImgDisabled 非选中禁用图片
	@param [in] dropDownImgCkNormal 选中常态图片
	@param [in] dropDownImgCkDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownImage(const QString& dropDownImgPath,
						  int32_t dropDownImgStateCount = 8,
						  int32_t dropDownImgNormal = 1,
						  int32_t dropDownImgHover = 2,
						  int32_t dropDownImgDisabled = 4,
						  int32_t dropDownImgExpandNormal = 5,
						  int32_t dropDownImgExpandDisabled = 8,
						  bool rePaint = false);

	/** 设置下拉箭头距离右上角的边距
	@param [in] topOrigin 上侧偏移量
	@param [in] rightOrigin 右侧偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setDropDownTopRightOrigin(int32_t topOrigin, int32_t rightOrigin, bool rePaint = false);

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

	/** 设置背景颜色
	@param [in] color 背景颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListBackgroundColor(const QColor& color, bool rePaint = false);

	/** 设置边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setListBorderWidth(int32_t width, bool rePaint = false);

	/** 设置边框颜色
	@param [in] color 常态
	@param [in] rePaint 是否立即重画
	*/
	void setListBorderColor(const QColor& color, bool rePaint = false);

	/** 设置节点背景颜色（list控件没有按下属性）
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListItemBorderColor(const QColor& normalColor,
								const QColor& hoverColor,
								const QColor& disabledColor,
								bool rePaint = false);

	/** 设置节点边框粗度
	@param [in] width 边框粗度
	@param [in] rePaint 是否立即重画
	*/
	void setListItemBorderWidth(int32_t width, bool rePaint = false);

	/** 设置节点背景图片，和边框颜色不用存
	@param [in] borderImgPath 背景图片路径，如果路径中必须使用正斜杠
	@param [in] borderImgStateCount 上下平分几份
	@param [in] borderImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] borderImgHover 非选中悬停图片
	@param [in] borderImgDisabled 非选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setListItemBorderImage(const QString& borderImgPath,
								int32_t borderImgStateCount = 4,
								int32_t borderImgNormal = 1,
								int32_t borderImgHover = 2,
								int32_t borderImgDisabled = 4,
								bool rePaint = false);

	/** 设置节点高度
	@param [in] height 节点高度
	@param [in] rePaint 是否立即重画
	*/
	void setListItemHeight(int32_t height, bool rePaint = false);

	/** 设置文字颜色
	@param [in] normalColor 常态颜色
	@param [in] hoverColor 悬停颜色
	@param [in] disabledColor 禁用颜色
	@param [in] rePaint 是否立即重画
	*/
	void setListTextColor(const QColor& normalColor,
						  const QColor& hoverColor,
						  const QColor& disabledColor,
						  bool rePaint = false);

	/** 设置字体
	@param [in] fontName 字体名
	@param [in] rePaint 是否立即重画
	*/
	void setListFontFace(const QString& fontName, bool rePaint = false);

	/** 设置字体大小
	@param [in] fontSize 字体大小
	@param [in] rePaint 是否立即重画
	*/
	void setListFontSize(int32_t fontSize, bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setListTextOrigin(int32_t origin, bool rePaint = false);

	/** 节点到窗口的四个外边距
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

	/** 重画
	*/
	virtual void repaint();

private:
	virtual void showEvent(QShowEvent* eve);
	virtual void mouseReleaseEvent(QMouseEvent* eve);
	virtual void showPopup();
	virtual void hidePopup();

private:
	ListWidget* m_listWidget;
	int32_t m_dropDownImgNormal;
	int32_t m_dropDownImgDisabled;
	int32_t m_dropDownImgExpandNormal;
	int32_t m_dropDownImgExpandDisabled;
	int32_t m_dropDownImgStateCount;
	std::map<int32_t, std::map<int32_t, int32_t>> m_imageStateMap;
	std::wstring m_imagePath;
};

#endif