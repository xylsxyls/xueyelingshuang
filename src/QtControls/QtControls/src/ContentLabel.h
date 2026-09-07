#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

class Label;
/** 双行内容标签组合控件，管理上下两个Label的文字、颜色、布局和单双行切换
*/
class QtControlsAPI ContentLabel
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ContentLabel(QWidget* parent = nullptr);

public:
	/** 设置上面的颜色
	@param [in] color 文字颜色
	*/
	void setUpColor(const QColor& color);

	/** 设置下面一行的颜色
	@param [in] color 文字颜色
	*/
	void setDownColor(const QColor& color);

	/** 设置为一行模式
	*/
	void setOneLine();

	/** 设置为两行模式
	*/
	void setTwoLine();

	/** 设置上面一行的文字
	@param [in] text 文字内容
	*/
	void setUpText(const QString& text);

	/** 设置下面一行的文字
	@param [in] text 文字内容
	*/
	void setDownText(const QString& text);

	/** 设置大小
	@param [in] rect 控件大小
	*/
	void setGeometry(const QRect& rect);

	/** 设置背景颜色
	@param [in] color 背景颜色
	*/
	void setBackgroundColor(const QColor& color);

	/** 设置字体
	@param [in] fontFace 字体名
	*/
	void setFontFace(const QString& fontFace);

	/** 设置字体大小
	@param [in] size 字体大小
	*/
	void setFontSize(qint32 size);

private:
	/** 根据当前单双行模式和矩形区域更新上下标签布局
	*/
	void update();

	/** 检测内部标签对象是否可用
	@return 返回true表示内部状态可用，false表示存在空指针
	*/
	bool check();

	/** 创建上下两个内部标签并设置默认布局状态
	*/
	void init();

public:
	// 上半部分文字标签
	Label* m_up;
	// 下半部分文字标签
	Label* m_down;
	// 组合控件所在矩形区域
	QRect m_rect;
	// 当前是否为单行显示模式
	bool m_isOne;
};