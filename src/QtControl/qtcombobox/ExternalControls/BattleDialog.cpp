#include "BattleDialog.h"
#include "RPGContentWidget.h"

BattleDialog::BattleDialog(int32_t row, int32_t column, QWidget* parent) :
BattleDialogBase(parent)
{
	m_logoInWidth = 22;
	m_content = new RPGContentWidget(row, column, this);
	QObject::connect(this, &BattleDialogBase::resizeDialog, this, &BattleDialog::onResizeDialog);
	//基类的初始化函数必须放在实例子类的构造函数中调用
	BattleDialogBase::init();
	setBattleState(SMALL_NORMAL);
	setLogo(true);
}

void BattleDialog::init(const QString& titleLeft,
						const QString& titleRight,
						const QStringList& header,
						const QStringList& value,
						const QStringList& progress)
{
	m_titleLeft = titleLeft;
	m_titleRight = titleRight;
	m_header = header;
	m_value = value;
	m_progress = progress;
	((RPGContentWidget*)m_content)->init(m_titleLeft, m_titleRight, m_header, m_value, m_progress);
}

void BattleDialog::setBattleState(BattleState state)
{
	switch (state)
	{
	case BattleDialog::SMALL_NORMAL:
	{
		setDisplayMode(true);
		setState(NORMAL);
		setContentState(RPGContentBase::NORMAL);
		break;
	}
	case BattleDialog::SMALL_ERROR_VALUE:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		setContentState(RPGContentBase::ERROR_VALUE);
		break;
	}
	case BattleDialog::SMALL_ERROR_HEADER:
	{
		setDisplayMode(true);
		setState(ERROR_PART);
		setContentState(RPGContentBase::ERROR_HEADER);
		break;
	}
	case BattleDialog::SMALL_ERROR_ALL:
	{
		setDisplayMode(true);
		setState(ERROR_ALL);
		setContentState(RPGContentBase::ERROR_ALL);
		break;
	}
	case BattleDialog::BIG_NORMAL:
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

void BattleDialog::onResizeDialog()
{
	int32_t dialogWidth = 0;
	int32_t dialogHeight = 0;
	switch (m_state)
	{
	case NORMAL:
	{
		dialogWidth = NORMAL_WIDTH;
		dialogHeight = NORMAL_HEIGHT;
		break;
	}
	case ERROR_PART:
	{
		switch (m_contentState)
		{
		case RPGContentBase::ERROR_VALUE:
		{
			dialogWidth = NORMAL_WIDTH;
			dialogHeight = NORMAL_HEIGHT;
			break;
		}
		case RPGContentBase::ERROR_HEADER:
		{
			dialogWidth = ERROR_WIDTH;
			dialogHeight = ERROR_HEIGHT;
			break;
		}
		default:
			break;
		}
		break;
	}
	case ERROR_ALL:
	{
		dialogWidth = ERROR_WIDTH;
		dialogHeight = ERROR_HEIGHT;
		break;
	}
	default:
		break;
	}

	if (m_isSmall == false)
	{
		dialogWidth = BIG_DIALOG_WIDTH;
		dialogHeight = BIG_DIALOG_HEIGHT;
	}
	resize(dialogWidth, dialogHeight);
}

bool BattleDialog::check()
{
	return m_content != nullptr;
}
