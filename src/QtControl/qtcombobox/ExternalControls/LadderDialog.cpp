#include "LadderDialog.h"
#include "RPGContentWidget.h"
#include "GameResultListPanelMini.h"
#include "GameResultListPanelMax.h"
#include "Label.h"

LadderDialog::LadderDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_logoInWidth = 22;
	m_content = new GameResultListPanelMini_Ladder(this);
	m_bigContent = new GameResultListPanelMax(this);
	QObject::connect(this, &BattleDialogBase::resizeDialog, this, &LadderDialog::onResizeDialog);
	QObject::connect(this, &BattleDialogBase::setGameResultSignal, this, &LadderDialog::onSetGameResultSignal);
	//基类的初始化函数必须放在实例子类的构造函数中调用
	BattleDialogBase::init();
	setBattleState(SMALL_NORMAL);
	setLogo(true);
}

void LadderDialog::setBattleState(BattleState state)
{
	switch (state)
	{
	case LadderDialog::SMALL_NORMAL:
	{
		setDisplayMode(true);
		setState(NORMAL);
		break;
	}
	case LadderDialog::SMALL_ERROR_VALUE:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		break;
	}
	case LadderDialog::SMALL_ERROR_ALL:
	{
		setDisplayMode(true);
		setState(ERROR_ALL);
		break;
	}
	case LadderDialog::BIG_NORMAL:
	{
		setDisplayMode(false);
		setState(NORMAL);
		break;
	}
	default:
		break;
	}
}

void LadderDialog::onResizeDialog()
{
	int32_t dialogWidth = NORMAL_WIDTH + m_logoInWidth;
	int32_t dialogHeight = NORMAL_HEIGHT;

	if (m_isSmall == false)
	{
		dialogWidth = BIG_DIALOG_WIDTH;
		dialogHeight = BIG_DIALOG_HEIGHT;
	}
	resize(dialogWidth, dialogHeight);
}

void LadderDialog::onSetGameResultSignal(const GameResultType::GameResult& gameResult)
{
	for (auto itCamp = gameResult.campList.begin(); itCamp != gameResult.campList.end(); ++itCamp)
	{
		auto& slotList = itCamp->slotList;
		for (auto itSlot = slotList.begin(); itSlot != slotList.end(); ++itSlot)
		{
			if (itSlot->isMe)
			{
				setLogo(itCamp->isVictory);
				if (gameResult.invalidGame)
				{
					setBattleState(SMALL_ERROR_ALL);
				}
				else if (gameResult.dataError)
				{
					setBattleState(SMALL_ERROR_VALUE);
				}
				else
				{
					setBattleState(SMALL_NORMAL);
				}
				break;
			}
		}
	}
}
