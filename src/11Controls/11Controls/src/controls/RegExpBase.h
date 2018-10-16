#pragma once
#include <QRegExp>
#include <QRegExpValidator>
#include "RegExpAttri.h"
#include "ControlsMacro.h"

class QComboBox;

/** ��QComboBox����������ʽ
ʹ�÷���
setRegularExpression�������ã�������Ҫinstall��Ҫ����QComboBox�������ź�
QObject::connect(htmlComboBox, &QComboBox::currentTextChanged, this, &InviteBattleDialog::onGameNetTextChanged);
QObject::connect(htmlComboBox->lineEdit(), &QLineEdit::editingFinished, this, &InviteBattleDialog::onNetEditTextFinish);
�ڲ����ú���
onSetTextChanged((QComboBox*)sender(), text);
onSetEditTextFinish((QComboBox*)sender());
�̳�eventFilter���ڲ�����RegExpEventFilter
*/
class ControlsAPI RegExpBase
{
protected:
	/** ����������ʽ
	@param [in] comboBox ������
	@param [in] regularStr ������ʽ
	@param [in] preText �ڵ�����ǰ׺
	*/
	void setRegularExpression(QComboBox* comboBox, const QString& regularStr, const QString& preText);

	/** ����eventFilter����
	@param [in] target ��Ӱ�����
	@param [in] eve �¼�
	*/
	void RegExpEventFilter(QObject* target, QEvent* eve);

	/** ����ComboBox���ݸı���ɺ�ĺ���
	@param [in] comboBox ������
	*/
	void onSetEditTextFinish(QComboBox* comboBox);

	/** ����ComboBox���ݸı��ĺ���
	@param [in] comboBox ������
	@param [in] ��ǰ�༭����
	*/
	void onSetTextChanged(QComboBox* comboBox, const QString& text);

private:
	void comboBoxPressMap(QObject* target);
	void comboBoxPress(QObject* target, QComboBox* pBox, const QString& addString, QString& curText, bool& finish);
	void comboBoxFocusOutMap(QObject* target);
	void comboBoxFocusOut(QObject* target, QComboBox* pBox, const QString& addString, QString& curText, bool& finish);
	void comboBoxTextChanged(const QString& text, QComboBox* pBox, const QRegExp& exp, const QString& addString, QString& curText);
	void comboBoxEditFinish(QComboBox* pBox, const QString& addString, QString& curText, bool& finish);

protected:
	std::map<QComboBox*, RE::RegExpAttri> m_mapAttri;
};