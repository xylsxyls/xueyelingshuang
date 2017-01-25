#include <SDKDDKVer.h>
#include "CjsonA.h"
#include "CstrValue.h"
#include "CszValue.h"
#include "Cjson.h"
#include "CFieldType.h"
#include "CStringManager/CStringManagerAPI.h"

extern vector<void *> vecp;

CjsonA::CjsonA(){
	
}

CjsonA::~CjsonA(){

}

CjsonA::CjsonA(const CjsonA& json){
	this->mapdata  = json.mapdata;
	this->vecField = json.vecField;
	this->vecCjson = json.vecCjson;
	this->mapError = json.mapError;
}

CjsonA CjsonA::operator = (const CjsonA& json){
	this->mapdata  = json.mapdata;
	this->vecField = json.vecField;
	this->vecCjson = json.vecCjson;
	this->mapError = json.mapError;
	return *this;
}

BOOL CjsonA::GetOneModule(string* strJson,BOOL* flag,string* strName,string *strValue){
	int nNameLeft = CStringManager::Find(*strJson,'{');
	nNameLeft++;
	int nNameRight = nNameLeft;
	//第一次找到的是字段名左侧引号位置
	nNameRight = CStringManager::Find(*strJson,'\"', nNameRight);
	if(nNameRight >= 0){
		name:
		//第二次找到的是右侧引号
		nNameRight = CStringManager::Find(*strJson,'\"', nNameRight);
		if((*strJson)[nNameRight - 1] == '\\') goto name;
	}
	//这时nNameRight的位置是引号
	//寻找双引号之后的冒号
	int nColon = CStringManager::Find(*strJson,':', nNameRight + 1);
	//如果无字段名无冒号则有可能全部是无效字符是一个空json，应该返回0
	if(nNameRight < 0 && nColon < 0){
		string strJsonTemp = *strJson;
		CStringManager::Replace(strJsonTemp,"\t", "");
		CStringManager::Replace(strJsonTemp,"\r\n", "");
		CStringManager::Replace(strJsonTemp,"\n", "");
		CStringManager::Replace(strJsonTemp," ", "");
		if(strJsonTemp != "{}") mapError[*strJson] = "不可识别的部分字符串";
		return 0;
	}
	//如果找不到右侧引号但有冒号
	if(nNameRight == -1 && nColon >= 0){
		mapError[*strJson] = "字段值只有左侧有引号";
		return 0;
	}
	//如果找不到
	if(nColon == -1 && nNameRight >= 0){
		mapError[*strJson] = "有字段名但无冒号";
		return 0;
	}
	//把字段取出
	CstrValue strNameValue = CStringManager::Mid(*strJson,nNameLeft,nColon - nNameLeft);
	if(strNameValue.type == 1) *strName = strNameValue.strValue;
	else{
		*strName = strNameValue.strValue;
		mapError[CStringManager::Mid(*strJson,nNameLeft, nColon - nNameLeft)] = "不可识别的字段名";
	}

	int temp = nColon;
	while(1){
		temp++;
		//如果第一个发现引号说明找到了一个字段
		if((*strJson)[temp] == '\"'){
			*flag = 1;
			rem:
			temp = CStringManager::Find(*strJson,"\"", temp + 1);
			//如果找不到引号，说明出错并且一定到了最后
			if(temp == -1){
				mapError[*strJson] = "无右侧引号";
				return 0;
			}
			if((*strJson)[temp - 1] == '\\') goto rem;
			//这时temp的值时右侧引号的位置，从这里到逗号必须都是无效字符
			*strValue = CStringManager::Mid(*strJson,nColon + 1, temp - nColon);
			while(1){
				temp++;
				if((*strJson)[temp] == '\t' || ((*strJson)[temp] == '\r' && (*strJson)[temp + 1] == '\n') || (*strJson)[temp] == '\n' || (*strJson)[temp] == ' ') continue;
				else if((*strJson)[temp] == ','){
					CStringManager::Delete(*strJson,nNameLeft, temp - nNameLeft + 1);
					return 1;
				}
				//说明到了结尾
				else if((*strJson)[temp] == '}'){
					CStringManager::Delete(*strJson,nNameLeft, temp - nNameLeft);
					return 1;
				}
				else if((*strJson)[temp] == 0){
					mapError[*strJson] = "字段值后无大括号";
					return 0;
				}
				else mapError[*strJson] = "字符串字段值后有多余字符";
			}
		}
		//如果第一个发现逗号说明这个找到的是一个字段
		if((*strJson)[temp] == ','){
			*strValue = CStringManager::Mid(*strJson,nColon + 1, temp - nColon - 1);
			//删掉找到的部分，包括逗号
			CStringManager::Delete(*strJson,nNameLeft, temp - nNameLeft + 1);
			//找到字段则标志设为1
			*flag = 1;
			break;
		}
		//如果第一个发现[说明找到了一个数组，取到strValue里不包括[]，删除时带着]删除
		if((*strJson)[temp] == '['){
			//如果找到了一个数组，那么冒号和[之前必须都是无效字符
			string strTemp = CStringManager::Mid(*strJson,nColon + 1,temp - nColon - 1);
			CStringManager::Replace(strTemp,"\t", "");
			CStringManager::Replace(strTemp,"\r\n", "");
			CStringManager::Replace(strTemp,"\n", "");
			CStringManager::Replace(strTemp," ", "");
			if(strTemp != "") mapError[CStringManager::Mid(*strJson,nColon + 1,temp - nColon - 1)] = "冒号和数组间有多余字符";

			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('[',']',temp);
			if(nszRight == -1) mapError[*strJson] = "[]不匹配";
			*strValue = CStringManager::Mid(*strJson,temp + 1, nszRight - temp - 1);
			//先找到后面的逗号然后再删，如果遇到}说明模块结束了则取}之前的位置
			while(1){
				nszRight++;
				if((*strJson)[nszRight] == '\t' || ((*strJson)[nszRight] == '\r' && (*strJson)[nszRight + 1] == '\n') || (*strJson)[nszRight] == '\n' || (*strJson)[nszRight] == ' ') continue;
				else if((*strJson)[nszRight] == '}'){
					nszRight--;
					break;
				}
				else if((*strJson)[nszRight] == ',') break;
				else mapError[*strJson] = "数组后有多余字符";
			}
			CStringManager::Delete(*strJson,nNameLeft, nszRight - nNameLeft + 1);
			
			//找到数组标志设为2
			*flag = 2;
			break;
		}
		//如果找到了大括号右侧说明只有一个字段
		if((*strJson)[temp] == '}'){
			*strValue = CStringManager::Mid(*strJson,nColon + 1, temp - nColon - 1);
			//删掉找到的部分，不包括大括号
			CStringManager::Delete(*strJson,nNameLeft, temp - nNameLeft);
			//找到字段标志设为1
			*flag = 1;
			break;
		}
		//如果找到了一个大括号左侧说明遇到了一个json，取出包括大括号
		if((*strJson)[temp] == '{'){
			//如果找到了一个数组，那么冒号和[之前必须都是无效字符
			string strTemp = CStringManager::Mid(*strJson,nColon + 1, temp - nColon - 1);
			CStringManager::Replace(strTemp,"\t", "");
			CStringManager::Replace(strTemp,"\r\n", "");
			CStringManager::Replace(strTemp,"\n", "");
			CStringManager::Replace(strTemp," ", "");
			if (strTemp != "") mapError[CStringManager::Mid(*strJson,nColon + 1, temp - nColon - 1)] = "冒号和json间有多余字符";

			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('{','}',temp);
			if(nszRight == -1) mapError[*strJson] = "{}不匹配";
			*strValue = CStringManager::Mid(*strJson,temp, nszRight - temp + 1);
			//先找到后面的逗号然后再删，如果遇到}说明模块结束了则取}之前的位置
			while(1){
				nszRight++;
				if((*strJson)[nszRight] == '\t' || ((*strJson)[nszRight] == '\r' && (*strJson)[nszRight + 1] == '\n') || (*strJson)[nszRight] == '\n' || (*strJson)[nszRight] == ' ') continue;
				else if((*strJson)[nszRight] == '}'){
					nszRight--;
					break;
				}
				else if((*strJson)[nszRight] == ',') break;
				else mapError[*strJson] = "json后有多余字符";
			}
			CStringManager::Delete(*strJson,nNameLeft, nszRight - nNameLeft + 1);
			//找到json标志设为3
			*flag = 3;
			break;
		}
	}
	return 1;
}

