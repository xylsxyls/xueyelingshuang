#ifndef CORIGINALDIALOG_H
#define CORIGINALDIALOG_H

#include <QColor>
#include <QDialog>
#include <QFont>
#include <QPoint>
#include <QRect>
#include <QtGlobal>
#include "QtControlsMacro.h"

class QCloseEvent;
class QKeyEvent;
class QMouseEvent;
class QWindow;

/** 无边框对话框基础类，封装自定义标题栏拖动、边缘缩放、激活状态和快捷键关闭控制
*/
class QtControlsAPI COriginalDialog : public QDialog
{
	Q_OBJECT
public:
	/** 构造无边框基础对话框
	@param [in] parent 父窗口指针
	*/
	COriginalDialog(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~COriginalDialog();

public:
	/** 设置对话框默认字体
	@param [in] font 对话框默认字体对象
	*/
	static void setDefaultDialogFont(const QFont& font);

	/** 获取对话框默认字体
	@return 返回对话框默认字体对象
	*/
	static QFont defaultDialogFont();

	/** 设置对话框默认文字颜色
	@param [in] color 对话框默认文字颜色
	*/
	static void setDefaultDialogTextColor(const QColor& color);

	/** 获取对话框默认文字颜色
	@return 返回对话框默认文字颜色
	*/
	static QColor defaultDialogTextColor();

	/** 设置鼠标命中窗口缩放边缘的宽度
	@param [in] n 边缘命中宽度，单位为像素
	*/
	void setTouchBorderWidth(int n);

	/** 获取鼠标命中窗口缩放边缘的宽度
	@return 边缘命中宽度，单位为像素
	*/
	int  touchBorderWidth();

	/** 设置自定义标题栏高度
	@param [in] n 标题栏高度，单位为像素
	*/
	void setCustomerTitleBarHeight(int n);

	/** 获取自定义标题栏高度
	@return 标题栏高度，单位为像素
	*/
	int  customerTitleBarHeight();

	/** 获取自定义标题栏区域
	@return 标题栏在窗口客户区内的矩形区域
	*/
	QRect customerTitleBarRect();

	/** 设置模态或浮动窗口依附的上层窗口
	@param [in] window 需要依附的Qt窗口对象
	*/
	void setTransientWindow(QWindow* window);

	/** 获取同进程内可作为依附对象的顶层窗口
	@param [in] window 传入的窗口对象
	@return 可依附的顶层窗口，无法解析时返回传入窗口
	*/
    QWindow* getAncestorHandle(QWindow* window);

	/** 获取当前依附的上层窗口
	@return 已设置的依附窗口对象
	*/
	QWindow* transientWindow();

	/** 设置是否允许Alt+F4触发关闭
	@param [in] enabled true表示允许，false表示忽略Alt+F4
	*/
    void setAltF4Enable(bool enabled);

	/** 获取Alt+F4关闭开关状态
	@return true表示允许Alt+F4关闭，false表示忽略
	*/
    bool altF4Enable();

signals:
	/** Windows非客户区激活状态变化信号，建议使用Qt::QueuedConnection连接
	@param [in] ncActive true表示窗口处于激活状态，false表示非激活
	*/
	void ncActiveChanged(bool ncActive);

	/** Windows窗口激活来源变化信号，建议使用Qt::QueuedConnection连接
	@param [in] activeType 鼠标激活为2，键盘激活为1，非激活为0
	*/
	void activeChanged(int activeType);

protected:
#ifdef Q_OS_WIN
	/** Windows下把鼠标坐标转换成非客户区命中测试结果
	@param [in] pt 鼠标所在屏幕坐标
	@return 返回Windows窗口命中测试常量
	*/
	long onNcHitTest(const QPoint& pt);
#endif

	/** 窗口尺寸变化时同步自定义标题栏矩形
	@param [in] eve Qt尺寸变化事件
	*/
	void resizeEvent(QResizeEvent* eve);

	/** 处理Windows原生消息，负责无边框命中测试、激活通知和系统键拦截
	@param [in] eventType Qt传入的原生事件类型
	@param [in] message 平台原生消息指针
	@param [out] result 原生消息处理结果
	@return 返回true表示消息已经处理，false表示继续交给Qt
	*/
	virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result);

