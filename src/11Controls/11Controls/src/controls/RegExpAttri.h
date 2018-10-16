#pragma once
#include <QRegExp>
#include <QRegExpValidator>

namespace RE
{
	/** 正则表达式相关属性
	*/
	struct RegExpAttri
	{
		//是否修改完毕
		bool m_finished;
		//正则表达式管理类
		QRegExp m_regExp;
		//正则表达式代理类
		QRegExpValidator m_validator;
		//当前下拉框中文本框内内容
		QString m_currentText;
		//下拉框节点内容前缀
		QString m_preText;

		RegExpAttri()
		{
			m_finished = true;
		}

		RegExpAttri(const RegExpAttri& attri)
		{

		}

		RegExpAttri operator = (const RegExpAttri& attri)
		{
			return *this;
		}
	};
};