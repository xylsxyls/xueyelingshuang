#pragma once
#include "Label.h"

/** 定做类，用于界面提示
*/
class TipLabel : public Label
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
	void setExistTime(int32_t time);

	/** 设置最大宽度
	@param [in] maxWidth 最大宽度
	*/
	void setMaxWidth(int32_t maxWidth);

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
	int32_t m_existTime;
	int32_t m_timeId;
	int32_t m_maxWidth;
	bool m_hasFocus;
	QPoint m_bottomRight;
};