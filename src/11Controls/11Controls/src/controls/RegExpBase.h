#pragma once
#include <QRegExp>
#include <QRegExpValidator>
#include "RegExpAttri.h"
#include "ControlsMacro.h"

class QComboBox;

/** 对QComboBox设置正则表达式
使用方法
setRegularExpression进行设置，父窗口要install，要链接QComboBox的两个信号
QObject::connect(htmlComboBox, &QComboBox::currentTextChanged, this, &InviteBattleDialog::onGameNetTextChanged);
QObject::connect(htmlComboBox->lineEdit(), &QLineEdit::editingFinished, this, &InviteBattleDialog::onNetEditTextFinish);
内部调用函数
onSetTextChanged((QComboBox*)sender(), text);
onSetEditTextFinish((QComboBox*)sender());
继承eventFilter，内部调用RegExpEventFilter
*/
class ControlsAPI RegExpBase
{
protected:
	/** 设置正则表达式
	@param [in] comboBox 下拉框
	@param [in] regularStr 正则表达式
	@param [in] preText 节点内容前缀
	*/
	void setRegularExpression(QComboBox* comboBox, const QString& regularStr, const QString& preText);

	/** 处理eventFilter函数
	@param [in] target 被影响的类
	@param [in] eve 事件
	*/
	void RegExpEventFilter(QObject* target, QEvent* eve);

	/** 处理ComboBox内容改变完成后的函数
	@param [in] comboBox 下拉框
	*/
	void onSetEditTextFinish(QComboBox* comboBox);

	/** 处理ComboBox内容改变后的函数
	@param [in] comboBox 下拉框
	@param [in] 当前编辑内容
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