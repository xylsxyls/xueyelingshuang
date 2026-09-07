#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

class QEvent;

/** 定做类，用于界面提示
*/
class QtControlsAPI TipLabel : public Label
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	TipLabel(QWidget* parent = nullptr);

	/** 设置存在时间（单位秒）
	@param [in] time 时间
	*/
	void setExistTime(qint32 time);

	/** 设置最大宽度
	@param [in] maxWidth 最大宽度
	*/
	void setMaxWidth(qint32 maxWidth);

	/** 设置是否有焦点
	@param [in] hasFocus 弹出时是否抢焦点
	*/
	void setHasFocus(bool hasFocus);

	/** 设置窗口所在位置（右下角）
	@param [in] bottomRight 右下角坐标
	*/
	void setBottomRight(const QPoint& bottomRight);

protected:
	/** 显示时计算提示框位置并启动自动关闭计时器
	@param [in] eve Qt显示事件
	*/
	virtual void showEvent(QShowEvent* eve);

	/** 处理自动关闭计时器事件
	@param [in] eve Qt定时器事件
	*/
	void timerEvent(QTimerEvent* eve);

	/** 处理跨平台激活和失焦事件，控制提示框层级和关闭行为
	@param [in] eve Qt事件对象
	@return 返回Qt事件分发结果
	*/
	bool event(QEvent* eve);

	/** 处理Windows原生激活消息，兼容提示框置顶和失焦关闭
	@param [in] eventType Qt传入的原生事件类型
	@param [in] message 平台原生消息指针
	@param [out] result 原生消息处理结果
	@return 返回true表示消息已经处理，false表示继续交给Qt
	*/
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);

private:
	// 提示框自动存在时间，单位秒
	qint32 m_existTime;
	// 自动关闭计时器ID，-1表示未启动
	qint32 m_timeId;
	// 提示文字最大显示宽度
	qint32 m_maxWidth;
	// 弹出时是否抢焦点
	bool m_hasFocus;
	// 提示框右下角目标坐标
	QPoint m_bottomRight;
};