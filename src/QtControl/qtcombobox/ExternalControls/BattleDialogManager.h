#pragma once
#include "game_result_view.h"

class BattleDialogBase;
/** ս���������
*/
class BattleDialogManager
{
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static BattleDialogManager& instance();

private:
	BattleDialogManager();

public:
	/** ��ʾս������
	@param [in] gameResult ��Ϸ���
	*/
	void showBattleDialog(const GameResultType::GameResult& gameResult);

private:
	BattleDialogBase* m_dialog;
};