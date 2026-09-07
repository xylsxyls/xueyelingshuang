#pragma once
#include <stdint.h>
#include <QColor>

/** 树控件单元格显示参数，集中保存文字、颜色、图片和嵌入控件绘制信息
*/
struct TreeText
{
public:
	// 字体大小
	int32_t m_fontSize;
	// 文字颜色
	QColor m_textColor;
	// 字体名称
	QString m_fontFace;
	// 文字内容
	QString m_text;
	// 文字对齐方式
	Qt::Alignment m_align;
	// 左侧偏移量
	int32_t m_origin;
	// 背景颜色
	QColor m_backgroundColor;
	// 悬停背景颜色
	QColor m_hoverColor;
	// 选中背景颜色
	QColor m_selectedColor;
	// 禁用背景颜色
	QColor m_disabledColor;
	// 嵌入图片，image为空时不绘制
	QImage m_image;
	// 嵌入图片相对当前格子左上角的位置
	QPoint m_imageLeftTop;
	// 嵌入控件，widget为空时不嵌入
	QWidget* m_widget;

public:
	/** 构造函数
	*/
	TreeText()
	{
		init();
	}

	/** 构造函数
	@param [in] text 文字内容
	*/
	TreeText(const QString& text)
	{
		init();
		m_text = text;
	}

	/** 构造函数
	@param [in] text 文字内容
	*/
	TreeText(const char* text)
	{
		init();
		m_text = text;
	}

	/** 初始化默认显示参数
	*/
	void init()
	{
		m_fontSize = 12;
		m_textColor = QColor(255, 255, 255, 255);
		m_fontFace = QStringLiteral("微软雅黑");
		m_align = Qt::AlignCenter;
		m_origin = 0;
		m_backgroundColor = QColor(0, 0, 0, 0);
		m_hoverColor = QColor(0, 0, 0, 0);
		m_selectedColor = QColor(0, 0, 0, 0);
		m_disabledColor = QColor(0, 0, 0, 0);
		m_imageLeftTop = QPoint(0, 0);
		m_widget = nullptr;
	}
};