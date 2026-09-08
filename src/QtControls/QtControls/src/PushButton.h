#pragma once
#include <string>
#include <QColor>
#include <QPoint>
#include <QTimer>
#include <QtWidgets/QPushButton>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QEvent;

/** 轻量按钮控件，基于状态化QSS拼接提供普通按钮和按钮常用能力
*/
class QtControlsAPI PushButton :
	public ControlShow < QPushButton >,
	public ControlFont < QPushButton >,
	public ControlSelf < QPushButton >,
	public ControlBorderForNormalHoverPressedDisabled < QPushButton >,
	public ControlBackgroundForNormalHoverPressedDisabled < QPushButton >
{
	Q_OBJECT
public:
	using ControlFont<QPushButton>::setFontSize;
	using ControlFont<QPushButton>::setFontBold;
	using ControlFont<QPushButton>::setTextOrigin;
	using ControlFont<QPushButton>::setTextRightOrigin;
	using ControlSelf<QPushButton>::setBorderWidth;
	using ControlSelf<QPushButton>::setBorderRadius;
	using ControlBorderForNormalHoverPressedDisabled<QPushButton>::setBorderImage;
	using ControlBorderForNormalHoverPressedDisabled<QPushButton>::setTextColor;
	using ControlBackgroundForNormalHoverPressedDisabled<QPushButton>::setBackgroundColor;
	using ControlBackgroundForNormalHoverPressedDisabled<QPushButton>::setBackgroundImage;

public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	PushButton(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~PushButton();

public slots:
	/** 设置按钮可用状态；点击防抖生效时会记录目标状态，等待防抖结束再同步到Qt控件
	@param [in] enabled true表示启用按钮，false表示禁用按钮
	*/
	void setEnabled(bool enabled);

public:
	/** 设置点击后临时禁用的时间
	@param [in] time 按钮点击后保持禁用的毫秒数，0表示关闭点击防抖
	*/
	void setClickBreathTime(quint64 time);

	/** 设置纵向状态图
	@param [in] bkgImage 图片路径，图片会按状态数量纵向切割
	@param [in] bkgImageStateCount 图片纵向状态数量
	@param [in] bkgImageNormal 未选中常态使用的状态序号，从1开始
	@param [in] bkgImageHover 未选中悬停使用的状态序号，从1开始
	@param [in] bkgImagePressed 未选中按下使用的状态序号，从1开始
	@param [in] bkgImageDisabled 未选中禁用使用的状态序号，从1开始
	@param [in] bkgImageCKNormal 选中常态使用的状态序号，从1开始
	@param [in] bkgImageCKHover 选中悬停使用的状态序号，从1开始
	@param [in] bkgImageCKPressed 选中按下使用的状态序号，从1开始
	@param [in] bkgImageCKDisabled 选中禁用使用的状态序号，从1开始
	*/
	void setBkgImage(const QString& bkgImage,
					 const quint64 bkgImageStateCount = 4,
					 const quint64 bkgImageNormal = 1,
					 const quint64 bkgImageHover = 2,
					 const quint64 bkgImagePressed = 3,
					 const quint64 bkgImageDisabled = 4,
					 const quint64 bkgImageCKNormal = 1,
					 const quint64 bkgImageCKHover = 2,
					 const quint64 bkgImageCKPressed = 3,
					 const quint64 bkgImageCKDisabled = 4);

	/** 设置未选中和选中状态的背景颜色
	@param [in] bkgColorNormal 未选中常态颜色
	@param [in] bkgColorHover 未选中悬停颜色
	@param [in] bkgColorPressed 未选中按下颜色
	@param [in] bkgColorDisabled 未选中禁用颜色
	@param [in] bkgColorCKNormal 选中常态颜色
	@param [in] bkgColorCKHover 选中悬停颜色
	@param [in] bkgColorCKPressed 选中按下颜色
	@param [in] bkgColorCKDisabled 选中禁用颜色
	*/
	void setBkgColor(const QColor& bkgColorNormal = QColor(0, 0, 0, 0),
					 const QColor& bkgColorHover = QColor(0, 0, 0, 0),
					 const QColor& bkgColorPressed = QColor(0, 0, 0, 0),
					 const QColor& bkgColorDisabled = QColor(0, 0, 0, 0),
					 const QColor& bkgColorCKNormal = QColor(0, 0, 0, 0),
					 const QColor& bkgColorCKHover = QColor(0, 0, 0, 0),
					 const QColor& bkgColorCKPressed = QColor(0, 0, 0, 0),
					 const QColor& bkgColorCKDisabled = QColor(0, 0, 0, 0));

	/** 设置未选中和选中状态的文字颜色
	@param [in] fontColorNormal 未选中常态文字颜色
	@param [in] fontColorHover 未选中悬停文字颜色
	@param [in] fontColorPressed 未选中按下文字颜色
	@param [in] fontColorDisabled 未选中禁用文字颜色
	@param [in] fontColorCKNormal 选中常态文字颜色
	@param [in] fontColorCKHover 选中悬停文字颜色
	@param [in] fontColorCKPressed 选中按下文字颜色
	@param [in] fontColorCKDisabled 选中禁用文字颜色
	*/
	void setFontColor(const QColor& fontColorNormal = QColor(255, 255, 255),
					  const QColor& fontColorHover = QColor(255, 255, 255),
					  const QColor& fontColorPressed = QColor(255, 255, 255),
					  const QColor& fontColorDisabled = QColor(255, 255, 255),
					  const QColor& fontColorCKNormal = QColor(255, 255, 255),
					  const QColor& fontColorCKHover = QColor(255, 255, 255),
					  const QColor& fontColorCKPressed = QColor(255, 255, 255),
					  const QColor& fontColorCKDisabled = QColor(255, 255, 255));

	/** 设置字体名称
	@param [in] value 字体名称
	*/
	void setFontFace(const QString& value);

	/** 设置字体大小
	@param [in] value 字体像素大小
	*/
	void setFontSize(const quint32& value);

	/** 设置字体是否加粗
	@param [in] value true表示加粗，false表示普通粗细
	*/
	void setBold(const bool& value);

	/** 设置字体是否倾斜
	@param [in] value true表示倾斜，false表示不倾斜
	*/
	void setItalic(const bool& value);

	/** 设置字体是否带下划线
	@param [in] value true表示显示下划线，false表示不显示
	*/
	void setUnderline(const bool& value);

	/** 设置边框宽度
	@param [in] value 边框像素宽度
	*/
	void setBorderWidth(const quint32& value);

	/** 设置边框圆角
	@param [in] value 圆角像素半径
	*/
	void setBorderRadius(const quint32& value);

	/** 设置边框样式
	@param [in] value QSS支持的边框样式，例如solid、dashed
	*/
	void setBorderStyle(const QString& value);

	/** 设置未选中和选中状态的边框颜色
	@param [in] borderColorNormal 未选中常态边框颜色
	@param [in] borderColorHover 未选中悬停边框颜色
	@param [in] borderColorPressed 未选中按下边框颜色
	@param [in] borderColorDisabled 未选中禁用边框颜色
	@param [in] borderColorCKNormal 选中常态边框颜色
	@param [in] borderColorCKHover 选中悬停边框颜色
	@param [in] borderColorCKPressed 选中按下边框颜色
	@param [in] borderColorCKDisabled 选中禁用边框颜色
	*/
	void setBorderColor(const QColor& borderColorNormal = QColor(0, 0, 0, 0),
						const QColor& borderColorHover = QColor(0, 0, 0, 0),
						const QColor& borderColorPressed = QColor(0, 0, 0, 0),
						const QColor& borderColorDisabled = QColor(0, 0, 0, 0),
						const QColor& borderColorCKNormal = QColor(0, 0, 0, 0),
						const QColor& borderColorCKHover = QColor(0, 0, 0, 0),
						const QColor& borderColorCKPressed = QColor(0, 0, 0, 0),
						const QColor& borderColorCKDisabled = QColor(0, 0, 0, 0));

	/** 设置文字对齐方式
	@param [in] align QSS支持的text-align值
	*/
	void setTextAlign(const QString& align);

	/** 设置按钮文字内边距
	@param [in] left 左边距像素值
	@param [in] top 上边距像素值
	@param [in] right 右边距像素值
	@param [in] bottom 下边距像素值
	*/
	void setMargins(const quint32& left,
					const quint32& top,
					const quint32& right,
					const quint32& bottom);

	/** 设置状态图九宫格边距
	@param [in] hMargin 左右边距像素值
	@param [in] vMargin 上下边距像素值
	*/
	void setBkgMargins(const quint32& hMargin, const quint32& vMargin);

	/** 设置tooltip弹出位置偏移
	@param [in] x 水平方向偏移
	@param [in] y 垂直方向偏移
	*/
	void setToolTipOffset(int x, int y);

protected:
	/** 处理tooltip显示偏移等Qt通用事件
	@param [in] eve Qt事件对象
	@return 返回Qt事件分发结果
	*/
	bool event(QEvent* eve);

private slots:
	/** 处理按钮点击信号，启动点击防抖计时
	*/
	void onClicked();

	/** 处理点击防抖计时结束，恢复按钮目标可用状态
	*/
	void onClickBreathTimerTimeout();

private:
	/** 根据当前兼容属性重建按钮QSS
	*/
	void updateCompatibleStyle();

	/** 为指定状态写入背景图、背景色、文字颜色和边框
	@param [in] firstState 第一层状态，空字符串表示不追加状态
	@param [in] secondState 第二层状态，空字符串表示不追加状态
	@param [in] imageIndex 状态图序号，从1开始
	@param [in] backgroundColor 背景颜色
	@param [in] fontColor 文字颜色
	@param [in] borderColor 边框颜色
	*/
	void updateStateStyle(const std::wstring& firstState,
						  const std::wstring& secondState,
						  quint32 imageIndex,
						  const QColor& backgroundColor,
						  const QColor& fontColor,
						  const QColor& borderColor);

	/** 为状态节点写入单个QSS键值
	@param [in] firstState 第一层状态，空字符串表示不追加状态
	@param [in] secondState 第二层状态，空字符串表示不追加状态
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	void addStateStyleValue(const std::wstring& firstState,
							const std::wstring& secondState,
							const std::wstring& key,
							const std::wstring& value);

	/** 生成指定状态图序号对应的border-image值
	@param [in] imageIndex 状态图序号，从1开始
	@return 返回QSS可直接使用的border-image值，图片不可用时返回none
	*/
	std::wstring borderImageValue(quint32 imageIndex) const;

	/** 生成当前边框QSS值
	@param [in] borderColor 边框颜色
	@return 返回QSS可直接使用的border值
	*/
	std::wstring borderValue(const QColor& borderColor) const;

private:
	// 纵向状态图路径
	QString m_bkgImage;
	// 纵向状态图切割数量
	quint32 m_bkgImageStateCount;
	// 未选中常态使用的状态图序号
	quint32 m_bkgImageNormal;
	// 未选中悬停使用的状态图序号
	quint32 m_bkgImageHover;
	// 未选中按下使用的状态图序号
	quint32 m_bkgImagePressed;
	// 未选中禁用使用的状态图序号
	quint32 m_bkgImageDisabled;
	// 选中常态使用的状态图序号
	quint32 m_bkgImageCKNormal;
	// 选中悬停使用的状态图序号
	quint32 m_bkgImageCKHover;
	// 选中按下使用的状态图序号
	quint32 m_bkgImageCKPressed;
	// 选中禁用使用的状态图序号
	quint32 m_bkgImageCKDisabled;
	// 未选中常态背景色
	QColor m_bkgColorNormal;
	// 未选中悬停背景色
	QColor m_bkgColorHover;
	// 未选中按下背景色
	QColor m_bkgColorPressed;
	// 未选中禁用背景色
	QColor m_bkgColorDisabled;
	// 选中常态背景色
	QColor m_bkgColorCKNormal;
	// 选中悬停背景色
	QColor m_bkgColorCKHover;
	// 选中按下背景色
	QColor m_bkgColorCKPressed;
	// 选中禁用背景色
	QColor m_bkgColorCKDisabled;
	// 未选中常态文字颜色
	QColor m_fontColorNormal;
	// 未选中悬停文字颜色
	QColor m_fontColorHover;
	// 未选中按下文字颜色
	QColor m_fontColorPressed;
	// 未选中禁用文字颜色
	QColor m_fontColorDisabled;
	// 选中常态文字颜色
	QColor m_fontColorCKNormal;
	// 选中悬停文字颜色
	QColor m_fontColorCKHover;
	// 选中按下文字颜色
	QColor m_fontColorCKPressed;
	// 选中禁用文字颜色
	QColor m_fontColorCKDisabled;
	// 字体名称
	QString m_fontFace;
	// 字体像素大小
	quint32 m_fontSize;
	// 是否加粗显示
	bool m_bold;
	// 是否倾斜显示
	bool m_italic;
	// 是否显示下划线
	bool m_underline;
	// 边框宽度
	quint32 m_borderWidth;
	// 边框圆角半径
	quint32 m_borderRadius;
	// QSS边框样式
	QString m_borderStyle;
	// 未选中常态边框颜色
	QColor m_borderColorNormal;
	// 未选中悬停边框颜色
	QColor m_borderColorHover;
	// 未选中按下边框颜色
	QColor m_borderColorPressed;
	// 未选中禁用边框颜色
	QColor m_borderColorDisabled;
	// 选中常态边框颜色
	QColor m_borderColorCKNormal;
	// 选中悬停边框颜色
	QColor m_borderColorCKHover;
	// 选中按下边框颜色
	QColor m_borderColorCKPressed;
	// 选中禁用边框颜色
	QColor m_borderColorCKDisabled;
	// 文字对齐方式
	QString m_textAlign;
	// 文字左内边距
	quint32 m_leftMargin;
	// 文字上内边距
	quint32 m_topMargin;
	// 文字右内边距
	quint32 m_rightMargin;
	// 文字下内边距
	quint32 m_bottomMargin;
	// 状态图左右九宫格边距
	quint32 m_bkgImageHMargin;
	// 状态图上下九宫格边距
	quint32 m_bkgImageVMargin;
	// tooltip弹出位置偏移
	QPoint m_toolTipOffset;
	// 点击后临时禁用的毫秒数
	quint64 m_clickBreathTime;
	// 点击防抖恢复计时器
	QTimer m_clickBreathTimer;
	// 防抖期间记录的目标启用状态
	bool m_savedEnabled;
};
