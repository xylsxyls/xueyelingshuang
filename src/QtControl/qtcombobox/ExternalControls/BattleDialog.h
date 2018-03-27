#pragma once
#include "../CW3LModalFrame.h"

class Label;
class COriginalButton;
/** ս�����
*/
class BattleDialog : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ������ָ��
	*/
	BattleDialog(QWidget* parent = nullptr);

	/** ��ʼ��
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