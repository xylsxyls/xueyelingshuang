#pragma once
#include "BattleDialogBase.h"

class RPGFourWidget;
/** ����ս��
*/
class BattleThreeDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ������ָ��
	*/
	BattleThreeDialog(QWidget* parent = nullptr);

protected:
	virtual void initWidget();

private:
	bool check();
};