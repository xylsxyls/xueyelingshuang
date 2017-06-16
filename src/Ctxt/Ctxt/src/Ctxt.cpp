#include <SDKDDKVer.h>
#include "Ctxt.h"
#include <string>
#include <fstream>
using namespace std;
#include <stdarg.h>

Ctxt::Ctxt(string strPath){
	this->strPath = strPath;
	txt = new ofstream(strPath.c_str(), ios::app);
	pFile = NULL;
}

Ctxt::~Ctxt(){
    delete (ofstream*)txt;
}

vector<string> split(string src,string separate_character){
	vector<string> strs;

	int separate_characterLen = separate_character.size();//分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	int lastPosition = 0,index = -1;
	while (-1 != (index = src.find(separate_character,lastPosition))){
		strs.push_back(src.substr(lastPosition,index - lastPosition));   
		lastPosition = index + separate_characterLen;   
	}
	string lastString = src.substr(lastPosition);//截取最后一个分隔符后的内容   
	//if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(lastString);
	return strs;
}

void Ctxt::LoadTxt(int flag,string strSplit){
	ifstream myfile(strPath);
	string strLine = "";
	int iLine = -1;
	while(getline(myfile,strLine)){
		iLine++;
		//转换成string具体处理每一行
		if(flag == 1){
			vector<string> vecPart = split(strSplit,",");
			int iPart1 = -1;
			int size = vecPart.size();
			vector<string> vecLine1;
			//分出得到第一对截取点，第0个和第2个就是首和尾
			while(iPart1++ != size - 1){
				vector<string> vecPoint = split(vecPart.at(iPart1),"-");
				if(vecPoint.size() >= 2){
					int begin = atoi(vecPoint.at(0).c_str());
					int end   = atoi(vecPoint.at(1).c_str());
                    vecLine1.push_back(strLine.substr(begin - 1, end - begin + 1));
				}
				//如果是传入了空字符串就把一行插入
				else{
                    vecLine1.push_back(strLine);
				}
			}
			vectxt.push_back(vecLine1);
		}
		if(flag == 2){
			vector<string> vecPart = split(strLine,strSplit);
			int sizePart = vecPart.size();
			vector<string> vecLine2;
			int iPart2 = -1;
			while(iPart2++ != sizePart - 1){
				vecLine2.push_back(vecPart.at(iPart2));
			}
			vectxt.push_back(vecLine2);
		}
	}
	myfile.close();
	myfile.clear();
	return;
}

void Ctxt::Save(){
	OpenFile_w();
	int i = -1;
	string strLine;
	while (i++ != vectxt.size() - 1){
		strLine = "";
		int j = -1;
		while (j++ != vectxt.at(i).size() - 1){
			strLine = strLine + vectxt.at(i).at(j);
		}
		if (i == vectxt.size() - 1) break;
		AddWriteLine("%s", strLine.c_str());
	}
	fprintf(pFile, "%s", strLine.c_str());
	CloseFile();
	return;
}

bool Ctxt::OpenFile_w(){
	pFile = fopen(strPath.c_str(), "w+");
	if (pFile == NULL) return 0;
	return 1;
}

bool Ctxt::OpenFile_a(){
	pFile = fopen(strPath.c_str(),"a+");
	if(pFile == NULL) return 0;
	return 1;
}

void Ctxt::AddWriteLine(string format,...){
	std::string result;
	va_list parameterlist = NULL;

	va_start(parameterlist, format);
	int size = _vscprintf(format.c_str(), parameterlist) + 2;
	result.resize(size);
	size = vsprintf_s(&result[0], size, format.c_str(), parameterlist);
	result.resize(size + 1);
	result[size] = 0;
	va_end(parameterlist);
	fprintf(pFile, "%s\n", result.c_str());
	return;
}

void Ctxt::CloseFile(){
	fclose(pFile);
	pFile = NULL;
	return;
}

void Ctxt::AddLine(string format, ...){
	std::string result;
	va_list parameterlist = NULL;

	va_start(parameterlist, format);
	int size = _vscprintf(format.c_str(), parameterlist);
	result.resize(size + 1);
	vsprintf_s(&result[0], size + 1, format.c_str(), parameterlist);
	result.resize(size);
	va_end(parameterlist);
    *(ofstream*)txt << result << endl;
	return;
}