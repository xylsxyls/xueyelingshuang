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