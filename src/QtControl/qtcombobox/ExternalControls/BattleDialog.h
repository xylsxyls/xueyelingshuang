#pragma once
#include "../CW3LModalFrame.h"

class Label;
class COriginalButton;
/** 战报外框
*/
class BattleDialog : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] 父窗口指针
	*/
	BattleDialog(QWidget* parent = nullptr);

	/** 初始化
	*/
	void init();

protected:
	void paintEvent(QPaintEvent* eve);
	void resizeEvent(QResizeEvent* eve);

private:
	bool check();

private:
	COriginalButton* m_exit;
	Label* m_logo;
	Label* m_title;
	COriginalButton* m_expand;
	QWidget* m_content;
};