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
	char *szbuff = new char[str.GetLength() + 1];
	memcpy(szbuff,(LPTSTR)(LPCTSTR)str,str.GetLength() + 1);
	int nSelectSize = 0;
	int n = 0;
	while(*(szbuff + n)){
		if(*(szbuff + n) == cLeft){
			vecn.push_back(n);
			//nSelect在左，记下现在的nSelectSize
			if(nSelect == n) nSelectSize = vecn.size();
		}
		if(*(szbuff + n) == cRight){
			if(nSelect == n){
				delete[] szbuff;
				return vecn.at(vecn.size() - 1);
			}
			//如果弹出之前发现size大小等于之前记录的，说明找到了对应的右侧
			if(vecn.size() == nSelectSize){
				delete[] szbuff;
				return n;
			}
			vecn.pop_back();
		}
		n++;
	}
	delete[] szbuff;
	return -1;
}