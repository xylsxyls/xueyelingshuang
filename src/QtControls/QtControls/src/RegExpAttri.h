#pragma once
#include <QRegExp>
#include <QRegExpValidator>

namespace RE
{
	/** ������ʽ�������
	*/
	struct RegExpAttri
	{
		//�Ƿ��޸����
		bool m_finished;
		//������ʽ������
		QRegExp m_regExp;
		//������ʽ������
		QRegExpValidator m_validator;
		//��ǰ���������ı���������
		QString m_currentText;
		//������ڵ�����ǰ׺
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