#pragma once
#include "BattleDialogBase.h"

class RPGFourWidget;
/** 四列战报
*/
class BattleThreeDialog : public BattleDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] 父窗口指针
	*/
	BattleThreeDialog(QWidget* parent = nullptr);

protected:
	virtual void initWidget();

private:
	bool check();
};