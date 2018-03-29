#include "BattleThreeDialog.h"
#include "RPGThreeWidget.h"

BattleThreeDialog::BattleThreeDialog(QWidget* parent) :
BattleDialogBase(parent)
{
	m_content = new RPGThreeWidget(this);
	//����ĳ�ʼ�������������ʵ������Ĺ��캯���е���
	init();
	setDisplayMode(true);
	setState(true);
	setLogo(false);
}

void BattleThreeDialog::initWidget()
{
	if (!check())
	{
		return;
	}

	QStringList headerThree;
	headerThree.push_back(QStringLiteral("����"));
	headerThree.push_back(QStringLiteral("��Ŀ"));
	headerThree.push_back(QStringLiteral("ɱ"));
	headerThree.push_back(QStringLiteral("����ɱ"));
	headerThree.push_back(QStringLiteral("��ɱ"));
	headerThree.push_back(QStringLiteral("����"));
	QStringList valueThree;
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
	((RPGThreeWidget*)m_content)->init(QStringLiteral("ŷ˹С����ŷҮ"), QStringLiteral("���������ܴ���"), headerThree, valueThree, progressThree);
}

bool BattleThreeDialog::check()
{
	return m_content != nullptr;
}
