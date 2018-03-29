#pragma once
#include <QWidget>
#include "RPGContentBase.h"

class Label;
class ContentLabel;
/** RPG四列战报显示
*/
class RPGFourWidget : public RPGContentBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGFourWidget(QWidget* parent = nullptr);

public:
	/** 初始化（该函数要求每个QStringList内必须有8个值，不管是不是错误状态）
	@param [in] titleLeft 左侧标题内容
	@param [in] titleRight 右侧标题内容
	@param [in] header 标题字符串集
	@param [in] value 内容字符串集
	@param [in] progress 变化字符串集
	*/
	void init(const QString& titleLeft,
			  const QString& titleRight,
			  const QStringList& header,
			  const QStringList& value,
			  const QStringList& progress);

protected:
	void resizeEvent(QResizeEvent* eve);

private:
	enum
	{
		/** 标题高度
		*/
		TITLE_HEIGHT = 24,

		/** 分割线高度
		*/
		SEPARATOR_HEIGHT = 1,

		/** 上部或下部标题栏高度
		*/
		UP_DOWN_TITLE_HEIGHT = 23,

		/** 列数
		*/
		COLUMN = 4
	};

private:
	void onState(int32_t state);
	bool check();

private:
	Label* m_titleLeft;
	Label* m_titleRight;
	Label* m_separator;
	Label* m_upTitleOne;
	Label* m_upTitleTwo;
	Label* m_upTitleThree;
	Label* m_upTitleFour;
	Label* m_downTitleOne;
	Label* m_downTitleTwo;
	Label* m_downTitleThree;
	Label* m_downTitleFour;
	ContentLabel* m_upContentOne;
	ContentLabel* m_upContentTwo;
	ContentLabel* m_upContentThree;
	ContentLabel* m_upContentFour;
	ContentLabel* m_downContentOne;
	ContentLabel* m_downContentTwo;
	ContentLabel* m_downContentThree;
	ContentLabel* m_downContentFour;
	Label* m_upError;
	Label* m_downError;
};