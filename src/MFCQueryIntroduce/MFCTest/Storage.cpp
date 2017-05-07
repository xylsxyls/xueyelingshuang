#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "Ctxt/CtxtAPI.h"
#include "Cini/CiniAPI.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "Search.h"

map<string, Person> mapData;
storage_ storage;

storage_::storage_(){
	Ctxt txt(configInfo.storage[ConfigInfo::txtPath].toValue<string>());
	txt.LoadTxt(2, "~!@#$%^&*()");
	int n = -1;
	while (n++ != txt.vectxt.size() - 1){
		Person person;
		person.name = txt.vectxt.at(n).at(ConfigInfo::name + ConfigInfo::txtOffset);
		person.birth = IntDateTime(atoi(txt.vectxt.at(n).at(ConfigInfo::birth + ConfigInfo::txtOffset).c_str()), 0);
		person.sex = txt.vectxt.at(n).at(ConfigInfo::sex + ConfigInfo::txtOffset);
		person.marriage = txt.vectxt.at(n).at(ConfigInfo::marriage + ConfigInfo::txtOffset);
		person.education = txt.vectxt.at(n).at(ConfigInfo::education + ConfigInfo::txtOffset);
		person.tall = atoi(txt.vectxt.at(n).at(ConfigInfo::tall + ConfigInfo::txtOffset).c_str());
		person.weight = atoi(txt.vectxt.at(n).at(ConfigInfo::weight + ConfigInfo::txtOffset).c_str());
		person.house = txt.vectxt.at(n).at(ConfigInfo::house + ConfigInfo::txtOffset);
		person.car = txt.vectxt.at(n).at(ConfigInfo::car + ConfigInfo::txtOffset);
		person.household = txt.vectxt.at(n).at(ConfigInfo::household + ConfigInfo::txtOffset);
		person.houseAttri = txt.vectxt.at(n).at(ConfigInfo::houseAttri + ConfigInfo::txtOffset);
		person.houseAddr = txt.vectxt.at(n).at(ConfigInfo::houseAddr + ConfigInfo::txtOffset);
		person.jobName = txt.vectxt.at(n).at(ConfigInfo::jobName + ConfigInfo::txtOffset);
		person.jobNature = txt.vectxt.at(n).at(ConfigInfo::jobNature + ConfigInfo::txtOffset);
		person.salary = atoi(txt.vectxt.at(n).at(ConfigInfo::salary + ConfigInfo::txtOffset).c_str());
		person.mobile = txt.vectxt.at(n).at(ConfigInfo::mobile + ConfigInfo::txtOffset);
		person.qq = txt.vectxt.at(n).at(ConfigInfo::qq + ConfigInfo::txtOffset);
		person.weChat = txt.vectxt.at(n).at(ConfigInfo::weChat);
		person.fatherJob = txt.vectxt.at(n).at(ConfigInfo::fatherJob + ConfigInfo::txtOffset);
		person.fatherPension = txt.vectxt.at(n).at(ConfigInfo::fatherPension + ConfigInfo::txtOffset);
		person.motherJob = txt.vectxt.at(n).at(ConfigInfo::motherJob + ConfigInfo::txtOffset);
		person.motherPension = txt.vectxt.at(n).at(ConfigInfo::motherPension + ConfigInfo::txtOffset);
		person.picture1 = txt.vectxt.at(n).at(ConfigInfo::picture1 + ConfigInfo::txtOffset);
		person.picture2 = txt.vectxt.at(n).at(ConfigInfo::picture2 + ConfigInfo::txtOffset);
		person.picture3 = txt.vectxt.at(n).at(ConfigInfo::picture3 + ConfigInfo::txtOffset);
		person.introduce = txt.vectxt.at(n).at(ConfigInfo::introduce + ConfigInfo::txtOffset);
		mapData[txt.vectxt.at(n).at(ConfigInfo::txtPerson)] = person;
	}
}

vector<Person> Storage::FindFromtxt(const Search& search){
	vector<Person> result;
	for (auto itmapData = mapData.begin(); itmapData != mapData.end(); itmapData++){
		Person person = itmapData->second;
		bool personResult = (person.birth <= search.bigBirth && person.birth >= search.smallBirth)
			&& (person.sex == search.sex)
			&& (search.marriage == configInfo.storage[ConfigInfo::notMarriage].toValue<string>()
					? person.marriage == configInfo.storage[ConfigInfo::notMarriage].toValue<string>()
					: person.marriage != configInfo.storage[ConfigInfo::notMarriage].toValue<string>())
			&& (person.tall <= search.bigTall && person.tall >= search.smallTall)
			&& (search.house == configInfo.storage[ConfigInfo::notHasHouse].toValue<string>()
					? person.house == configInfo.storage[ConfigInfo::notHasHouse].toValue<string>()
					: person.house != configInfo.storage[ConfigInfo::notHasHouse].toValue<string>())
			&& (person.weight <= search.bigWeight && person.weight >= search.smallWeight)
			&& (search.education == "本科以上"
					? person.education != "中专" && person.education != "大专"
					: search.education == "硕士以上"
					? person.education == "学硕" || person.education == "专硕"
							|| person.education == "硕士双学位" || person.education == "硕士海外留学"
							|| person.education == "博士" || person.education == "博士海外留学"
					: search.education == "博士"
					? person.education == "博士" || person.education == "博士海外留学"
					: 0)
			&& (person.salary >= search.salary);
		if (personResult == true) result.push_back(person);
	}
	return result;
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
		+ person->tall + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->weight + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
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
    person->tall = vecValue[ConfigInfo::tall];
    person->weight = vecValue[ConfigInfo::weight];
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