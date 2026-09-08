#ifndef CORIGINALBUTTON_H
#define CORIGINALBUTTON_H

/** 废弃类：COriginalButton只保留历史代码，默认不编译历史实现。
调用方不得在新代码中继续依赖该控件；缺失能力请补到PushButton。
如必须临时编译旧代码，需要显式定义QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON。
*/
#include <QPushButton>
#include <QTimer>
#include "QtControlsMacro.h"

#if defined(QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON)

/** 旧版状态按钮控件，内部用整段QSS模板替换方式管理背景、文字、边框和点击防抖状态
*/
class QtControlsAPI COriginalButton : public QPushButton
{
	Q_OBJECT
public:
	/** 构造旧版按钮控件
	@param [in] parent 父窗口指针
	*/
    COriginalButton(QWidget* parent = nullptr);

	/** 析构函数
	*/
    ~COriginalButton();

protected:
	// 当前Qt类名，用于拼接QSS选择器
	QString  m_className;
	// 纵向状态图路径
    QString  m_bkgImage;
	// 纵向状态图切割数量
    quint32  m_bkgImageStateCount;
	// 未选中常态使用的状态图序号
    quint32  m_bkgImageNormal;
	// 未选中悬停使用的状态图序号
    quint32  m_bkgImageHover;
	// 未选中按下使用的状态图序号
    quint32  m_bkgImagePressed;
	// 未选中禁用使用的状态图序号
    quint32  m_bkgImageDisabled;
	// 选中常态使用的状态图序号
    quint32  m_bkgImageCkNormal;
	// 选中悬停使用的状态图序号
    quint32  m_bkgImageCkHover;
	// 选中按下使用的状态图序号
    quint32  m_bkgImageCkPressed;
	// 选中禁用使用的状态图序号
    quint32  m_bkgImageCkDisabled;
	// 未选中常态背景色
    QColor   m_bkgColorNormal;
	// 未选中悬停背景色
    QColor   m_bkgColorHover;
	// 未选中按下背景色
    QColor   m_bkgColorPressed;
	// 未选中禁用背景色
    QColor   m_bkgColorDisabled;
	// 选中常态背景色
    QColor   m_bkgColorCkNormal;
	// 选中悬停背景色
    QColor   m_bkgColorCkHover;
	// 选中按下背景色
    QColor   m_bkgColorCkPressed;
	// 选中禁用背景色
    QColor   m_bkgColorCkDisabled;
	// 未选中常态文字颜色
    QColor   m_fontColorNormal;
	// 未选中悬停文字颜色
    QColor   m_fontColorHover;
	// 未选中按下文字颜色
    QColor   m_fontColorPressed;
	// 未选中禁用文字颜色
    QColor   m_fontColorDisabled;
	// 选中常态文字颜色
    QColor   m_fontColorCkNormal;
	// 选中悬停文字颜色
    QColor   m_fontColorCkHover;
	// 选中按下文字颜色
    QColor   m_fontColorCkPressed;
	// 选中禁用文字颜色
    QColor   m_fontColorCkDisabled;
	// 字体名称
    QString  m_fontFace;
	// 字体像素大小
    quint32  m_fontSize;
	// 是否加粗显示
    bool     m_bold;
	// 是否倾斜显示
    bool     m_italic;
	// 是否显示下划线
    bool     m_underline;
	// 边框宽度
    quint32  m_borderWidth;
	// 边框圆角半径
    quint32  m_borderRadius;
	// QSS边框样式
    QString  m_borderStyle;
	// 未选中常态边框颜色
    QColor   m_borderColorNormal;
	// 未选中悬停边框颜色
    QColor   m_borderColorHover;
	// 未选中按下边框颜色
    QColor   m_borderColorPressed;
	// 未选中禁用边框颜色
    QColor   m_borderColorDisabled;
	// 选中常态边框颜色
    QColor   m_borderColorCkNormal;
	// 选中悬停边框颜色
    QColor   m_borderColorCkHover;
	// 选中按下边框颜色
    QColor   m_borderColorCkPressed;
	// 选中禁用边框颜色
    QColor   m_borderColorCkDisabled;
	// 文字对齐方式
    QString  m_textAlign;
	// 文字左内边距
    quint32  m_leftMargin;
	// 文字上内边距
    quint32  m_topMargin;
	// 文字右内边距
    quint32  m_rightMargin;
	// 文字下内边距
    quint32  m_bottomMargin;
	// 状态图左右九宫格边距
	quint32  m_bkgImageHMargin;
	// 状态图上下九宫格边距
	quint32  m_bkgImageVMargin;
	// tooltip弹出位置偏移
	QPoint m_tooltipOffset;
	// 点击后临时禁用的毫秒数
	quint64 m_clickBreathTime;
	// 点击防抖恢复计时器
	QTimer  m_clickBreathTimer;
	// 防抖期间记录的目标启用状态
	bool    m_savedEnabled;

protected:
	/** 根据当前属性重建旧版按钮QSS
	*/
    void updateStyle();

	/** 处理tooltip偏移等Qt通用事件
	@param [in] e Qt事件对象
	@return 返回Qt事件分发结果
	*/
	bool event(QEvent *e);

private:
	/** 计算纵向状态图指定状态的上边切片位置
	@param [in] stepHeight 单个状态图高度
	@param [in] stateCount 状态图切片数量
	@param [in] imageIndex 状态图序号，从1开始，0表示不裁切
	@return 返回上边切片位置，参数无效时返回0
	*/
	static qint32 imageSliceTop(qint32 stepHeight, quint32 stateCount, quint32 imageIndex);

