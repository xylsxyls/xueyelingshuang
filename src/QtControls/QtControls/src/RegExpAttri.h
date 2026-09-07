#pragma once
#include <QRegExp>
#include <QRegExpValidator>

namespace RE
{
	/** 下拉框正则输入状态，保存校验表达式、输入缓存和节点前缀
	*/
	struct RegExpAttri
	{
	public:
		// 当前编辑是否已经完成
		bool m_finished;
		// 正则表达式对象
		QRegExp m_regExp;
		// 正则表达式校验器
		QRegExpValidator m_validator;
		// 当前下拉框编辑文本
		QString m_currentText;
		// 下拉框节点内容前缀
		QString m_preText;

	public:
		/** 构造默认正则输入状态
		*/
		RegExpAttri()
		{
			m_finished = true;
		}

		/** 拷贝构造正则输入状态，校验器用源对象表达式重新初始化
		@param [in] attri 源正则输入状态
		*/
		RegExpAttri(const RegExpAttri& attri)
			: m_finished(attri.m_finished)
			, m_regExp(attri.m_regExp)
			, m_validator(attri.m_regExp)
			, m_currentText(attri.m_currentText)
			, m_preText(attri.m_preText)
		{
		}

		/** 复制正则输入状态，保持旧返回值签名以兼容已有代码
		@param [in] attri 源正则输入状态
		@return 返回复制后的当前对象
		*/
		RegExpAttri operator = (const RegExpAttri& attri)
		{
			if (this == &attri)
			{
				return *this;
			}
			m_finished = attri.m_finished;
			m_regExp = attri.m_regExp;
			m_validator.setRegExp(m_regExp);
			m_currentText = attri.m_currentText;
			m_preText = attri.m_preText;
			return *this;
		}
	};
};