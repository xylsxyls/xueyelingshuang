#pragma once
#include "game_result_view.h"

class BattleDialogBase;
/** 战报框管理类
*/
class BattleDialogManager
{
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static BattleDialogManager& instance();

private:
	BattleDialogManager();

public:
	/** 显示战报窗口
	@param [in] gameResult 游戏结果
	*/
	void showBattleDialog(const GameResultType::GameResult& gameResult);

private:
	BattleDialogBase* m_dialog;
};