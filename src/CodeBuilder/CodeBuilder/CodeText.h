#pragma once
#include <string>

class Ctxt;

class CodeText
{
public:
	CodeText();

public:
	void initSimpleClass(const std::string& className);

	void setFatherClass(const std::string& fatherClass);

	//ע�ͣ����ͣ����֣������б���ʼ��ֵ������ǲ���ʼ���������Ƿ���ָ��
	bool addVar(const std::string& varString);

	//���ɾ��
	void addDestruct(const std::string& name);

	//ע�ͣ�������������ע�ͻ���������ͻ��Ĭ���޲�
	void addFun(const std::string& funString);

	void clearAdd();

	void save();

protected:
	//��ӳ�ʼ���б��ð��
	void addStructColon();

	//��ӱ���
	void addVar(const std::string& note, const std::string& type, std::string& name, bool isPtr);

	//��ӳ�ʼ���б�
	void addInitList(const std::string& name, const std::string& text);

	//��ӳ�ʼ���б�
	void addStruct(const std::string& type, std::string& name, const std::string& text = "");

	//��Ӽ��
	void addCheck(std::string& name);

	//��Ӻ���
	void addFun(const std::string& note, const std::string& funName, const std::string& returnNote, const std::string& returnType);

protected:
	bool check();

private:
	Ctxt* m_head;
	Ctxt* m_cpp;
	std::string m_className;
	std::string m_fatherName;
};