	/** 处理Alt+F4快捷键触发的关闭请求，子类可重写以改写关闭语义
	*/
    virtual void altF4PressedEvent();

	/** 过滤子控件事件，用于判断自定义标题栏区域能否拖动
	@param [in] tar 事件目标对象
	@param [in] eve Qt事件对象
	@return 返回true表示事件被拦截，false表示继续分发
	*/
	bool eventFilter(QObject* tar, QEvent* eve);

	/** 处理键盘事件，统一拦截Alt+F4行为
	@param [in] eve Qt键盘事件
	*/
	void keyPressEvent(QKeyEvent* eve);
#ifndef Q_OS_WIN
	/** 处理非Windows平台的鼠标按下，用于开始拖动或缩放
	@param [in] eve Qt鼠标事件
	*/
	void mousePressEvent(QMouseEvent* eve);

	/** 处理非Windows平台的鼠标移动，用于窗口拖动、缩放和光标更新
	@param [in] eve Qt鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent* eve);

	/** 处理非Windows平台的鼠标释放，用于结束拖动或缩放
	@param [in] eve Qt鼠标事件
	*/
	void mouseReleaseEvent(QMouseEvent* eve);

	/** 处理非Windows平台的关闭事件，配合Alt+F4开关控制关闭行为
	@param [in] eve Qt关闭事件
	*/
	void closeEvent(QCloseEvent* eve);
public:
	/** 在非Windows平台按统一规则关闭窗口
	*/
	void close();
protected:
	enum TouchType
	{
		// 未命中可缩放边缘
		NORMAL,
		// 命中上边缘
		NORTH,
		// 命中下边缘
		SOUTH,
		// 命中右边缘
		EAST,
		// 命中左边缘
		WEST,
		// 命中右上角
		NORTH_EAST,
		// 命中左下角
		SOUTH_WEST,
		// 命中左上角
		NORTH_WEST,
		// 命中右下角
		SOUTH_EAST
	};

	/** 根据客户区坐标判断鼠标所在的缩放区域
	@param [in] clientPos 鼠标在客户区内的位置
	@return 命中的窗口边缘或角落类型
	*/
	TouchType getTouchType(const QPoint& clientPos) const;

	/** 判断坐标是否位于可拖动的自定义标题栏区域
	@param [in] clientPos 鼠标在客户区内的位置
	@return true表示可以拖动窗口，false表示不作为标题栏拖动处理
	*/
	bool isTitleDraggableArea(const QPoint& clientPos) const;

	/** 根据命中区域更新鼠标光标
	@param [in] touchType 当前命中的窗口边缘或角落类型
	*/
	void updateCursorByTouchType(TouchType touchType);

	/** 根据鼠标全局坐标应用窗口缩放
	@param [in] globalPos 鼠标当前全局坐标
	*/
	void applyResizeByGlobalPos(const QPoint& globalPos);
#endif

private:
	// 鼠标命中窗口缩放边缘的宽度
	qint32   m_touchBorderWidth;
	// 自定义标题栏高度
	qint32   m_customerTitleBarHeight;
	// 自定义标题栏在客户区中的矩形
	QRect    m_customerTitleBarRect;
	// 是否允许Alt+F4触发关闭
	bool     m_altF4Enable;
#ifndef Q_OS_WIN
	// 非Windows平台本次关闭是否来自Alt+F4
	bool     m_altF4Close;
	// 非Windows平台延迟处理的关闭事件指针
	QCloseEvent* m_altCloseEve;
	// 鼠标左键是否处于按下状态
	bool     m_isLeftButtonPressed;
	// 当前按下动作是否进入标题栏拖动状态
	bool     m_isMovePressed;
	// 鼠标按下时的全局坐标
	QPoint   m_pressGlobalPos;
	// 鼠标按下时的窗口几何信息
	QRect    m_pressGeometry;
	// 鼠标当前命中的缩放区域
	TouchType m_currentType;
	// 鼠标按下时锁定的缩放区域
	TouchType m_pressedType;
#endif
};

#endif //CORIGINALDIALOG_H