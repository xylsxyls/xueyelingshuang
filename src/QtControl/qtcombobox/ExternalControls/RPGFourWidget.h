#pragma once
#include <QWidget>

class Label;
class ContentLabel;
/** RPG四列战报显示
*/
class RPGFourWidget : public QWidget
{
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


	};
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGFourWidget(QWidget* parent = nullptr);

public:
	/** 初始化
	@param [in] header 标题字符串集
	@param [in] value 内容字符串集
	@param [in] progress 变化字符串集
	*/
	void init(const QStringList& header,const QStringList& value, const QStringList& progress);

protected:
	void resizeEvent(QResizeEvent* eve);

private:
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