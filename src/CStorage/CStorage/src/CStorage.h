#pragma once
#include <map>
#include "boost/any.hpp"
#include <string>
#include <unordered_map>
#include <afxwin.h>
#include "CStorageMacro.h"

using namespace std;
using namespace boost;

class CStorageAPI CStorage{
public:
	CStorage();
	bool changeString;
	string strCurrentKey;
	unordered_map<string, any> mapData;
	bool changeInt;
	int nCurrentKey;
	unordered_map<int, any> mapDataInt;

	CStorage& operator[](int key);
	CStorage& operator[](CString key);
    CStorage operator=(const char* value);
	CStorage operator=(any anyValue);

	template<typename T>
	T toValue(){
		if (changeString == true){
			changeString = false;
			auto it = mapData.find(strCurrentKey);
			any anyValue;
			CString strError;
			try{
				if (it != mapData.end()){
					anyValue = it->second;
					return any_cast<T>(anyValue);
				}
#ifdef _UNICODE
				strError.Format(L"key值：%s，不存在", strCurrentKey.c_str());
#else
				strError.Format("key值：%s，不存在", strCurrentKey.c_str());
#endif
				AfxMessageBox(strError);
				return T();
			}
			catch (boost::bad_any_cast &e){
#ifdef _UNICODE
				strError.Format(L"错误，类型为:%s，\nboost解释：%s", it->second.type().name(), e.what());
#else
				strError.Format("错误，类型为:%s，\nboost解释：%s", it->second.type().name(), e.what());
#endif
				AfxMessageBox(strError);
				return T();
			}
		}
		else if (changeInt == true){
			changeInt = false;
			auto it = mapDataInt.find(nCurrentKey);
			any anyValue;
			CString strError;
			try{
				if (it != mapDataInt.end()){
					anyValue = it->second;
					return any_cast<T>(anyValue);
				}
#ifdef _UNICODE
				strError.Format(L"key值：%d，不存在", nCurrentKey);
#else
				strError.Format("key值：%d，不存在", nCurrentKey);
#endif
				AfxMessageBox(strError);
				return T();
			}
			catch (boost::bad_any_cast &e){
#ifdef _UNICODE
				strError.Format(L"错误，类型为:%s，\nboost解释：%s", it->second.type().name(), e.what());
#else
				strError.Format("错误，类型为:%s，\nboost解释：%s", it->second.type().name(), e.what());
#endif
				AfxMessageBox(strError);
				return T();
			}
		}
		else return T();
	}
};