#pragma once
#include <QWidget>
#include <stdint.h>

class RPGContentBase : public QWidget
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGContentBase(QWidget* parent = nullptr);

public:
	enum ContentState
	{
		/** 常态
		*/
		NORMAL,

		/** 无值错误
		*/
		ERROR_VALUE,

		/** 无标题错误
		*/
		ERROR_HEADER,

		/** 所有数据都没有
		*/
		ERROR_ALL
	};

public:
	/** 设置当前是否是错误状态（支持多线程）
	@param [in] state 错误状态
	*/
	void setState(ContentState state);

Q_SIGNALS:
	//子类通过该信号改变窗口内容显示
	void stateSignal(ContentState state);
};