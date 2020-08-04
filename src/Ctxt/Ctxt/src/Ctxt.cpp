#include "Ctxt.h"
#include <fstream>
#include <stdarg.h>

Ctxt::Ctxt(const std::string& strPath)
{
	m_strPath = strPath;
	m_txt = new std::ofstream(strPath.c_str(), std::ios::app);
}

Ctxt::~Ctxt()
{
	delete (std::ofstream*)m_txt;
}

static void Split(std::vector<std::string>& result, const std::string& splitString, const std::string& separate_character)
{
	result.clear();
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		result.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	result.push_back(splitString.substr(lastPosition));
}

static size_t ReplaceStr(std::string& str, const std::string& oldstr, const std::string& newstr)
{
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos == std::string::npos)
		{
			break;
		}
		str.replace(pos, oldstr.length(), newstr);
		pos += newstr.length();
		++count;
	}
	return count;
}

void Ctxt::LoadTxt(Load flag, const std::string& strSplit)
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

bool Ctxt::Save()
{
	return SaveAs(m_strPath);
}

void Ctxt::Write(FILE* pFile, const char* fmt, ...)
{
	if (pFile == nullptr)
	{
		return;
	}

	std::string result;
	va_list args;
	va_start(args, fmt);
#ifdef _WIN32
	int size = _vscprintf(fmt, args);
#elif __linux__
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, fmt, argcopy);
#endif
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	if (size != 0)
	{
#ifdef _WIN32
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&result[0], size + 1, fmt, args);
#elif __linux__
		vsnprintf(&result[0], size + 1, fmt, args);
#endif
	}
	va_end(args);
	::fprintf(pFile, "%s", result.c_str());
}

void Ctxt::AddLineWithoutOpenFile(const char* fmt, ...)
{
	std::string result;
	va_list args;
	va_start(args, fmt);
#ifdef _WIN32
	int size = _vscprintf(fmt, args);
#elif __linux__
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, fmt, argcopy);
#endif
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	if (size != 0)
	{
#ifdef _WIN32
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&result[0], size + 1, fmt, args);
#elif __linux__
		vsnprintf(&result[0], size + 1, fmt, args);
#endif
	}
	va_end(args);
    *(std::ofstream*)m_txt << result << std::endl;
}

bool Ctxt::SaveAs(const std::string& path)
{
	FILE* pFile = ::fopen(path.c_str(), "w+b");
	if (pFile == nullptr)
	{
		return false;
	}
	int32_t lineIndex = -1;
	int32_t partIndex = -1;
	std::string strLine;
	while (lineIndex++ != m_vectxt.size() - 1)
	{
		strLine.clear();
		partIndex = -1;
		std::vector<std::string>& vecLine = m_vectxt[lineIndex];
		while (partIndex++ != (int32_t)vecLine.size() - 1)
		{
			strLine.append(vecLine[partIndex]);
		}
		Write(pFile, (lineIndex == m_vectxt.size() - 1) ? "%s" : "%s\n", strLine.c_str());
	}
	::fclose(pFile);
	pFile = nullptr;
	return true;
}

int32_t Ctxt::Replace(const std::string& oldstr, const std::string& newstr)
{
	int32_t result = 0;
	int32_t lineIndex = -1;
	while (lineIndex++ != m_vectxt.size() - 1)
	{
		auto& lineVec = m_vectxt[lineIndex];
		int32_t columnIndex = -1;
		while (columnIndex++ != lineVec.size() - 1)
		{
			std::string& text = lineVec[columnIndex];
			result += ReplaceStr(text, oldstr, newstr);
		}
	}
	return result;
}

void Ctxt::LoadTxtWithPointToPoint(const std::string& strSplit)
{
	std::vector<std::vector<int32_t>> vecSplit;
	std::vector<std::string> vecPoint1ToPoint2;
	Split(vecPoint1ToPoint2, strSplit, ",");
	std::vector<int32_t> vecPointInt;
	std::vector<std::string> vecPoint;
	//分出得到第一对截取点，第0个和第2个就是首和尾
	int32_t iPart1 = -1;
	while (iPart1++ != (int32_t)vecPoint1ToPoint2.size() - 1)
	{
		Split(vecPoint, vecPoint1ToPoint2.at(iPart1), "-");
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
		Split(vecLine, strLine, strSplit);
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