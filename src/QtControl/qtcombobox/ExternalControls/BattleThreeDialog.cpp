#include "BattleThreeDialog.h"
#include "RPGContentWidget.h"

BattleThreeDialog::BattleThreeDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_content = new RPGContentWidget(2, 3, this);
	//基类的初始化函数必须放在实例子类的构造函数中调用
	init();
	setDisplayMode(true);
	setState(RPGContentBase::ERROR_ALL);
	setLogo(false);
}

void BattleThreeDialog::initWidget()
{
	if (!check())
	{
		return;
	}

	QStringList headerThree;
	headerThree.push_back(QStringLiteral("屠龙"));
	headerThree.push_back(QStringLiteral("炫目"));
	headerThree.push_back(QStringLiteral("杀"));
	headerThree.push_back(QStringLiteral("怪兽杀"));
	headerThree.push_back(QStringLiteral("局杀"));
	headerThree.push_back(QStringLiteral("其他"));
	headerThree.push_back(QStringLiteral("其他"));
	headerThree.push_back(QStringLiteral("其他"));
	QStringList valueThree;
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	valueThree.push_back(QStringLiteral("999999"));
	QStringList progressThree;
	progressThree.push_back(QStringLiteral(""));
	progressThree.push_back(QStringLiteral("(+125)"));
	progressThree.push_back(QStringLiteral(""));
	progressThree.push_back(QStringLiteral(""));
	progressThree.push_back(QStringLiteral(""));
	progressThree.push_back(QStringLiteral("(-125)"));
	((RPGContentWidget*)m_content)->init(QStringLiteral("欧斯小清新欧耶"), QStringLiteral("拉尔李连杰打破"), headerThree, valueThree, progressThree);
}

bool BattleThreeDialog::check()
{
	return m_content != nullptr;
}
