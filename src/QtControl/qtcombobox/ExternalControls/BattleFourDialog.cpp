#include "BattleFourDialog.h"
#include "RPGFourWidget.h"

BattleFourDialog::BattleFourDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_content = new RPGFourWidget(this);
	//����ĳ�ʼ�������������ʵ������Ĺ��캯���е���
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
	headerFour.push_back(QStringLiteral("����"));
	headerFour.push_back(QStringLiteral("��Ŀ"));
	headerFour.push_back(QStringLiteral("����"));
	headerFour.push_back(QStringLiteral("ɱ"));
	headerFour.push_back(QStringLiteral("����ɱ"));
	headerFour.push_back(QStringLiteral("��ɱ"));
	headerFour.push_back(QStringLiteral("����"));
	headerFour.push_back(QStringLiteral("����"));
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
	((RPGFourWidget*)m_content)->init(QStringLiteral("ŷ˹С����ŷҮ"), QStringLiteral("���������ܴ���"), headerFour, valueFour, progressFour);
}

bool BattleFourDialog::check()
{
	return m_content != nullptr;
}
