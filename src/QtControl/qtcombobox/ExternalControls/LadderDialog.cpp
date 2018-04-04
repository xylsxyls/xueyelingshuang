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
		setContentState(RPGContentBase::NORMAL);
		break;
	}
	case LadderDialog::SMALL_ERROR_VALUE:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		setContentState(RPGContentBase::ERROR_VALUE);
		break;
	}
	case LadderDialog::SMALL_ERROR_HEADER:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		setContentState(RPGContentBase::ERROR_HEADER);
		break;
	}
	case LadderDialog::SMALL_ERROR_ALL:
	{
		setDisplayMode(true);
		setState(ERROR_ALL);
		setContentState(RPGContentBase::ERROR_ALL);
		break;
	}
	case LadderDialog::BIG_NORMAL:
	{
		setDisplayMode(false);
		setState(NORMAL);
		setContentState(RPGContentBase::NORMAL);
		break;
	}
	default:
		break;
	}
}

void LadderDialog::onResizeDialog()
{
	int32_t dialogWidth = NORMAL_WIDTH;
	int32_t dialogHeight = NORMAL_HEIGHT;

	if (m_isSmall == false)
	{
		dialogWidth = BIG_DIALOG_WIDTH;
		dialogHeight = BIG_DIALOG_HEIGHT;
	}
	resize(dialogWidth, dialogHeight);
}