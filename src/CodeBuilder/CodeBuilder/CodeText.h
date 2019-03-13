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

	//注释，类型，名字，参数列表或初始化值或不填就是不初始化，返回是否是指针
	bool addVar(const std::string& varString);

	//添加删除
	void addDestruct(const std::string& name);

	//注释，函数名，返回注释或不填，返回类型或不填，默认无参
	void addFun(const std::string& funString);

	void clearAdd();

	void save();

protected:
	//添加初始化列表的冒号
	void addStructColon();

	//添加变量
	void addVar(const std::string& note, const std::string& type, std::string& name, bool isPtr);

	//添加初始化列表
	void addInitList(const std::string& name, const std::string& text);

	//添加初始化列表
	void addStruct(const std::string& type, std::string& name, const std::string& text = "");

	//添加检测
	void addCheck(std::string& name);

	//添加函数
	void addFun(const std::string& note, const std::string& funName, const std::string& returnNote, const std::string& returnType);

protected:
	bool check();

private:
	Ctxt* m_head;
	Ctxt* m_cpp;
	std::string m_className;
	std::string m_fatherName;
};