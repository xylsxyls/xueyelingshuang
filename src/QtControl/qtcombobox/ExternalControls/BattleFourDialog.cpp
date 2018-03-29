#include "BattleFourDialog.h"
#include "RPGFourWidget.h"

BattleFourDialog::BattleFourDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_content = new RPGFourWidget(this);
	//基类的初始化函数必须放在实例子类的构造函数中调用
	init();
	setDisplayMode(true);
	setState(false);
	setLogo(true);
}

void BattleFourDialog::initWidget()
{
	if (!check())
	{
		return;
	}

	QStringList headerFour;
	headerFour.push_back(QStringLiteral("屠龙"));
	headerFour.push_back(QStringLiteral("炫目"));
	headerFour.push_back(QStringLiteral("玉树"));
	headerFour.push_back(QStringLiteral("杀"));
	headerFour.push_back(QStringLiteral("怪兽杀"));
	headerFour.push_back(QStringLiteral("局杀"));
	headerFour.push_back(QStringLiteral("逃跑"));
	headerFour.push_back(QStringLiteral("其他"));
	QStringList valueFour;
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	QStringList progressFour;
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral("(+125)"));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral("(+125)"));
	progressFour.push_back(QStringLiteral("(-125)"));
	((RPGFourWidget*)m_content)->init(QStringLiteral("欧斯小清新欧耶"), QStringLiteral("拉尔李连杰打破"), headerFour, valueFour, progressFour);
}

bool BattleFourDialog::check()
{
	return m_content != nullptr;
}
