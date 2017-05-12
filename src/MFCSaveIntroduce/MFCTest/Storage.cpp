#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "Ctxt/CtxtAPI.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"

map<string, string> mapData;
storage_ storage;

storage_::storage_(){
	Ctxt txt(configInfo.storage[ConfigInfo::txtPath].toValue<string>());
	txt.LoadTxt(2, "~!@#$%^&*()");
	int n = -1;
	while (n++ != txt.vectxt.size() - 1){
		int i = 0;
		string strValue;
		while (i++ != txt.vectxt.at(n).size() - 1){
			strValue = strValue + txt.vectxt.at(n).at(i) + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str();
		}
		mapData[txt.vectxt.at(n).at(0)] = strValue;
	}
}

void Storage::SaveTotxt(Person* person){
    string path = CGetPath::GetCurrentExePath() + "picture/";
    if (!PathIsDirectory(path.c_str())){
        ::CreateDirectory(path.c_str(), NULL);
    }

	string strPerson;
	if (person->picture1 != ""){
        bool x = CScreen::ChangeToBmp(path + person->name + person->birth.dateNumToString() + "1.bmp", person->picture1);
		person->picture1 = "picture/" + person->name + person->birth.dateNumToString() + "1.bmp";
	}
	if (person->picture2 != ""){
        CScreen::ChangeToBmp(path + person->name + person->birth.dateNumToString() + "2.bmp", person->picture2);
		person->picture2 = "picture/" + person->name + person->birth.dateNumToString() + "2.bmp";
	}
	if (person->picture3 != ""){
        CScreen::ChangeToBmp(path + person->name + person->birth.dateNumToString() + "3.bmp", person->picture3);
		person->picture3 = "picture/" + person->name + person->birth.dateNumToString() + "3.bmp";
	}
	strPerson = person->name + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->birth.dateNumToString() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->sex + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->marriage + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->education + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ CStringManager::Format("%d", person->tall).c_str() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ CStringManager::Format("%d", person->weight).c_str() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->house + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->car + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->household + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->houseAttri + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->houseAddr + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->jobName + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->jobNature + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ CStringManager::Format("%d", person->salary) + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->mobile + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->qq + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->weChat + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->fatherJob + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->fatherPension + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->motherJob + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->motherPension + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->picture1 + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->picture2 + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->picture3 + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->introduce + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str();

    mapData[person->name + person->birth.dateNumToString()] = strPerson;
	Ctxt txt(configInfo.storage[ConfigInfo::txtPath].toValue<string>());
	txt.AddLine("%s", (person->name + person->birth.dateNumToString() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str() + strPerson).c_str());
}

void Storage::GetFromtxt(Person* person){
    auto itmapData = mapData.find(person->name + person->birth.dateNumToString());
    if (itmapData == mapData.end()) return;
    string value = itmapData->second;
    vector<string> vecValue = CStringManager::split(value, configInfo.storage[ConfigInfo::Split].toValue<string>().c_str());
    person->name = vecValue[ConfigInfo::name];
    person->birth = IntDateTime(atoi(vecValue[ConfigInfo::birth].c_str()), 0);
    person->sex = vecValue[ConfigInfo::sex];
    person->marriage = vecValue[ConfigInfo::marriage];
    person->education = vecValue[ConfigInfo::education];
    person->tall = atoi(vecValue[ConfigInfo::tall].c_str());
    person->weight = atoi(vecValue[ConfigInfo::weight].c_str());
	person->house = vecValue[ConfigInfo::house];
	person->car = vecValue[ConfigInfo::car];
	person->household = vecValue[ConfigInfo::household];
	person->houseAttri = vecValue[ConfigInfo::houseAttri];
	person->houseAddr = vecValue[ConfigInfo::houseAddr];
    person->jobName = vecValue[ConfigInfo::jobName];
    person->jobNature = vecValue[ConfigInfo::jobNature];
    person->salary = atoi(vecValue[ConfigInfo::salary].c_str());
    person->mobile = vecValue[ConfigInfo::mobile];
    person->qq = vecValue[ConfigInfo::qq];
    person->weChat = vecValue[ConfigInfo::weChat];
    person->fatherJob = vecValue[ConfigInfo::fatherJob];
    person->fatherPension = vecValue[ConfigInfo::fatherPension];
    person->motherJob = vecValue[ConfigInfo::motherJob];
    person->motherPension = vecValue[ConfigInfo::motherPension];
    person->picture1 = vecValue[ConfigInfo::picture1];
    person->picture2 = vecValue[ConfigInfo::picture2];
    person->picture3 = vecValue[ConfigInfo::picture3];
    person->introduce = vecValue[ConfigInfo::introduce];
}