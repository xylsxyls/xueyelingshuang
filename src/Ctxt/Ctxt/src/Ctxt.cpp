#include "Ctxt.h"
#include <string>
#include <fstream>
#include <stdarg.h>

Ctxt::Ctxt(const std::string& strPath)
{
	this->m_strPath = strPath;
	m_txt = new std::ofstream(strPath.c_str(), std::ios::app);
	m_pFile = NULL;
}

Ctxt::~Ctxt()
{
	delete (std::ofstream*)m_txt;
}

void Split(const std::string& src, const std::string& separate_character, std::vector<std::string>& strs)
{
	strs.clear();
	//分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	int32_t separate_characterLen = (int32_t)separate_character.size();
	int32_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)src.find(separate_character,lastPosition)))
	{
		strs.push_back(src.substr(lastPosition,index - lastPosition));   
		lastPosition = index + separate_characterLen;   
	}
	//截取最后一个分隔符后的内容
	std::string lastString = src.substr(lastPosition);
	//if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(lastString);
	return;
}

void Ctxt::LoadTxt(int32_t flag, const std::string& strSplit)
{
	m_vectxt.clear();
	switch (flag)
	{
	case POINT_TO_POINT:
	{
		LoadTxtWithPointToPoint(strSplit);
		break;
	}
	case SPLIT:
	{
		LoadTxtWithSplit(strSplit);
		break;
	}
	case ONE_LINE:
	{
		LoadTxtWithOneLine();
		break;
	}
	default:
		break;
	}
	
	return;
}

void Ctxt::Save()
{
	OpenFile_w();
	bool inWhile = true;
	int32_t lineIndex = -1;
	int32_t partIndex = -1;
	std::string strLine;
	while (inWhile)
	{
		++lineIndex;
		strLine.clear();
		partIndex = -1;
		std::vector<std::string>& vecLine = m_vectxt[lineIndex];
		while (partIndex++ != (int32_t)vecLine.size() - 1)
		{
			strLine.append(vecLine[partIndex]);
		}
		if (lineIndex == (int32_t)m_vectxt.size() - 1)
		{
			break;
		}
		AddWriteLine("%s", strLine.c_str());
	}
	::fprintf(m_pFile, "%s", strLine.c_str());
	CloseFile();
	return;
}

bool Ctxt::OpenFile_w()
{
	m_pFile = ::fopen(m_strPath.c_str(), "w+");
	if (m_pFile == NULL) return 0;
	return 1;
}

bool Ctxt::OpenFile_a()
{
	m_pFile = ::fopen(m_strPath.c_str(), "a+");
	if(m_pFile == NULL) return 0;
	return 1;
}

void Ctxt::AddWriteLine(const char* fmt, ...)
{
	std::string result;
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	//?即便分配了足够内存，长度必须加1，否则会崩溃
	vsprintf_s(&result[0], size + 1, fmt, args);
	va_end(args);

	::fprintf(m_pFile, "%s\n", result.c_str());

	return;
}

void Ctxt::CloseFile()
{
	::fclose(m_pFile);
	m_pFile = NULL;
	return;
}

void Ctxt::AddLine(const char* fmt, ...)
{
	std::string result;
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	//?即便分配了足够内存，长度必须加1，否则会崩溃
	vsprintf_s(&result[0], size + 1, fmt, args);
	va_end(args);

    *(std::ofstream*)m_txt << result << std::endl;
	return;
}

void Ctxt::SaveAs(const std::string& path)
{
	m_pFile = ::fopen(path.c_str(), "w+");
	bool inWhile = true;
	int32_t lineIndex = -1;
	int32_t partIndex = -1;
	std::string strLine;
	while (inWhile)
	{
		++lineIndex;
		strLine.clear();
		partIndex = -1;
		std::vector<std::string>& vecLine = m_vectxt[lineIndex];
		while (partIndex++ != (int32_t)vecLine.size() - 1)
		{
			strLine.append(vecLine[partIndex]);
		}
		if (lineIndex == (int32_t)m_vectxt.size() - 1)
		{
			break;
		}
		AddWriteLine("%s", strLine.c_str());
	}
	::fprintf(m_pFile, "%s", strLine.c_str());
	::fclose(m_pFile);
	m_pFile = NULL;
}

void Ctxt::LoadTxtWithPointToPoint(const std::string& strSplit)
{
	std::vector<std::vector<int32_t>> vecSplit;
	std::vector<std::string> vecPoint1ToPoint2;
	Split(strSplit, ",", vecPoint1ToPoint2);
	std::vector<int32_t> vecPointInt;
	std::vector<std::string> vecPoint;
	//分出得到第一对截取点，第0个和第2个就是首和尾
	int32_t iPart1 = -1;
	while (iPart1++ != (int32_t)vecPoint1ToPoint2.size() - 1)
	{
		Split(vecPoint1ToPoint2.at(iPart1), "-", vecPoint);
		if (vecPoint.size() == 2)
		{
			vecPointInt.clear();
			vecPointInt.push_back(atoi(vecPoint[0].c_str()));
			vecPointInt.push_back(atoi(vecPoint[1].c_str()));
			vecSplit.push_back(vecPointInt);
		}
	}
	int32_t vecSplitSize = (int32_t)vecSplit.size();

	std::ifstream myfile(m_strPath);
	std::string strLine;
	int32_t pointToPointIndex = -1;
	int32_t begin = 0;
	int32_t end = 0;
	std::vector<std::string> vecLine;
	while (getline(myfile, strLine))
	{
		vecLine.clear();
		//分出得到第一对截取点，第0个和第2个就是首和尾
		pointToPointIndex = -1;
		while (pointToPointIndex++ != vecSplitSize - 1)
		{
			begin = vecSplit[pointToPointIndex][0];
			end = vecSplit[pointToPointIndex][1];
			vecLine.push_back(strLine.substr(begin - 1, end - begin + 1));
		}
		m_vectxt.push_back(vecLine);
	}
	myfile.close();
	myfile.clear();
}

void Ctxt::LoadTxtWithSplit(const std::string& strSplit)
{
	std::ifstream myfile(m_strPath);
	std::string strLine;
	std::vector<std::string> vecLine;
	while (getline(myfile, strLine))
	{
		Split(strLine, strSplit, vecLine);
		m_vectxt.push_back(vecLine);
	}
	myfile.close();
	myfile.clear();
}

void Ctxt::LoadTxtWithOneLine()
{
	std::ifstream myfile(m_strPath);
	std::string strLine;
	std::vector<std::string> vecLine;
	while (getline(myfile, strLine))
	{
		vecLine.clear();
		vecLine.push_back(strLine);
		m_vectxt.push_back(vecLine);
	}
	myfile.close();
	myfile.clear();
}