#include "Cini.h"
#include <windows.h>
#include <vector>

Cini::Cini(const string& iniPath){
	this->iniPath = iniPath;
}

string Cini::ReadIni(const string& key, const string& strSection, int MaxLength)const
{
	char *szText = new char[MaxLength];
	GetPrivateProfileString(strSection.c_str(),key.c_str(),"-1",szText,MaxLength,iniPath.c_str());
	string strText = szText;
	delete[] szText;
	return strText;
}

void Cini::WriteIni(const string& key, const string& text, const string& strSection)const
{
	WritePrivateProfileString(strSection.c_str(),key.c_str(),text.c_str(),iniPath.c_str());
	return;
}

std::vector<string> Cini::GetSection(int MaxSectionNum, int MaxLength)const
{
    std::vector<string> vecSection;
    int iLength = 0;
    int iPos = 0;
    //总长度
    unsigned int MaxSectionLength = (unsigned int)MaxSectionNum * MaxLength;
    //总的提出来的字符串
    TCHAR* chSectionNames = new TCHAR[MaxSectionLength];
    //存放一个段名
    TCHAR* chSection = new TCHAR[MaxLength];
    DWORD nChar = GetPrivateProfileSectionNames(chSectionNames, MaxSectionLength, iniPath.c_str());
    //以下循环，截断到两个连续的0 
    for (int i = 0; (DWORD)i < nChar; i++)
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

string Cini::GetFirstKey(const string& section, int MaxLength)const
{
    TCHAR* strBuffer = new TCHAR[MaxLength];
    GetPrivateProfileString(section.c_str(), NULL, "-1", strBuffer, MaxLength, iniPath.c_str());
    string result = strBuffer;
    delete[] strBuffer;
    return result;
}

void Cini::DeleteKey(const string& key, const string& section)const
{
    WritePrivateProfileString(section.c_str(), key.c_str(), NULL, iniPath.c_str());
}

void Cini::DeleteSection(const string& section)const
{
    WritePrivateProfileSection(section.c_str(), NULL, iniPath.c_str());
}

int main(){
    Cini ini("D:\\xueyelingshuang\\lib\\save.data");
    return 0;
}