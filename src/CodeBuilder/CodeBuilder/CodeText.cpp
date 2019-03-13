#include "CodeText.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

CodeText::CodeText():
m_head(nullptr),
m_cpp(nullptr)
{
	m_fatherName = "SimpleFatherClassTest";
}

void CodeText::initSimpleClass(const std::string& className)
{
	if (m_head != nullptr || m_cpp != nullptr)
	{
		return;
	}

	m_className = className;
	m_head = new Ctxt("SimpleClassTest.h");
	m_head->LoadTxt(Ctxt::ONE_LINE);
	m_head->Replace("SimpleClassTest", m_className);
	m_cpp = new Ctxt("SimpleClassTest.cpp");
	m_cpp->LoadTxt(Ctxt::ONE_LINE);
	m_cpp->Replace("SimpleClassTest", m_className);
}

void CodeText::setFatherClass(const std::string& fatherClass)
{
	if (!check())
	{
		return;
	}

	m_fatherName = fatherClass;
	m_head->Replace("SimpleFatherClassTest", m_fatherName);
	m_cpp->Replace("SimpleFatherClassTest", m_fatherName);
}

bool CodeText::addVar(const std::string& varString)
{
	std::vector<std::string> vec = CStringManager::split(varString, " ");
	if (vec.size() < 3)
	{
		return false;
	}
	std::string note = vec[0];
	std::string type = vec[1];
	bool ptr = false;
	if (CStringManager::Replace(type, "*", "") != 0)
	{
		ptr = true;
	}
	std::string name = vec[2];
	std::string structText;
	int32_t index = 2;
	while (index++ != vec.size() - 1)
	{
		structText += (vec[index] + " ");
	}
	if (!structText.empty())
	{
		structText.pop_back();
	}

	addVar(note, type, name, ptr);
	addInitList(name, ptr ? "nullptr" : structText);
	if (ptr)
	{
		addStruct(type, name, structText);
		addCheck(name);
	}
	return ptr;
}

void CodeText::addVar(const std::string& note, const std::string& type, std::string& name, bool isPtr)
{
	for (auto it = m_head->m_vectxt.begin(); it != m_head->m_vectxt.end(); ++it)
	{
		if (CStringManager::Find((*it)[0], "#addVar") != -1)
		{
			std::vector<std::string> line;
			line.push_back("\t" + type + (isPtr ? "*" : "") + " " + name + ";");
			it = m_head->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\t//" + note);
			it = m_head->m_vectxt.insert(it, line);
			return;
		}
	}
}

void CodeText::addDestruct(const std::string& name)
{
	if (!check())
	{
		return;
	}

	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if ((*it)[0] == "#addDestruct")
		{
			std::vector<std::string> line;
			line.push_back("\tdelete " + name + ";");
			it = m_cpp->m_vectxt.insert(it, line);
			return;
		}
	}
}

void CodeText::addFun(const std::string& funString)
{
	std::vector<std::string> vec = CStringManager::split(funString, " ");
	if (vec.size() < 2)
	{
		return;
	}

	std::string note = vec[0];
	std::string funName = vec[1];
	std::string returnNote = "";
	std::string returnType = "void";
	if (vec.size() >= 4)
	{
		returnNote = vec[2];
		returnType = vec[3];
	}
	addFun(note, funName, returnNote, returnType);
}