	/** 计算纵向状态图指定状态的下边切片位置
	@param [in] imageHeight 完整图片高度
	@param [in] stepHeight 单个状态图高度
	@param [in] stateCount 状态图切片数量
	@param [in] imageIndex 状态图序号，从1开始，0表示不裁切
	@return 返回下边切片位置，参数无效时返回0
	*/
	static qint32 imageSliceBottom(qint32 imageHeight, qint32 stepHeight, quint32 stateCount, quint32 imageIndex);

	/** 将外部传入的图片状态序号限制到qint32可表达范围内
	@param [in] value 外部传入的无符号状态值
	@return 返回可安全参与QSS计算的状态值
	*/
	static quint32 clampImageValue(quint64 value);

private slots:
	/** 处理按钮点击信号，启动点击防抖计时
	*/
	void _internalOnClicked();

	/** 处理点击防抖计时结束，恢复按钮目标可用状态
	*/
	void _internalOnClickBreathTimerTimeout();

public:
	/** 设置按钮可用状态；点击防抖生效时会延后同步到Qt控件
	@param [in] s true表示启用按钮，false表示禁用按钮
	*/
	void setEnabled(bool s);

	/** 设置点击后临时禁用的时间
	@param [in] t 按钮点击后保持禁用的毫秒数，0表示关闭点击防抖
	*/
	void setClickBreathTime(quint64 t);

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
                     const quint64  bkgImageStateCount   = 4,
                     const quint64  bkgImageNormal       = 1,
                     const quint64  bkgImageHover        = 2,
                     const quint64  bkgImagePressed      = 3,
                     const quint64  bkgImageDisabled     = 4,
                     const quint64  bkgImageCKNormal     = 1,
                     const quint64  bkgImageCKHover      = 2,
                     const quint64  bkgImageCKPressed    = 3,
                     const quint64  bkgImageCKDisabled   = 4);

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
    void setBkgColor(const QColor& bkgColorNormal     = QColor(0,0,0,0),
                     const QColor& bkgColorHover      = QColor(0,0,0,0),
                     const QColor& bkgColorPressed    = QColor(0,0,0,0),
                     const QColor& bkgColorDisabled   = QColor(0,0,0,0),
                     const QColor& bkgColorCKNormal   = QColor(0,0,0,0),
                     const QColor& bkgColorCKHover    = QColor(0,0,0,0),
                     const QColor& bkgColorCKPressed  = QColor(0,0,0,0),
                     const QColor& bkgColorCKDisabled = QColor(0,0,0,0)
                     );

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
    void setFontColor(const QColor& fontColorNormal     = QColor(255,255,255),
                      const QColor& fontColorHover      = QColor(255,255,255),
                      const QColor& fontColorPressed    = QColor(255,255,255),
                      const QColor& fontColorDisabled   = QColor(255,255,255),
                      const QColor& fontColorCKNormal   = QColor(255,255,255),
                      const QColor& fontColorCKHover    = QColor(255,255,255),
                      const QColor& fontColorCKPressed  = QColor(255,255,255),
                      const QColor& fontColorCKDisabled = QColor(255,255,255)
                     );

	/** 设置字体名称
	@param [in] v 字体名称
	*/
    void setFontFace (const QString& v);

	/** 设置字体大小
	@param [in] v 字体像素大小
	*/
    void setFontSize (const quint32& v);

	/** 设置字体是否加粗
	@param [in] v true表示加粗，false表示普通粗细
	*/
    void setBold     (const bool&    v);

	/** 设置字体是否倾斜
	@param [in] v true表示倾斜，false表示不倾斜
	*/
    void setItalic   (const bool&    v);

	/** 设置字体是否带下划线
	@param [in] v true表示显示下划线，false表示不显示
	*/
    void setUnderline(const bool&    v);

	/** 设置边框宽度
	@param [in] v 边框像素宽度
	*/
    void setBorderWidth (const quint32& v);

	/** 设置边框圆角
	@param [in] v 圆角像素半径
	*/
    void setBorderRadius(const quint32& v);

	/** 设置边框样式
	@param [in] v QSS支持的边框样式，例如solid、dashed
	*/
    void setBorderStyle (const QString& v);

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
    void setBorderColor(const QColor& borderColorNormal     = QColor(0,0,0,0),
                        const QColor& borderColorHover      = QColor(0,0,0,0),
                        const QColor& borderColorPressed    = QColor(0,0,0,0),
                        const QColor& borderColorDisabled   = QColor(0,0,0,0),
                        const QColor& borderColorCKNormal   = QColor(0,0,0,0),
                        const QColor& borderColorCKHover    = QColor(0,0,0,0),
                        const QColor& borderColorCKPressed  = QColor(0,0,0,0),
                        const QColor& borderColorCKDisabled = QColor(0,0,0,0));

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
    void setMargins(const quint32& left  ,
                    const quint32& top   ,
                    const quint32& right ,
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
};

#else

/** 废弃占位控件，只用于让历史工程文件和Qt moc保持可编译。
构造和析构保持私有，外部代码不能继续实例化该控件。
*/
class QtControlsAPI COriginalButton : public QPushButton
{
	Q_OBJECT
private:
	/** 私有构造函数，阻止外部继续创建旧按钮实例
	@param [in] parent 父窗口指针
	*/
	COriginalButton(QWidget* parent = nullptr);

	/** 私有析构函数，配合私有构造函数阻止外部管理旧按钮生命周期
	*/
	~COriginalButton();

private slots:
	/** 兼容历史moc生成文件的点击槽，占位实现不执行任何业务
	*/
	void _internalOnClicked();

	/** 兼容历史moc生成文件的点击防抖计时槽，占位实现不执行任何业务
	*/
	void _internalOnClickBreathTimerTimeout();
};

#endif // QTCONTROLS_ENABLE_DEPRECATED_ORIGINAL_BUTTON

#endif // CORIGINALBUTTON_H
