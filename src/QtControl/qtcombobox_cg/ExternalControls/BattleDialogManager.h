#pragma once
#include "game_result_view.h"

class BattleDialogManager
{
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static BattleDialogManager& instance();

public:
	/** 显示战报窗口
	@param [in] gameResult 游戏结果
	*/
	void showBattleDialog(const GameResultType::GameResult& gameResult);
};