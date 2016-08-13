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

BOOL CjsonA::GetOneModule(CString* strJson,BOOL* flag,CString* strName,CString *strValue){
	int nNameLeft = strJson->Find('{');
	nNameLeft++;
	int nNameRight = nNameLeft;
	//第一次找到的是字段名左侧引号位置
	nNameRight = strJson->Find('\"',nNameRight);
	if(nNameRight >= 0){
		name:
		//第二次找到的是右侧引号
		nNameRight = strJson->Find('\"',nNameRight);
		if((*strJson)[nNameRight - 1] == '\\') goto name;
	}
	//这时nNameRight的位置是引号
	//寻找双引号之后的冒号
	int nColon = strJson->Find(':',nNameRight + 1);
	//如果无字段名无冒号则有可能全部是无效字符是一个空json，应该返回0
	if(nNameRight < 0 && nColon < 0){
		CString strJsonTemp = *strJson;
		strJsonTemp.Replace("\t","");
		strJsonTemp.Replace("\r\n","");
		strJsonTemp.Replace("\n","");
		strJsonTemp.Replace(" ","");
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
	CstrValue strNameValue = strJson->Mid(nNameLeft,nColon - nNameLeft);
	if(strNameValue.type == 1) *strName = strNameValue.strValue;
	else{
		*strName = strNameValue.strValue;
		mapError[strJson->Mid(nNameLeft,nColon - nNameLeft)] = "不可识别的字段名";
	}

	int temp = nColon;
	while(1){
		temp++;
		//如果第一个发现引号说明找到了一个字段
		if((*strJson)[temp] == '\"'){
			*flag = 1;
			rem:
			temp = strJson->Find("\"",temp + 1);
			if((*strJson)[temp - 1] == '\\') goto rem;
			//这时temp的值时右侧引号的位置，从这里到逗号必须都是无效字符
			*strValue = strJson->Mid(nColon + 1,temp - nColon);
			while(1){
				temp++;
				if((*strJson)[temp] == '\t' || ((*strJson)[temp] == '\r' && (*strJson)[temp + 1] == '\n') || (*strJson)[temp] == '\n' || (*strJson)[temp] == ' ') continue;
				else if((*strJson)[temp] == ','){
					strJson->Delete(nNameLeft,temp - nNameLeft + 1);
					return 1;
				}
				//说明到了结尾
				else if((*strJson)[temp] == '}'){
					strJson->Delete(nNameLeft,temp - nNameLeft);
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
			*strValue = strJson->Mid(nColon + 1,temp - nColon - 1);
			//删掉找到的部分，包括逗号
			strJson->Delete(nNameLeft,temp - nNameLeft + 1);
			//找到字段则标志设为1
			*flag = 1;
			break;
		}
		//如果第一个发现[说明找到了一个数组，取到strValue里不包括[]，删除时带着]删除
		if((*strJson)[temp] == '['){
			//如果找到了一个数组，那么冒号和[之前必须都是无效字符
			CString strTemp = strJson->Mid(nColon + 1,temp - nColon - 1);
			strTemp.Replace("\t","");
			strTemp.Replace("\r\n","");
			strTemp.Replace("\n","");
			strTemp.Replace(" ","");
			if(strTemp != "") mapError[strJson->Mid(nColon + 1,temp - nColon - 1)] = "冒号和数组间有多余字符";

			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('[',']',temp);
			if(nszRight == -1) mapError[*strJson] = "[]不匹配";
			*strValue = strJson->Mid(temp + 1,nszRight - temp - 1);
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
			strJson->Delete(nNameLeft,nszRight - nNameLeft + 1);
			
			//找到数组标志设为2
			*flag = 2;
			break;
		}
		//如果找到了大括号右侧说明只有一个字段
		if((*strJson)[temp] == '}'){
			*strValue = strJson->Mid(nColon + 1,temp - nColon - 1);
			//删掉找到的部分，不包括大括号
			strJson->Delete(nNameLeft,temp - nNameLeft);
			//找到字段标志设为1
			*flag = 1;
			break;
		}
		//如果找到了一个大括号左侧说明遇到了一个json，取出包括大括号
		if((*strJson)[temp] == '{'){
			//如果找到了一个数组，那么冒号和[之前必须都是无效字符
			CString strTemp = strJson->Mid(nColon + 1,temp - nColon - 1);
			strTemp.Replace("\t","");
			strTemp.Replace("\r\n","");
			strTemp.Replace("\n","");
			strTemp.Replace(" ","");
			if(strTemp != "") mapError[strJson->Mid(nColon + 1,temp - nColon - 1)] = "冒号和json间有多余字符";

			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('{','}',temp);
			if(nszRight == -1) mapError[*strJson] = "{}不匹配";
			*strValue = strJson->Mid(temp,nszRight - temp + 1);
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
			strJson->Delete(nNameLeft,nszRight - nNameLeft + 1);
			//找到json标志设为3
			*flag = 3;
			break;
		}
	}
	return 1;
}

void CjsonA::LoadOneModule(CString strName,CString strValue,BOOL flag){
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
		map<CString,CString> mapJsonError = jsonTemp.LoadJson(strValue);
		mapdata[strName] = jsonTemp;

		if(mapJsonError.size() != 0){
			auto it = mapJsonError.begin();
			for(;it != mapJsonError.end();it++){
				mapError[it->first] = it->second;
			}
		}
	}
}

void CjsonA::OnlyLoadOneJson(CString strJson){
	//先取出第一块，进行处理，再取第二块进行处理，直到最后，退出循环
	while(1){
		CString strName = "";
		CString strValue = "";
		BOOL flag = 0;
		//取出一个模块，这个模块可能是一个字段可能是一个数组，flag0表示字段，1表示数组，取到strOneJson中，同时strJson中去掉取出的部分
		BOOL nNext = GetOneModule(&strJson,&flag,&strName,&strValue);
		if(nNext == 0) break;
		//把这个字段或数组加载到成员函数中
		LoadOneModule(strName,strValue,flag);
	}
}

void CjsonA::GetOneJson(CString *strJson,CString* strOneJson){
	int nLeft = strJson->Find('{');
	if(nLeft == -1) mapError[*strJson] = "找不到{";
	else{
		CString strTemp = strJson->Mid(0,nLeft);
		strTemp.Replace("\t","");
		strTemp.Replace("\r\n","");
		strTemp.Replace("\n","");
		strTemp.Replace(" ","");
		if(strTemp != "") mapError[*strJson] = "json前有多余字符";
	}
	CStringManager manager = *strJson;
	int nRight = manager.FindOther('{','}',nLeft);
	if(nRight == -1) mapError[*strJson] = "{}不匹配";
	else{
		CString strJsonNext = strJson->Mid(nRight + 1,strJson->GetLength() - nRight - 1);
		strJsonNext.Replace("\t","");
		strJsonNext.Replace("\r\n","");
		strJsonNext.Replace("\n","");
		strJsonNext.Replace(" ","");
		if(strJsonNext != "") mapError[*strJson] = "json后有多余字符";
	}
	*strOneJson = strJson->Mid(nLeft,nRight - nLeft + 1);
	*strJson = strJson->Mid(nRight + 1,strJson->GetLength() - nRight);
}

map<CString,CString> CjsonA::LoadJson(CString strJson){
	//先把内部清空
	mapdata.clear();
	mapError.clear();
	//先取出第一块json，同时去掉取出的部分
	CString strOneJson = "";
	GetOneJson(&strJson,&strOneJson);
	OnlyLoadOneJson(strOneJson);
	return mapError;
}

vector<CString> CjsonA::GetField(){
	vector<CString> vecField;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		vecField.push_back(it->first);
	}
	return vecField;
}

int CjsonA::size(){
	return mapdata.size();
}

CString CjsonA::toCString(CString NewLineSign,CString FormatSign,int FormatLength){
	CString strResult = "{}";
	int nInsert = strResult.GetLength() - 1;
	int i = -1;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		i++;
		CString strFieldTemp = it->first;
		Cjson TypeValueTemp = it->second;
		strResult = AddTypeValue(strResult,&nInsert,!i,strFieldTemp,TypeValueTemp,NewLineSign,FormatSign,FormatLength);
	}
	CString strt = "";
	int lengthtemp = FormatLength - 1;
	while(lengthtemp--!=0){
		strt = strt + FormatSign;
	}
	strResult.Insert(strResult.GetLength() - 1,NewLineSign + strt);
	return strResult;
}

CString CjsonA::AddTypeValue(CString strResult,int *nInsert,BOOL ifFirst,CString strField,Cjson TypeValue,CString NewLineSign,CString FormatSign,int FormatLength){
	int length = FormatLength;
	CString strFormatSign = "";
	while(length--!=0){
		strFormatSign = strFormatSign + FormatSign;
	}
	CString strOne = strFormatSign + "\"" + strField + "\"";
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
		CString strszValue = "";
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
				CString strResultTemp = jsonTemp.toCString(NewLineSign,FormatSign,FormatLength + 1);
				strszValue = strszValue + strResultTemp;
			}
		}
		strOne = strOne + " : [" + strszValue + "]";//空数组
	}
	if(TypeValue.type == "class CjsonA"){
		CjsonA jsonTemp = TypeValue.toJson();
		CString strResultTemp = jsonTemp.toCString(NewLineSign,FormatSign,FormatLength + 1);
		strOne = strOne + " : " + strResultTemp;
	}
	if(ifFirst == 1){
		strOne = NewLineSign + strOne;
	}
	else{
		strOne = "," + NewLineSign + strOne;
	}
	strResult.Insert(*nInsert,strOne);
	*nInsert = *nInsert + strOne.GetLength();
	return strResult;
}