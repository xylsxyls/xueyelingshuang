#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "Cini/CiniAPI.h"
#include <atlimage.h>
#include "CSystem/CSystemAPI.h"
#include "CCharset/CCharsetAPI.h"

#define SetPersonString(person, strKey) storage.ini->WriteIni(#strKey, person->strKey, person->name + person->birth.dateNumToString())
#define SetPersonBirth(person, strKey) storage.ini->WriteIni(#strKey, person->strKey.dateNumToString(), person->name + person->birth.dateNumToString())
#define SetPersonInt(person, strKey) storage.ini->WriteIni(#strKey, CStringManager::Format("%d", person->strKey), person->name + person->birth.dateNumToString())

#define GetPersonString(person, strKey) person->strKey = storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024)
#define GetPersonBirth(person, strKey) person->strKey = IntDateTime(atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str()), 0)
#define GetPersonInt(person, strKey) person->strKey = atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str())

map<string, string> mapData;
storage_ storage;

storage_::storage_(){
    ini = new Cini(GetConfig(iniPath, string));
}

void Storage::SaveTotxt(Person* person){
    string path = CGetPath::GetCurrentExePath() + "picture/";
    if (!PathIsDirectory(CCharset::AnsiToUnicode(path).c_str())){
		::CreateDirectory(CCharset::AnsiToUnicode(path).c_str(), NULL);
    }

	string strPerson;
    string personKey = person->name + person->birth.dateNumToString();
	if (person->picture1 != ""){
		string dstPathBmp1 = path + personKey + "1.bmp";
		bool x = CScreen::ChangeToBmp(dstPathBmp1, person->picture1);
		CImage img;
		img.Load(CCharset::AnsiToUnicode(dstPathBmp1).c_str());
		string dstPathJpg1 = path + personKey + "1.jpg";
		img.Save(CCharset::AnsiToUnicode(dstPathJpg1).c_str());
		person->picture1 = dstPathJpg1;
		::remove(dstPathBmp1.c_str());
	}
	if (person->picture2 != ""){
		string dstPathBmp2 = path + personKey + "2.bmp";
		CScreen::ChangeToBmp(dstPathBmp2, person->picture2);
		CImage img;
		img.Load(CCharset::AnsiToUnicode(dstPathBmp2).c_str());
		string dstPathJpg2 = path + personKey + "2.jpg";
		img.Save(CCharset::AnsiToUnicode(dstPathJpg2).c_str());
		person->picture2 = dstPathJpg2;
		::remove(dstPathBmp2.c_str());
	}
	if (person->picture3 != ""){
		string dstPathBmp3 = path + personKey + "3.bmp";
		CScreen::ChangeToBmp(dstPathBmp3, person->picture3);
		CImage img;
		img.Load(CCharset::AnsiToUnicode(dstPathBmp3).c_str());
		string dstPathJpg3 = path + personKey + "3.jpg";
		img.Save(CCharset::AnsiToUnicode(dstPathJpg3).c_str());
		person->picture3 = dstPathJpg3;
		::remove(dstPathBmp3.c_str());
	}

    SetPersonString(person, name         );
    SetPersonBirth (person, birth        );
    SetPersonString(person, sex          );
    SetPersonString(person, marriage     );
    SetPersonString(person, education    );
    SetPersonInt   (person, tall         );
    SetPersonInt   (person, weight       );
    SetPersonString(person, house        );
    SetPersonString(person, car          );
    SetPersonString(person, household    );
    SetPersonString(person, houseAttri   );
    SetPersonString(person, houseAddr    );
    SetPersonString(person, jobName      );
    SetPersonString(person, jobNature    );
    SetPersonInt   (person, salary       );
    SetPersonString(person, mobile       );
    SetPersonString(person, qq           );
    SetPersonString(person, weChat       );
    SetPersonString(person, fatherJob    );
    SetPersonString(person, fatherPension);
    SetPersonString(person, motherJob    );
    SetPersonString(person, motherPension);
    SetPersonString(person, picture1     );
    SetPersonString(person, picture2     );
    SetPersonString(person, picture3     );
    SetPersonString(person, introduce    );

	CSystem::CopyFileOver(GetConfig(iniPathBk, string), GetConfig(iniPath, string), true);
}

void Storage::GetFromtxt(Person* person){
    GetPersonString(person, name         );
    GetPersonBirth (person, birth        );
    GetPersonString(person, sex          );
    GetPersonString(person, marriage     );
    GetPersonString(person, education    );
    GetPersonInt   (person, tall         );
    GetPersonInt   (person, weight       );
    GetPersonString(person, house        );
    GetPersonString(person, car          );
    GetPersonString(person, household    );
    GetPersonString(person, houseAttri   );
    GetPersonString(person, houseAddr    );
    GetPersonString(person, jobName      );
    GetPersonString(person, jobNature    );
    GetPersonInt   (person, salary       );
    GetPersonString(person, mobile       );
    GetPersonString(person, qq           );
    GetPersonString(person, weChat       );
    GetPersonString(person, fatherJob    );
    GetPersonString(person, fatherPension);
    GetPersonString(person, motherJob    );
    GetPersonString(person, motherPension);
    GetPersonString(person, picture1     );
    GetPersonString(person, picture2     );
    GetPersonString(person, picture3     );
    GetPersonString(person, introduce    );
}

void Storage::DeleteIntxt(const Person& person)
{
	void* vp = (void*)&person;
	Person* pPerson = (Person*)vp;
	GetPersonString(pPerson, picture1);
	GetPersonString(pPerson, picture2);
	GetPersonString(pPerson, picture3);
	if (pPerson->picture1 != "")
	{
		::remove(pPerson->picture1.c_str());
	}
	if (pPerson->picture2 != "")
	{
		::remove(pPerson->picture2.c_str());
	}
	if (pPerson->picture3 != "")
	{
		::remove(pPerson->picture3.c_str());
	}
	storage.ini->DeleteSection(person.name + person.birth.dateNumToString());
	CSystem::CopyFileOver(GetConfig(iniPathBk, string), GetConfig(iniPath, string), true);
}