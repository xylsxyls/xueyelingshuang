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

BattleDialogManager::BattleDialogManager():
m_dialog(nullptr)
{

}

void BattleDialogManager::showBattleDialog(const GameResultType::GameResult& gameResult)
{
	if (m_dialog != nullptr)
	{
		delete m_dialog;
		m_dialog = nullptr;
	}
	switch (gameResult.type)
	{
	case GameResultType::ResultType_RPG:
	{
		m_dialog = new BattleDialog;
		break;
	}
	case GameResultType::ResultType_Ladder:
	case GameResultType::ResultType_Battle:
	{
		m_dialog = new LadderDialog;
		break;
	}
	case GameResultType::ResultType_MingJiang:
	{
		m_dialog = new MingJiangDialog;
		break;
	}
	default:
		break;
	}

	if (m_dialog == nullptr)
	{
		return;
	}

	m_dialog->setGameResult(gameResult);
	m_dialog->show();
}