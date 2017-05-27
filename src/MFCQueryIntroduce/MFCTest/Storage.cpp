#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "Ctxt/CtxtAPI.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "Search.h"

#define SetPersonString(person, strKey) storage.ini->WriteIni(#strKey, person->strKey, person->name + person->birth.dateNumToString())
#define SetPersonBirth(person, strKey) storage.ini->WriteIni(#strKey, person->strKey.dateNumToString(), person->name + person->birth.dateNumToString())
#define SetPersonInt(person, strKey) storage.ini->WriteIni(#strKey, CStringManager::Format("%d", person->strKey), person->name + person->birth.dateNumToString())

#define GetPersonString(person, strKey) person->strKey = storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024)
#define GetPersonBirth(person, strKey) person->strKey = IntDateTime(atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str()), 0)
#define GetPersonInt(person, strKey) person->strKey = atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str())


map<string, Person> mapData;
storage_ storage;

storage_::storage_(){
    ini = new Cini(GetConfig(txtPath, string));
    /*
    Ctxt txt(GetConfig(txtPath, string));
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
    */
}

vector<Person> Storage::FindFromtxt(const Search& search){
    vector<Person> result;
	for (auto itmapData = mapData.begin(); itmapData != mapData.end(); itmapData++){
		Person person = itmapData->second;
		bool personResult = (person.birth <= search.bigBirth && person.birth >= search.smallBirth)
			&& (person.sex == search.sex)
			&& (search.marriage == configInfo.storage[notMarriage].toValue<string>()
					? person.marriage == configInfo.storage[notMarriage].toValue<string>()
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
		if (personResult == false) result.push_back(person);
	}
	return result;
}