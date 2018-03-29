#pragma once
#include <QWidget>
#include <stdint.h>

class RPGContentBase : public QWidget
{
public:
	enum
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

	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGContentBase(QWidget* parent = nullptr);

public:
	/** 设置当前是否是错误状态（支持多线程）
	@param [in] state 错误状态
	*/
	void setState(int32_t state);

Q_SIGNALS:
	void stateSignal(int32_t state);
};