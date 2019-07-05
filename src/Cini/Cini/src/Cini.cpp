#include "Cini.h"
#include <windows.h>
#include <vector>
#include "SimpleIni.h"

Cini::Cini(const std::string& iniPath, bool useSimpleIni, bool isUtf8):
m_useSimpleIni(false),
m_isUtf8(true),
m_length(1024),
m_sectionNum(1024),
m_simpleIni(nullptr),
m_loadSuccess(false)
{
	m_iniPath = iniPath;
	m_useSimpleIni = useSimpleIni;
	m_isUtf8 = isUtf8;
	if (m_useSimpleIni)
	{
		m_simpleIni = new CSimpleIni(isUtf8);
		if (m_simpleIni != nullptr)
		{
			m_loadSuccess = (m_simpleIni->LoadFile(iniPath.c_str()) == SI_OK);
		}
	}
}

void Cini::setMaxLength(int32_t length)
{
	m_length = length;
}

void Cini::setMaxSection(int32_t sectionNum)
{
	m_sectionNum = sectionNum;
}

std::string Cini::readIni(const std::string& key, const std::string& section) const
{
	if (m_loadSuccess)
	{
		return m_simpleIni->GetValue(section.c_str(), key.c_str(), "");
	}

	if (m_length == 0)
	{
		return "";
	}
	std::string value;
	value.resize(m_length);
	GetPrivateProfileStringA(section.c_str(), key.c_str(), "", &value[0], m_length, m_iniPath.c_str());
	value.resize(strlen(&value[0]));
	return value;
}

void Cini::writeIni(const std::string& key, const std::string& value, const std::string& section) const
{
	if (m_loadSuccess)
	{
		m_simpleIni->SetValue(section.c_str(), key.c_str(), value.c_str(), nullptr, true);
		m_simpleIni->SaveFile(m_iniPath.c_str(), m_isUtf8);
		return;
	}

	WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), m_iniPath.c_str());
}

std::vector<std::string> Cini::getAllSection() const
{
	if (m_loadSuccess)
	{
		CSimpleIniTempl<char, SI_NoCase<char>, SI_ConvertA<char>>::TNamesDepend sections;
		m_simpleIni->GetAllSections(sections);
		std::vector<std::string> result;
		for (auto it = sections.begin(); it != sections.end(); ++it)
		{
			result.push_back(it->pItem);
		}
		return result;
	}

	std::vector<std::string> vecSection;
	int32_t iLength = 0;
	int32_t iPos = 0;
	//总长度
	uint32_t maxSectionLength = (uint32_t)m_sectionNum * (uint32_t)m_length;
	//总的提出来的字符串
	CHAR* chSectionNames = new CHAR[maxSectionLength];
	//存放一个段名
	CHAR* chSection = new CHAR[m_length];
	DWORD nChar = GetPrivateProfileSectionNamesA(chSectionNames, maxSectionLength, m_iniPath.c_str());
	//以下循环，截断到两个连续的0
	for (int32_t i = 0; (DWORD)i < nChar; i++)
	{
		if (chSectionNames[i] == 0)
		{
			chSection[iLength] = 0;
			vecSection.push_back(chSection);
			if (chSectionNames[i] == chSectionNames[i + 1])
			{
				break;
			}
			memset(chSection, 0, iLength);
			iPos = 0;
			iLength = 0;
		}
		else
		{
			chSection[iPos++] = chSectionNames[i];
			iLength++;
		}
	}
	delete[] chSectionNames;
	delete[] chSection;
	return vecSection;
}

std::vector<std::string> Cini::getAllKey(const std::string& section) const
{
	if (m_loadSuccess)
	{
		CSimpleIniTempl<char, SI_NoCase<char>, SI_ConvertA<char>>::TNamesDepend sections;
		m_simpleIni->GetAllKeys(section.c_str(), sections);
		std::vector<std::string> result;
		for (auto it = sections.begin(); it != sections.end(); ++it)
		{
			result.push_back(it->pItem);
		}
		return result;
	}
	return std::vector<std::string>();
}

std::string Cini::getFirstKey(const std::string& section) const
{
	if (m_loadSuccess)
	{
		CSimpleIniTempl<char, SI_NoCase<char>, SI_ConvertA<char>>::TNamesDepend sections;
		m_simpleIni->GetAllKeys(section.c_str(), sections);
		if (sections.empty())
		{
			return "";
		}
		return sections.begin()->pItem;
	}

	std::string result;
	result.resize(m_length);
	GetPrivateProfileStringA(section.c_str(), nullptr, "", &result[0], m_length, m_iniPath.c_str());
	result.resize(strlen(&result[0]));
	return result;
}

void Cini::deleteKey(const std::string& key, const std::string& section) const
{
	if (m_loadSuccess)
	{
		m_simpleIni->Delete(section.c_str(), key.c_str());
		m_simpleIni->SaveFile(m_iniPath.c_str(), m_isUtf8);
		return;
	}
	WritePrivateProfileStringA(section.c_str(), key.c_str(), nullptr, m_iniPath.c_str());
}

void Cini::deleteSection(const std::string& section) const
{
	if (m_loadSuccess)
	{
		std::vector<std::string> vecKeys = getAllKey(section);
		int32_t index = -1;
		while (index++ != vecKeys.size() - 1)
		{
			m_simpleIni->Delete(section.c_str(), vecKeys[index].c_str(), true);
		}
		m_simpleIni->SaveFile(m_iniPath.c_str(), m_isUtf8);
		return;
	}
	WritePrivateProfileSectionA(section.c_str(), nullptr, m_iniPath.c_str());
}

//int32_t main()
//{
//	Cini ini("D:\\xueyelingshuang\\lib\\save.ini");
//	ini.WriteIni("sad", "1234a");
//	std::string str = ini.ReadIni("sad");
//	printf("str = %s, len = %d\n", str.c_str(), str.length());
//	std::string key = ini.GetFirstKey();
//	printf("key = %s\n", key.c_str());
//	std::vector<std::string> vecSection = ini.GetSection();
//	printf("vec.size = %d, vec[0] = %s\n", vecSection.size(), vecSection[0].c_str());
//	getchar();
//	return 0;
//}