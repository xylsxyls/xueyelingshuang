#pragma once
#include "BattleDialogBase.h"

class RPGFourWidget;
/** ����ս��
*/
class BattleFourDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] ������ָ��
	*/
	BattleFourDialog(QWidget* parent = nullptr);

protected:
	virtual void initWidget();

private:
	bool check();
};