void CjsonA::LoadOneModule(string strName,string strValue,BOOL flag){
	//如果找到了一个字段
	if(flag == 1){
		CstrValue strValueTemp = strValue;
		if(strValueTemp.type == -1){
			mapError[strValue] = "不可识别的字段值";
		}
		mapdata[strName] = (Cjson)strValueTemp;
	}
	//如果找到了一个数组
	if(flag == 2){
		CszValue szValueTemp = strValue;
		mapdata[strName] = szValueTemp;
		if(szValueTemp.mapszError.size() != 0){
			auto it = szValueTemp.mapszError.begin();
			for(;it != szValueTemp.mapszError.end();it++){
				mapError[it->first] = it->second;
			}
		}
	}
	//如果找到了一个json
	if(flag == 3){
		CjsonA jsonTemp;
		map<string,string> mapJsonError = jsonTemp.LoadJson(strValue);
		mapdata[strName] = jsonTemp;

		if(mapJsonError.size() != 0){
			auto it = mapJsonError.begin();
			for(;it != mapJsonError.end();it++){
				mapError[it->first] = it->second;
			}
		}
	}
}

void CjsonA::OnlyLoadOneJson(string strJson){
	//先取出第一块，进行处理，再取第二块进行处理，直到最后，退出循环
	while(1){
		string strName = "";
		string strValue = "";
		BOOL flag = 0;
		//取出一个模块，这个模块可能是一个字段可能是一个数组，flag0表示字段，1表示数组，取到strOneJson中，同时strJson中去掉取出的部分
		BOOL nNext = GetOneModule(&strJson,&flag,&strName,&strValue);
		if(nNext == 0) break;
		//把这个字段或数组加载到成员函数中
		LoadOneModule(strName,strValue,flag);
	}
}

