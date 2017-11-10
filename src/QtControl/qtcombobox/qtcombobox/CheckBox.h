#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <stdint.h>
#include <string>

class CheckBox : public QCheckBox
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	CheckBox(QWidget* parent = NULL);

public:
	/** 设置CheckBox指示器图片
	@param [in] indicatorImg 背景图片路径，如果路径中必须使用正斜杠
	@param [in] indicatorImgStateCount 只能填4或8，表示4态按钮或8态按钮
	@param [in] indicatorImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] indicatorImgHover 非选中悬停图片
	@param [in] indicatorImgPressed 非选中按下图片
	@param [in] indicatorImgDisabled 非选中禁用图片
	@param [in] indicatorImgCKNormal 选中常态图片
	@param [in] indicatorImgCKHover 选中悬停图片
	@param [in] indicatorImgCKPressed 选中按下图片
	@param [in] indicatorImgCKDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setIndicatorImage(const QString& indicatorImg,
			   			   int32_t indicatorImgStateCount = 4,
						   int32_t indicatorImgNormal = 1,
						   int32_t indicatorImgHover = 2,
						   int32_t indicatorImgPressed = 3,
						   int32_t indicatorImgDisabled = 4,
						   int32_t indicatorImgCKNormal = 1,
						   int32_t indicatorImgCKHover = 2,
						   int32_t indicatorImgCKPressed = 3,
						   int32_t indicatorImgCKDisabled = 4,
						   bool rePaint = false);

	/** 设置CheckBox指示器和文字整体的背景图片
	@param [in] backgroundImg 背景图片路径，如果路径中必须使用正斜杠
	@param [in] backgroundImgStateCount 只能填4或8，表示4态按钮或8态按钮
	@param [in] backgroundImgNormal 非选中常态图片，如果填1表示将图片纵向分割4份或8份，选最上面的第一份
	@param [in] backgroundImgHover 非选中悬停图片
	@param [in] backgroundImgPressed 非选中按下图片
	@param [in] backgroundImgDisabled 非选中禁用图片
	@param [in] backgroundImgCKNormal 选中常态图片
	@param [in] backgroundImgCKHover 选中悬停图片
	@param [in] backgroundImgCKPressed 选中按下图片
	@param [in] backgroundImgCKDisabled 选中禁用图片
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundImage(const QString& backgroundImg,
			   			    int32_t backgroundImgStateCount = 4,
						    int32_t backgroundImgNormal = 1,
						    int32_t backgroundImgHover = 2,
						    int32_t backgroundImgPressed = 3,
						    int32_t backgroundImgDisabled = 4,
						    int32_t backgroundImgCKNormal = 1,
						    int32_t backgroundImgCKHover = 2,
						    int32_t backgroundImgCKPressed = 3,
						    int32_t backgroundImgCKDisabled = 4,
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

	/** 设置字体颜色
	@param [in] 同上，8态颜色
	@param [in] rePaint 是否立即重画
	*/
	void setTextColor(const QColor& textNormalColor,
	 	 		 	  const QColor& textHoverColor,
					  const QColor& textPressedColor,
					  const QColor& textDisabledColor,
					  const QColor& textCkNormalColor,
					  const QColor& textCkHoverColor,
					  const QColor& textCkPressedColor,
					  const QColor& textCkDisabledColor,
					  bool rePaint = false);

	/** 设置背景颜色，指示器和文字整体的背景颜色
	@param [in] 同上，8态颜色
	@param [in] rePaint 是否立即重画
	*/
	void setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 		    const QColor& backgroundHoverColor,
						    const QColor& backgroundPressedColor,
						    const QColor& backgroundDisabledColor,
						    const QColor& backgroundCkNormalColor,
						    const QColor& backgroundCkHoverColor,
						    const QColor& backgroundCkPressedColor,
						    const QColor& backgroundCkDisabledColor,
							bool rePaint = false);

	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 重画
	*/
	void repaint();

private:
	void updateStyle();
	void showEvent(QShowEvent* eve);

private:
	bool m_hasSetIndicatorImg;
	bool m_loadIndicatorImgSuccess;
	std::wstring m_indicatorImgPath;
	int32_t m_indicatorImgStateCount;
	std::vector<int32_t> m_vecIndicatorImgHeight;
	std::map<int32_t,std::map<int32_t,int32_t>> m_indicatorImgMap;

	bool m_hasSetBackgroundImg;
	bool m_loadBackgroundImgSuccess;
	std::wstring m_backgroundImgPath;
	int32_t m_backgroundImgStateCount;
	std::vector<int32_t> m_vecBackgroundImgHeight;
	std::map<int32_t,std::map<int32_t,int32_t>> m_backgroundImgMap;

	bool m_hasSetFontFace;
	std::wstring m_fontName;

	bool m_hasSetFontSize;
	int32_t m_fontSize;

	bool m_hasSetTextColor;
	std::map<int32_t,std::map<int32_t,QColor>> m_textColorMap;

	bool m_hasSetBackgroundColor;
	std::map<int32_t,std::map<int32_t,QColor>> m_backgroundColorMap;

	bool m_hasSetTextOrigin;
	int32_t m_textOrigin;
};

#endif