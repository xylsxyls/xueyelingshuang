#include "BattleDialogManager.h"
#include "BattleDialog.h"
#include "LadderDialog.h"
#include "MingJiangDialog.h"
#include "game_result_view.h"

BattleDialogManager& BattleDialogManager::instance()
{
	static BattleDialogManager battleDialogHelper;
	return battleDialogHelper;
}

void BattleDialogManager::showBattleDialog(const GameResultType::GameResult& gameResult)
{
	switch (gameResult.type)
	{
	case GameResultType::ResultType_RPG:
	{
		BattleDialog* battleDialog = new BattleDialog;
		battleDialog->setGameResult(gameResult);
		battleDialog->show();
		break;
	}
	case GameResultType::ResultType_Ladder:
	{
		LadderDialog* ladderDialog = new LadderDialog;
		ladderDialog->show();
		break;
	}
	case GameResultType::ResultType_Battle:
	{
		LadderDialog* ladderDialog = new LadderDialog;
		ladderDialog->show();
		break;
	}
	case GameResultType::ResultType_MingJiang:
	{
		MingJiangDialog* mingJiangDialog = new MingJiangDialog;
		mingJiangDialog->show();
		break;
	}
	default:
		break;
	}
}