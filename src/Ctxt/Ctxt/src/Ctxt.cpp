#include <SDKDDKVer.h>
#include "Ctxt.h"
#include <string>
#include <fstream>
using namespace std;

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

void Ctxt::LoadTxt(CString strPath,BOOL flag,CString strSplit){
	ifstream myfile(strPath);
	string strLine = "";
	int iLine = -1;
	while(getline(myfile,strLine) != 0){
		iLine++;
		//转换成CString具体处理每一行
		CString cstrLine = strLine.c_str();
		if(flag == 1){
			vector<string> vecPart = split(strSplit.GetBuffer(),","); strSplit.ReleaseBuffer();
			int iPart1 = -1;
			int size = vecPart.size();
			vector<CString> vecLine1;
			//分出得到第一对截取点，第0个和第2个就是首和尾
			while(iPart1++ != size - 1){
				vector<string> vecPoint = split(vecPart.at(iPart1).c_str(),"-");
				if(vecPoint.size() >= 2){
					int begin = atoi(vecPoint.at(0).c_str());
					int end   = atoi(vecPoint.at(1).c_str());
					vecLine1.push_back(cstrLine.Mid(begin,end - begin));
				}
				//如果是传入了空字符串就把一行插入
				else{
					vecLine1.push_back(cstrLine);
				}
			}
			vectxt.push_back(vecLine1);
		}
		if(flag == 2){
			vector<string> vecPart = split(strLine,strSplit.GetBuffer()); strSplit.ReleaseBuffer();
			int sizePart = vecPart.size();
			vector<CString> vecLine2;
			int iPart2 = -1;
			while(iPart2++ != sizePart - 1){
				vecLine2.push_back(vecPart.at(iPart2).c_str());
			}
			vectxt.push_back(vecLine2);
		}
	}
	return;
}