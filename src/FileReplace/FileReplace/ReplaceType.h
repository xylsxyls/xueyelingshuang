#pragma once

enum ReplaceTypeEnum
{
	//�滻�����ļ���һ���������ļ�·��
	REPLACE_FILE,
	//�滻Ŀ¼�������ļ���һ��������Ŀ¼
	REPLACE_DIR,
	//�滻Ŀ¼��ƥ�䲿���ļ����������ļ���3��������Ŀ¼��ƥ����ļ������Ƿ����ִ�Сд��1��ʾ���֣�0��ʾ������
	REPLACE_FILES,
};

enum ReplaceCommandEnum
{
	//ƥ���滻������������ƥ���ַ������滻�ַ��������ִ�Сд
	REPLACE,
	//�в���ƥ�������滻�������������в���ƥ���ַ����������ַ��������ִ�Сд
	LINE,
	//�ļ���������3��������ƥ��Ĳ����ļ������滻�����ļ������ַ�����ƥ��Ĳ����ļ����Ƿ����ִ�Сд��1��ʾ���֣�0��ʾ������
	NAME,
	//�в���ƥ�䣬����ƥ���滻��3������������ƥ������ݣ�����ƥ����ַ������滻���ַ���
	PART
};

struct ReplaceParam
{
	//�滻����
	ReplaceTypeEnum m_replaceType;
	//�滻��ʽ
	ReplaceCommandEnum m_replaceCommand;
	//�����ļ�·��
	std::string m_filePath;
	//Ŀ¼
	std::string m_folder;
	//����ƥ����ļ���
	std::string m_fileName;
	//Ŀ¼����Ҫ�޸ĵ�ƥ���ļ����Ƿ����ִ�Сд��true��ʾ����
	bool m_isMatchCase;
	//ƥ����ַ���
	std::string m_oldStr;
	//�滻���ַ���
	std::string m_newStr;
	//�滻�������ַ���
	std::string m_newLine;
	//ƥ����ļ���
	std::string m_oldName;
	//�滻�ļ���ƥ�䲿�ֵ��ַ���
	std::string m_newName;
	//�滻ʱƥ����ļ����Ƿ����ִ�Сд��true��ʾ����
	bool m_isNameMatchCase;
	//���ڲ���ƥ������
	std::string m_oldPart;
	//��ʼ�滻������ţ���1��ʼ
	int32_t m_beginLine;
	//�����滻������ţ���1��ʼ
	int32_t m_endLine;

	/** ���캯��
	*/
	ReplaceParam()
	{
		m_replaceType = REPLACE_FILE;
		m_replaceCommand = REPLACE;
		m_isMatchCase = true;
		m_isNameMatchCase = true;
		m_beginLine = 0;
		m_endLine = 0;
	}
};

struct ReplaceNameParam
{
	//�����ļ�·��
	std::string m_filePath;
	//ƥ����ļ���
	std::string m_oldName;
	//�滻�ļ���ƥ�䲿�ֵ��ַ���
	std::string m_newName;
	//�滻ʱƥ����ļ����Ƿ����ִ�Сд��true��ʾ����
	bool m_isNameMatchCase;

	/** ���캯��
	*/
	ReplaceNameParam()
	{
		m_isNameMatchCase = true;
	}
};

struct ReplaceContentParam
{
	//�滻��ʽ
	ReplaceCommandEnum m_replaceCommand;
	//�����ļ�·��
	std::string m_filePath;
	//ƥ����ַ���
	std::string m_oldStr;
	//�滻���ַ���
	std::string m_newStr;
	//�滻�������ַ���
	std::string m_newLine;
	//���ڲ���ƥ������
	std::string m_oldPart;
	//��ʼ�滻������ţ���1��ʼ
	int32_t m_beginLine;
	//�����滻������ţ���1��ʼ
	int32_t m_endLine;
};