void CodeText::addFun(const std::string& note, const std::string& funName, const std::string& returnNote, const std::string& returnType)
{
	for (auto it = m_head->m_vectxt.begin(); it != m_head->m_vectxt.end(); ++it)
	{
		if (CStringManager::Find((*it)[0], "#addFun") != -1)
		{
			std::vector<std::string> line;
			line.push_back("\t" + returnType + " " + funName + "();");
			it = m_head->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\t*/");
			it = m_head->m_vectxt.insert(it, line);
			if (!returnNote.empty())
			{
				line.clear();
				line.push_back("\t@return " + returnNote);
				it = m_head->m_vectxt.insert(it, line);
			}
			line.clear();
			line.push_back("\t@param [] ");
			it = m_head->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\t/** " + note);
			it = m_head->m_vectxt.insert(it, line);
			break;
		}
	}

	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if (CStringManager::Find((*it)[0], "#addFun") != -1)
		{
			std::vector<std::string> line;
			line.push_back("}");
			it = m_cpp->m_vectxt.insert(it, line);
			if (returnType != "void")
			{
				line.clear();
				line.push_back("\treturn " + returnType + "();");
				it = m_cpp->m_vectxt.insert(it, line);
			}
			line.clear();
			line.push_back("\t");
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\t}");
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back(returnType == "void" ? "\t\treturn;" : ("\t\treturn " + returnType + "();"));
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\t{");
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("\tif (!check())");
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back("{");
			it = m_cpp->m_vectxt.insert(it, line);
			line.clear();
			line.push_back(returnType + " " + m_className + "::" + funName + "()");
			it = m_cpp->m_vectxt.insert(it, line);
			break;
		}
	}
}

void CodeText::clearAdd()
{
	if (!check())
	{
		return;
	}

	for (auto it = m_head->m_vectxt.begin(); it != m_head->m_vectxt.end();)
	{
		if (CStringManager::Find((*it)[0], "#add") != -1)
		{
			it = m_head->m_vectxt.erase(it);
			continue;
		}
		++it;
	}
	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end();)
	{
		if (CStringManager::Find((*it)[0], "#add") != -1)
		{
			if (CStringManager::Find((*it)[0], "#addCheck") != -1)
			{
				--it;
				CStringManager::Replace((*(it))[0], " &&", "");
				(*(it))[0].push_back(';');
				++it;
			}
			else if (CStringManager::Find((*it)[0], "#addDestruct") != -1)
			{
				--it;
				if ((*(it))[0] == "{")
				{
					++it;
					(*(it))[0] = "\t";
					continue;
				}
				++it;
			}
			it = m_cpp->m_vectxt.erase(it);
			continue;
		}
		++it;
	}
}

void CodeText::save()
{
	if (!check())
	{
		return;
	}
	m_head->SaveAs(m_className + ".h");
	m_cpp->SaveAs(m_className + ".cpp");
}

void CodeText::addStructColon()
{
	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if (CStringManager::Find((*it)[0], "#addStructEnd") != -1)
		{
			++it;
			if ((*(it))[0].back() != ':')
			{
				(*(it))[0].push_back(':');
				return;
			}
		}
	}
}

void CodeText::addInitList(const std::string& name, const std::string& text)
{
	if (text.empty())
	{
		return;
	}
	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if (CStringManager::Find((*it)[0], "#addInitList") != -1)
		{
			--it;
			if ((*it)[0].back() != ':')
			{
				(*it)[0].push_back(',');
			}
			++it;
			std::vector<std::string> line;
			line.push_back(name + "(" + text + ")");
			it = m_cpp->m_vectxt.insert(it, line);
			return;
		}
	}
}

void CodeText::addStruct(const std::string& type, std::string& name, const std::string& text)
{
	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if ((*it)[0] == "#addStruct")
		{
			std::vector<std::string> line;
			line.push_back("\t" + name + " = new " + type + (text.empty() ? "" : "(" + text + ")") + ";");
			it = m_cpp->m_vectxt.insert(it, line);
			return;
		}
	}
}

void CodeText::addCheck(std::string& name)
{
	for (auto it = m_cpp->m_vectxt.begin(); it != m_cpp->m_vectxt.end(); ++it)
	{
		if ((*it)[0] == "#addCheck")
		{
			std::vector<std::string> line;
			line.push_back("\t\t" + name + " != nullptr &&");
			it = m_cpp->m_vectxt.insert(it, line);
			return;
		}
	}
}

bool CodeText::check()
{
	return m_head != nullptr && m_cpp != nullptr;
}