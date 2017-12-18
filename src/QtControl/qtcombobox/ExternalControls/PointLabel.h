#pragma once
#include "Label.h"

/** 定做类，控件文字过长时显示省略号
*/
class PointLabel : public Label
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
	void setTextOrigin(int32_t origin, bool rePaint = false);

	/** 插入文本，过长显示省略号
	@param [in] str 字符串
	*/
	void setText(const QString& str);

	/** 获取文本
	@return 返回文本
	*/
	QString text();

private:
	virtual void showEvent(QShowEvent* eve);

private:
	std::wstring m_str;
	int32_t m_leftOrigin;
};