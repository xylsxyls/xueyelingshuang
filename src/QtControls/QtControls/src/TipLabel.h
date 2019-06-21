#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

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
	virtual void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);

private:
	qint32 m_existTime;
	qint32 m_timeId;
	qint32 m_maxWidth;
	bool m_hasFocus;
	QPoint m_bottomRight;
};