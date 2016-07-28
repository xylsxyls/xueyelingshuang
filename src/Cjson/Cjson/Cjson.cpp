#include <SDKDDKVer.h>
#include "Cjson.h"
#include <map>
using namespace std;
#include "CStringManagerAPI.h"
#include "CstrValue.h"
#include "CszValue.h" 

Cjson::Cjson(){

}

Cjson::~Cjson(){

}

BOOL GetOneModule(CString* strJson,BOOL* flag,CString* strName,CString *strValue){

	int nNameLeft = strJson->Find('\"');
	//如果找不到引号说明这是一个空的json，则返回0
	if(nNameLeft == -1) return 0;
	int nNameRight = strJson->Find('\"',nNameLeft + 1);
	//获得第一个双引号里的内容
	*strName = strJson->Mid(nNameLeft + 1,nNameRight - nNameLeft - 1);
	//寻找双引号之后的冒号
	int nColon = strJson->Find(':',nNameRight + 1);
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
			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('[',']',temp);
			*strValue = strJson->Mid(temp + 1,nszRight - temp - 1);
			//先找到后面的逗号然后再删，如果遇到}说明模块结束了则取}之前的位置
			while(1){
				nszRight++;
				if((*strJson)[nszRight] == '}'){
					nszRight--;
					break;
				}
				if((*strJson)[nszRight] == ',') break;
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
			CStringManager manager = *strJson;
			int nszRight = manager.FindOther('{','}',temp);
			*strValue = strJson->Mid(temp,nszRight - temp + 1);
			//先找到后面的逗号然后再删，如果遇到}说明模块结束了则取}之前的位置
			while(1){
				nszRight++;
				if((*strJson)[nszRight] == '}'){
					nszRight--;
					break;
				}
				if((*strJson)[nszRight] == ',') break;
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
		vecstr.push_back(strName);
		CstrValue strValueTemp = strValue;
		mapstr[strName] = strValueTemp;
		CTypeValue strTypeValueTemp = strValueTemp;
		mapdata[strName] = strTypeValueTemp;
	}
	//如果找到了一个数组
	if(flag == 2){
		vecsz.push_back(strName);
		CszValue szValueTemp = strValue;
		mapsz[strName] = szValueTemp;
		CTypeValue szTypeValueTemp = szValueTemp;
		mapdata[strName] = szTypeValueTemp;
	}
	//如果找到了一个json
	if(flag == 3){
		vecjson.push_back(strName);
		Cjson jsonTemp;
		jsonTemp.LoadJson(strValue);
		mapjson[strName] = jsonTemp;
		CTypeValue jsonTypeValueTemp = jsonTemp;
		mapdata[strName] = jsonTypeValueTemp;
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

BOOL GetOneJson(CString *strJson,CString* strOneJson){
	int nLeft = strJson->Find('{');
	if(nLeft == -1) return 0;
	CStringManager manager = *strJson;
	int nRight = manager.FindOther('{','}',nLeft);
	*strOneJson = strJson->Mid(nLeft,nRight - nLeft + 1);
	*strJson = strJson->Mid(nRight + 1,strJson->GetLength() - nRight);
	return 1;
}

void Cjson::LoadJson(CString strJson){
	//先把内部清空
	vecstr.clear();
	vecsz.clear();
	vecjson.clear();
	mapdata.clear();
	mapjson.clear();
	mapstr.clear();
	mapsz.clear();
	//先取出第一块json，同时去掉取出的部分
	while(1){
		CString strOneJson = "";
		BOOL nNext = GetOneJson(&strJson,&strOneJson);
		if(nNext == 0) break;
		OnlyLoadOneJson(strOneJson);
	}
	return;
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
	json.LoadJson("{\"layout\":1,\"videoList\":[{\"wid\":0,\"isAudioValid\":1,\"fileList\":[{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1.mp4\",\"startTime\":0,\"stopTime\":3200},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1_1.mp4\",\"startTime\":3200,\"stopTime\":6201},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_2.mp4\",\"startTime\":6231,\"stopTime\":6889}]}]}");
	int x = 0;
	Sleep(15000);
	while(1){
		x++;
		if(x >200000){
			break;
		}
		//"{   \"videoList\":[   {\"fileList\":[  {},{}  ]}   ]   }"
		json.LoadJson("{\"layout\":1,\"videoList\":[{\"wid\":0,\"isAudioValid\":1,\"fileList\":[{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1.mp4\",\"startTime\":0,\"stopTime\":3200},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_1_1.mp4\",\"startTime\":3200,\"stopTime\":6201},{\"fileName\":\"F:\\1111\\(2015)苏民终字第00596号\\(2015)苏民终字第00596号_【合成】2016-04-13 16.33.53\\A00_218_2.mp4\",\"startTime\":6231,\"stopTime\":6889}]}]}");
		//json.toCString("\r\n","    ");
		//AfxMessageBox(json.toCString("\r\n","    "));
		//CTypeValue xxxxx;
		//CTypeValue xxxx = json;
		//CString zhangsan = xxxx["key8"]["key52"][2]["name"].toValue().strValue;
		//CTypeValue ssss = xxxx.toJson().mapdata["key10"];
		//xxxx.toJson().mapdata["key10"] = CstrValue("124");
		//xxxx.toJson().mapdata["key11"] = CszValue("124,\"456\"");
		//xxxx[""] = 1.1;
		//xxxx["layout1"][0]["123"]["1"] = 1.2;
		//AfxMessageBox(json.toCString());
		//xxxx = json;
		//xxxx["key33"][0] = "delete";
		//CszValue szValue = xxxx.toJson().mapdata["key9"].tosz();
		//CString strrrrrr = szValue.vecszValue.at(0).toValue().strValue;
		//xxxx.toJson().mapdata["key9"].tosz().vecszValue.push_back(CstrValue("123456"));
		//CTypeValue strrr = xxxx.toJson().mapdata["key10"];
		//AfxMessageBox(xxxx.toJson().toCString());
	}
	while(1){

	}
	CTypeValue xxxx = json;
	
	int x1 = xxxx["key4"][2].toValue().nValue;
	return 0;
}