void CjsonA::GetOneJson(string *strJson,string* strOneJson){
	int nLeft = CStringManager::Find(*strJson,'{');
	if(nLeft == -1) mapError[*strJson] = "找不到{";
	else{
		string strTemp = CStringManager::Mid(*strJson,0, nLeft);
		CStringManager::Replace(strTemp,"\t", "");
		CStringManager::Replace(strTemp,"\r\n", "");
		CStringManager::Replace(strTemp,"\n", "");
		CStringManager::Replace(strTemp," ", "");
		if(strTemp != "") mapError[*strJson] = "json前有多余字符";
	}
	CStringManager manager = *strJson;
	int nRight = manager.FindOther('{','}',nLeft);
	if(nRight == -1) mapError[*strJson] = "{}不匹配";
	else{
		string strJsonNext = CStringManager::Mid(*strJson, nRight + 1, CStringManager::GetLength(*strJson) - nRight - 1);
		CStringManager::Replace(strJsonNext,"\t", "");
		CStringManager::Replace(strJsonNext,"\r\n", "");
		CStringManager::Replace(strJsonNext,"\n", "");
		CStringManager::Replace(strJsonNext," ", "");
		if(strJsonNext != "") mapError[*strJson] = "json后有多余字符";
	}
	*strOneJson = CStringManager::Mid(*strJson,nLeft, nRight - nLeft + 1);
	*strJson = CStringManager::Mid(*strJson, nRight + 1, CStringManager::GetLength(*strJson) - nRight);
}

