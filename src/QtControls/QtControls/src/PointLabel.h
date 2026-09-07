#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** 定做类，控件文字过长时显示省略号
*/
class QtControlsAPI PointLabel : public Label
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	PointLabel(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~PointLabel();

public:
	/** 设置文本偏移量
	@param [in] origin 文本偏移量
	@param [in] rePaint 是否立即重画
	*/
	void setTextOrigin(qint32 origin, bool rePaint = false);

	/** 插入文本，过长显示省略号
	@param [in] str 字符串
	*/
	void setText(const QString& str);

	/** 获取文本
	@return 返回文本
	*/
	QString text();

protected:
	/** 显示时根据当前控件宽度刷新省略号文本
	@param [in] eve Qt显示事件
	*/
	virtual void showEvent(QShowEvent* eve);

private:
	// 原始完整文本
	std::wstring m_str;
	// 文本左侧偏移量
	qint32 m_leftOrigin;
};