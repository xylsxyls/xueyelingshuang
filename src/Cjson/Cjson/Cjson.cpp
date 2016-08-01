#include <SDKDDKVer.h>
#include "Cjson.h"
#include "CstrValue.h"
#include "CszValue.h"
#include "CTypeValue.h"
#include "CFieldType.h"
#include "CStringManagerAPI.h"


extern vector<void *> vecp;

Cjson::Cjson(){

}

Cjson::~Cjson(){

}

BOOL Cjson::GetOneModule(CString* strJson,BOOL* flag,CString* strName,CString *strValue){

	/*
	int nNameLeft = strJson->Find('\"');
	//如果找不到引号说明这是一个空的json，则返回0
	if(nNameLeft == -1) return 0;
	int nNameRight = strJson->Find('\"',nNameLeft + 1);
	//获得第一个双引号里的内容
	*strName = strJson->Mid(nNameLeft + 1,nNameRight - nNameLeft - 1);
	//寻找双引号之后的冒号
	int nColon = strJson->Find(':',nNameRight + 1);
	*/
	int nNameLeft = strJson->Find('{');
	nNameLeft++;
	int nNameRight = strJson->Find('\"',nNameLeft + 1);
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
	//如果找不到
	if(nColon < 0){
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
		//如果第一个发现逗号或者大括号说明这个找到的是一个字段
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

void Cjson::LoadOneModule(CString strName,CString strValue,BOOL flag){
	//如果找到了一个字段
	if(flag == 1){
		CstrValue strValueTemp = strValue;
		if(strValueTemp.type == -1){
			mapError[strValue] = "不可识别的字段值";
		}
		mapdata[strName] = strValueTemp;
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
		Cjson jsonTemp;
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

void Cjson::OnlyLoadOneJson(CString strJson){
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

void Cjson::GetOneJson(CString *strJson,CString* strOneJson){
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

map<CString,CString> Cjson::LoadJson(CString strJson){
	//先把内部清空
	mapdata.clear();
	mapError.clear();
	//先取出第一块json，同时去掉取出的部分
	CString strOneJson = "";
	GetOneJson(&strJson,&strOneJson);
	OnlyLoadOneJson(strOneJson);
	return mapError;
}

vector<CString> Cjson::GetField(){
	vector<CString> vecField;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		vecField.push_back(it->first);
	}
	return vecField;
}

int Cjson::size(){
	return mapdata.size();
}

CString Cjson::toCString(CString NewLineSign,CString FormatSign,int FormatLength){
	CString strResult = "{}";
	int nInsert = strResult.GetLength() - 1;
	int i = -1;
	auto it = mapdata.begin();
	for(;it != mapdata.end();it++){
		i++;
		CString strFieldTemp = it->first;
		CTypeValue TypeValueTemp = it->second;
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

CString Cjson::AddTypeValue(CString strResult,int *nInsert,BOOL ifFirst,CString strField,CTypeValue TypeValue,CString NewLineSign,CString FormatSign,int FormatLength){
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
			CTypeValue TypeValueTemp = szValue.vecszValue.at(i);
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
			if(TypeValueTemp.type == "class Cjson"){
				Cjson jsonTemp = TypeValueTemp.toJson();
				CString strResultTemp = jsonTemp.toCString(NewLineSign,FormatSign,FormatLength + 1);
				strszValue = strszValue + strResultTemp;
			}
		}
		strOne = strOne + " : [" + strszValue + "]";//空数组
	}
	if(TypeValue.type == "class Cjson"){
		Cjson jsonTemp = TypeValue.toJson();
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



int main(){
	
	map<int,int> map;
	map[0] = 1;

	
	Cjson json;
	json.LoadJson("{\"layout\":1,\"videoList\":[{\"wid\":0,\"isAudioValid\":1,\"fileList\":[{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1.mp4\",\"startTime\":0,\"stopTime\": 3200},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1_1.mp4\",\"startTime\":3200,\"stopTime\":6201},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_2.mp4\",\"startTime\":6231,\"stopTime\":6889}   ]    }   ]  }");
	int xxxxxxxxx = json.mapError.size();
	vecp = vecp;
	int x = 0;
	AfxMessageBox(json.toCString());
	while(1){
		x++;
		if(x >2000){
			//break;
		}
		//"{   \"videoList\":[   {\"fileList\":[  {},{}  ]}   ]   }"
		json.LoadJson("{\"a\":1}");
		json.toCString("\r\n","    ");
		//AfxMessageBox(json.toCString("\r\n","    "));
		//CTypeValue xxxxx;
		CTypeValue xxxx = json;
		//CString zhangsan = xxxx["key8"]["key52"][2]["name"].toValue().strValue;
		//CTypeValue ssss = xxxx.toJson().mapdata["key10"];
		//xxxx.toJson().mapdata["key10"] = CstrValue("124");
		//xxxx.toJson().mapdata["key11"] = CszValue("124,\"456\"");
		//xxxx[""] = 1.1;
		//xxxx["layout1"][0] = 1.2;
		//xxxx["layout1"][1] = 1.3;
		//xxxx["layout1"][2] = 1.4;
		//xxxx["layout1"][3] = 1.5;
		//xxxx["layout1"][2] = "delete";
		//xxxx["layout1"][1] = "insert";
		xxxx["layout1"][0] = "insert";
		xxxx["layout1"] = "delete";
		xxxx["layout1"][0] = 1.2;
		xxxx["layout1"][0][3][7] = 1.3;
		xxxx["layout1"][2] = 1.4;
		xxxx["layout1"][3] = 1.5;
		//AfxMessageBox(json.toCString());
		//xxxx = json;
		//xxxx["key33"][0] = "delete";
		//CszValue szValue = xxxx.toJson().mapdata["key9"].tosz();
		//CString strrrrrr = szValue.vecszValue.at(0).toValue().strValue;
		//xxxx.toJson().mapdata["key9"].tosz().vecszValue.push_back(CstrValue("123456"));
		//CTypeValue strrr = xxxx.toJson().mapdata["key10"];
		//CString strjson = xxxx.toJson().toCString();
		AfxMessageBox(xxxx.toJson().toCString());
	}
	AfxMessageBox("2");
	CTypeValue xxxx = json;
	
	int x1 = xxxx["key4"][2].toValue().nValue;
	

	return 0;
}