map<string,string> CjsonA::LoadJson(string strJson){
	//先把内部清空
	mapdata.clear();
	mapError.clear();
	//先取出第一块json，同时去掉取出的部分
	string strOneJson = "";
	GetOneJson(&strJson,&strOneJson);
	OnlyLoadOneJson(strOneJson);
	return mapError;
}

vector<string> CjsonA::GetField(){
	vector<string> vecField;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		vecField.push_back(it->first.c_str());
	}
	return vecField;
}

int CjsonA::size(){
	return mapdata.size();
}

string CjsonA::tostring(string NewLineSign,string FormatSign,int FormatLength){
	string strResult = "{}";
	int nInsert = CStringManager::GetLength(strResult) - 1;
	int i = -1;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		i++;
		string strFieldTemp = it->first.c_str();
		Cjson TypeValueTemp = it->second;
		strResult = AddTypeValue(strResult,&nInsert,!i,strFieldTemp,TypeValueTemp,NewLineSign,FormatSign,FormatLength);
	}
	string strt = "";
	int lengthtemp = FormatLength - 1;
	while(lengthtemp--!=0){
		strt = strt + FormatSign;
	}
	CStringManager::Insert(strResult, CStringManager::GetLength(strResult) - 1, NewLineSign + strt);
	return strResult;
}

string CjsonA::AddTypeValue(string strResult,int *nInsert,BOOL ifFirst,string strField,Cjson TypeValue,string NewLineSign,string FormatSign,int FormatLength){
	int length = FormatLength;
	string strFormatSign = "";
	while(length--!=0){
		strFormatSign = strFormatSign + FormatSign;
	}
	string strOne = strFormatSign + "\"" + strField + "\"";
	if(TypeValue.type == "class CstrValue"){
		CstrValue strValueTemp = TypeValue.toValue();
		//字符串
		if(strValueTemp.type == 1){
			strOne = strOne + " : \"" + strValueTemp.strValue + "\"";
		}
		//整数和小数
		if(strValueTemp.type == 2 || strValueTemp.type == 3){
			strOne = strOne + " : " + strValueTemp.strValue;
		}
	}
	if(TypeValue.type == "class CszValue"){
		CszValue szValue = TypeValue.tosz();
		string strszValue = "";
		int i = -1;
		while(i++ != szValue.size() - 1){
			if(i != 0) strszValue = strszValue + ",";
			Cjson TypeValueTemp = szValue.vecszValue.at(i);
			if(TypeValueTemp.type == "class CstrValue"){
				CstrValue strValueTemp = TypeValueTemp.toValue();
				//字符串
				if(strValueTemp.type == 1){
					strszValue = strszValue + "\"" + strValueTemp.strValue + "\"";
				}
				//整数和小数
				if(strValueTemp.type == 2 || strValueTemp.type == 3){
					strszValue = strszValue + strValueTemp.strValue;
				}
			}
			if(TypeValueTemp.type == "class CjsonA"){
				CjsonA jsonTemp = TypeValueTemp.toJson();
				string strResultTemp = jsonTemp.tostring(NewLineSign,FormatSign,FormatLength + 1);
				strszValue = strszValue + strResultTemp;
			}
		}
		strOne = strOne + " : [" + strszValue + "]";//空数组
	}
	if(TypeValue.type == "class CjsonA"){
		CjsonA jsonTemp = TypeValue.toJson();
		string strResultTemp = jsonTemp.tostring(NewLineSign,FormatSign,FormatLength + 1);
		strOne = strOne + " : " + strResultTemp;
	}
	if(ifFirst == 1){
		strOne = NewLineSign + strOne;
	}
	else{
		strOne = "," + NewLineSign + strOne;
	}
	CStringManager::Insert(strResult,*nInsert,strOne);
	*nInsert = *nInsert + CStringManager::GetLength(strOne);
	return strResult;
}