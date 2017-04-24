#include "Storage.h"
#include "ConfigInfo.h"
#include "Ctxt/CtxtAPI.h"
#include "Cini/CiniAPI.h"
#include "CScreen/CScreenAPI.h"

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
	CString strSalary;
	strSalary.Format("%d", person->salary);
	string strPerson;
	if (person->picture1 != ""){
		CScreen::ChangeToBmp("picture/" + person->name + person->birth.dateNumToString() + "1.bmp", person->picture1);
		person->picture1 = "picture/" + person->name + person->birth.dateNumToString() + "1.bmp";
	}
	if (person->picture2 != ""){
		CScreen::ChangeToBmp("picture/" + person->name + person->birth.dateNumToString() + "2.bmp", person->picture2);
		person->picture2 = "picture/" + person->name + person->birth.dateNumToString() + "2.bmp";
	}
	if (person->picture3 != ""){
		CScreen::ChangeToBmp("picture/" + person->name + person->birth.dateNumToString() + "3.bmp", person->picture3);
		person->picture3 = "picture/" + person->name + person->birth.dateNumToString() + "3.bmp";
	}
	strPerson = person->name + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->birth.dateNumToString() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->sex + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->marriage + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->education + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->tall + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->weight + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->jobName + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ person->jobNature + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
		+ (LPSTR)(LPCTSTR)strSalary + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str()
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

	mapData[person->name + person->birth.toString()] = strPerson;
	Ctxt txt(configInfo.storage[ConfigInfo::txtPath].toValue<string>());
	txt.AddLine("%s", (person->name + person->birth.dateNumToString() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str() + strPerson).c_str());
}