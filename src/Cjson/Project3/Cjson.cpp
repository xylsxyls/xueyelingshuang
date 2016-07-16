#include <SDKDDKVer.h>
#include "Cjson.h"
#include <map>
using namespace std;
#include "CStringManagerAPI.h"
#include "CstrValue.h"
#include "CszValue.h" 

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
		CTypeValue strTypeValueTemp;
		strTypeValueTemp.fill(strValueTemp);
		mapdata[strName] = strTypeValueTemp;
		strTypeValueTemp.free();
	}
	//如果找到了一个数组
	if(flag == 2){
		vecsz.push_back(strName);
		CszValue szValueTemp = strValue;
		mapsz[strName] = szValueTemp;
		CTypeValue szTypeValueTemp;
		szTypeValueTemp.fill(szValueTemp);
		mapdata[strName] = szTypeValueTemp;
		szTypeValueTemp.free();
	}
	//如果找到了一个json
	if(flag == 3){
		vecjson.push_back(strName);
		Cjson jsonTemp;
		jsonTemp.LoadJson(strValue);
		mapjson[strName] = jsonTemp;
		CTypeValue jsonTypeValueTemp;
		jsonTypeValueTemp.fill(jsonTemp);
		mapdata[strName] = jsonTypeValueTemp;
		jsonTypeValueTemp.free();
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

int main(){
	int x = atoi("  \t\t \r\n   1");
	CString str = "123456";
	
	while(1){
		Cjson json;
		json.LoadJson("{\"sieipaddr\" : \"192.168.1.1\" ,\"sieport\" : 9000,\"recordList\" : [{\"nodeid\" : 1,\"nodename\" : \"XXX案件\",\"domaincode\" : \"域ID\",\"devicecode\" : \"设备ID\",\"channlecode\" : \"通道ID\",\"streamcode\" : \"码流ID\"}]}");
	}
	/*
	auto it = json.mapdata.begin();
	it++;
	it++;
	it++;
	it++;
	auto key5it = ((Cjson *)it->second.pClass)->mapdata.begin();
	CString kkk = ((CstrValue *)key5it->second.pClass)->strValue;
	key5it++;
	CString hhh = ((CstrValue *)((CszValue *)key5it->second.pClass)->vecszValue.at(0).pClass)->strValue;
	int n99 = ((CstrValue *)((CszValue *)key5it->second.pClass)->vecszValue.at(1).pClass)->nValue;
	auto nameit = ((Cjson *)((CszValue *)key5it->second.pClass)->vecszValue.at(2).pClass)->mapdata.begin();
	CString zhangsan = ((CstrValue *)nameit->second.pClass)->strValue;
	*/
	/*
	int xx = ((CstrValue *)json.mapdata["key1"])->nValue;

	

	auto it = json.mapdata.begin();
	CString onefirst = it->first;
	int onesecond = ((CstrValue*)it->second)->nValue;

	it++;
	CString twofirst = it->first;
	CString twosecond = ((CstrValue*)it->second)->strValue;

	it++;
	CString thirdfirst = it->first;
	double thirdsecond = ((CstrValue*)it->second)->dValue;

	it++;
	CString forthfirst = it->first;
	//CString forthsecond = ((CszValue*)it->second);

	it++;
	CString fifthfirst = it->first;
	auto fifthsecondit = ((Cjson*)it->second)->mapdata.begin();

	CString fifthsecondOneFirst = fifthsecondit->first;
	CString fifthsecondOnesecond = ((CstrValue *)fifthsecondit->second)->strValue;

	fifthsecondit++;
	CString fifthsecondTwoFirst   = fifthsecondit->first;
	CString fifthsecondTwosecond0 = ((CszValue *)fifthsecondit->second)->vecszOneValue.at(0).strValue.strValue;
	int     fifthsecondTwosecond1 = ((CszValue *)fifthsecondit->second)->vecszOneValue.at(1).strValue.nValue;
	auto  fifthsecondTwosecond2it = ((CszValue *)fifthsecondit->second)->vecjson.at(0).mapdata.begin();

	CString fifthsecondTwosecond2OneFirst = fifthsecondTwosecond2it->first;
	CString fifthsecondTwosecond2OneSecond = ((CstrValue *)fifthsecondTwosecond2it->second)->strValue;*/
	/*
	CString strFirstJsonName = json.vecjson.at(0);
	Cjson strJson = json.mapjson.find(strFirstJsonName)->second;
	CString szName = strJson.vecsz.at(0);
	CszValue strszValue = strJson.mapsz.find(szName)->second;
	CString xxx = strszValue.vecszOneValue.at(0).strValue.strValue;
	int xx = json.mapjson.find(json.vecjson.at(0))->second.mapsz.find(json.mapjson.find(json.vecjson.at(0))->second.vecsz.at(0))->second.vecszOneValue.at(1).strValue.nValue;
	
	CString str2 = (++(json.mapstr.begin()))->first;
	auto it = json.mapstr.begin();
	it++;
	CString str3 = it->first;*/
	return 0;
}