#pragma once
#include "game_result_view.h"

class BattleDialogManager
{
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static BattleDialogManager& instance();

public:
	/** ��ʾս������
	@param [in] gameResult ��Ϸ���
	*/
	void showBattleDialog(const GameResultType::GameResult& gameResult);
};