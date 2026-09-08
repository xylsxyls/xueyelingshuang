#pragma once
#include <map>
#include <QRegExp>
#include <QRegExpValidator>
#include "RegExpAttri.h"
#include "QtControlsMacro.h"

class QComboBox;

/** 对QComboBox设置正则表达式
使用方法
setRegularExpression进行设置，父窗口要install，要链接QComboBox的两个信号
QObject::connect(htmlComboBox, &QComboBox::currentTextChanged, this, &InviteBattleDialog::onGameNetTextChanged);
QObject::connect(htmlComboBox->lineEdit(), &QLineEdit::editingFinished, this, &InviteBattleDialog::onNetEditTextFinish);
内部调用函数
onSetTextChanged(qobject_cast<QComboBox*>(sender()), text);
onSetEditTextFinish(qobject_cast<QComboBox*>(sender()));
继承eventFilter，内部调用RegExpEventFilter
*/
class QtControlsAPI RegExpBase
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
	@param [in] text 当前编辑内容
	*/
	void onSetTextChanged(QComboBox* comboBox, const QString& text);

private:
	/** 遍历已注册下拉框，处理鼠标按下进入编辑状态
	@param [in] target 事件目标对象
	*/
	void comboBoxPressMap(QObject* target);

	/** 处理单个下拉框鼠标按下时的文本前缀移除和编辑缓存恢复
	@param [in] target 事件目标对象
	@param [in] pBox 下拉框指针
	@param [in] addString 节点内容前缀
	@param [in,out] curText 当前编辑缓存文本
	@param [in,out] finish 当前编辑是否完成
	*/
	void comboBoxPress(QObject* target, QComboBox* pBox, const QString& addString, QString& curText, bool& finish);

	/** 遍历已注册下拉框，处理焦点离开时的编辑完成逻辑
	@param [in] target 事件目标对象
	*/
	void comboBoxFocusOutMap(QObject* target);

	/** 处理单个下拉框焦点离开时的文本前缀补全和状态恢复
	@param [in] target 事件目标对象
	@param [in] pBox 下拉框指针
	@param [in] addString 节点内容前缀
	@param [in,out] curText 当前编辑缓存文本
	@param [in,out] finish 当前编辑是否完成
	*/
	void comboBoxFocusOut(QObject* target, QComboBox* pBox, const QString& addString, QString& curText, bool& finish);

	/** 处理单个下拉框文本变化时的正则校验
	@param [in] text 当前编辑文本
	@param [in] pBox 下拉框指针
	@param [in] exp 正则表达式对象
	@param [in] addString 节点内容前缀
	@param [in,out] curText 当前编辑缓存文本
	*/
	void comboBoxTextChanged(const QString& text, QComboBox* pBox, const QRegExp& exp, const QString& addString, QString& curText);

	/** 处理单个下拉框编辑完成后的缓存更新
	@param [in] pBox 下拉框指针
	@param [in] addString 节点内容前缀
	@param [in,out] curText 当前编辑缓存文本
	@param [in,out] finish 当前编辑是否完成
	*/
	void comboBoxEditFinish(QComboBox* pBox, const QString& addString, QString& curText, bool& finish);

protected:
	// 下拉框和对应正则输入状态的映射
	std::map<QComboBox*, RE::RegExpAttri> m_mapAttri;
};