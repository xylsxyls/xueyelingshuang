#include "MingJiangDialog.h"
#include "RPGContentWidget.h"
#include "GameResultListPanelMini.h"
#include "GameResultListPanelMax.h"
#include "Label.h"

MingJiangDialog::MingJiangDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_logoInWidth = 22;
	m_content = new GameResultListPanelMini_MingJiang(this);
	m_bigContent = new GameResultListPanelMax(this);
	QObject::connect(this, &BattleDialogBase::resizeDialog, this, &MingJiangDialog::onResizeDialog);
	QObject::connect(this, &BattleDialogBase::setGameResultSignal, this, &MingJiangDialog::onSetGameResultSignal);
	//����ĳ�ʼ�������������ʵ������Ĺ��캯���е���
	BattleDialogBase::init();
	setBattleState(SMALL_NORMAL);
	setLogo(true);
}

void MingJiangDialog::setBattleState(BattleState state)
{
	switch (state)
	{
	case MingJiangDialog::SMALL_NORMAL:
	{
		setDisplayMode(true);
		setState(NORMAL);
		break;
	}
	case MingJiangDialog::SMALL_ERROR_VALUE:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		break;
	}
	case MingJiangDialog::SMALL_ERROR_ALL:
	{
		setDisplayMode(true);
		setState(ERROR_ALL);
		break;
	}
	case MingJiangDialog::BIG_NORMAL:
	{
		setDisplayMode(false);
		setState(NORMAL);
		break;
	}
	default:
		break;
	}
}

void MingJiangDialog::onResizeDialog()
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

void MingJiangDialog::onSetGameResultSignal(const GameResultType::GameResult& gameResult)
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