#include "Cini.h"
#include <windows.h>
#include <vector>

Cini::Cini(const std::string& iniPath)
{
	m_iniPath = iniPath;
}

std::string Cini::ReadIni(const std::string& key, const std::string& strSection, int32_t maxLength) const
{
	if (maxLength == 0)
	{
		return "";
	}
	std::string text;
	text.resize(maxLength);
	GetPrivateProfileString(strSection.c_str(), key.c_str(), "", &text[0], maxLength, m_iniPath.c_str());
	text.resize(strlen(&text[0]));
	return text;
}

void Cini::WriteIni(const std::string& key, const std::string& text, const std::string& strSection) const
{
	WritePrivateProfileString(strSection.c_str(), key.c_str(), text.c_str(), m_iniPath.c_str());
}

std::vector<std::string> Cini::GetSection(int32_t maxSectionNum, int32_t maxLength) const
{
	std::vector<std::string> vecSection;
	int32_t iLength = 0;
	int32_t iPos = 0;
	//总长度
	uint32_t maxSectionLength = (uint32_t)maxSectionNum * (uint32_t)maxLength;
	//总的提出来的字符串
	TCHAR* chSectionNames = new TCHAR[maxSectionLength];
	//存放一个段名
	TCHAR* chSection = new TCHAR[maxLength];
	DWORD nChar = GetPrivateProfileSectionNames(chSectionNames, maxSectionLength, m_iniPath.c_str());
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

std::string Cini::GetFirstKey(const std::string& section, int32_t maxLength) const
{
	std::string result;
	result.resize(maxLength);
	GetPrivateProfileString(section.c_str(), nullptr, "-1", &result[0], maxLength, m_iniPath.c_str());
	result.resize(strlen(&result[0]));
	return result;
}

void Cini::DeleteKey(const std::string& key, const std::string& section) const
{
	WritePrivateProfileString(section.c_str(), key.c_str(), nullptr, m_iniPath.c_str());
}

void Cini::DeleteSection(const std::string& section) const
{
	WritePrivateProfileSection(section.c_str(), nullptr, m_iniPath.c_str());
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