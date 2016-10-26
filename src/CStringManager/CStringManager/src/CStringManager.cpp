#include <SDKDDKVer.h>
#include "CStringManager.h"
#include <vector>
using namespace std;

CStringManager::CStringManager(CString str){
	this->str = str;
}

CStringManager CStringManager::operator = (CString str){
	this->str = str;
	return *this;
}

int CStringManager::FindOther(char cLeft,char cRight,int nSelect){
	vector<int> vecn;
	int nSelectSize = 0;
	int n = 0;
	while(str[n]){
		if(str[n] == cLeft){
			vecn.push_back(n);
			//nSelect在左，记下现在的nSelectSize
			if(nSelect == n) nSelectSize = vecn.size();
		}
		if(str[n] == cRight){
			if(nSelect == n){
				return vecn.at(vecn.size() - 1);
			}
			//如果弹出之前发现size大小等于之前记录的，说明找到了对应的右侧
			if(vecn.size() == nSelectSize){
				return n;
			}
			vecn.pop_back();
		}
		n++;
	}
	return -1;
}

vector<CString> CStringManager::split(CString separate_character){
	string strTemp = str;
	vector<CString> strs;

	int separate_characterLen = separate_character.GetLength();//分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	int lastPosition = 0,index = -1;
	while (-1 != (index = strTemp.find(separate_character,lastPosition))){
		strs.push_back(strTemp.substr(lastPosition,index - lastPosition).c_str());   
		lastPosition = index + separate_characterLen;   
	}
	string lastString = strTemp.substr(lastPosition);//截取最后一个分隔符后的内容   
	//if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(lastString.c_str());
	return strs;
}