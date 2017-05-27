#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "Ctxt/CtxtAPI.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "Cini/CiniAPI.h"

#define SetPersonString(person, strKey) storage.ini->WriteIni(#strKey, person->strKey, person->name + person->birth.dateNumToString())
#define SetPersonBirth(person, strKey) storage.ini->WriteIni(#strKey, person->strKey.dateNumToString(), person->name + person->birth.dateNumToString())
#define SetPersonInt(person, strKey) storage.ini->WriteIni(#strKey, CStringManager::Format("%d", person->strKey), person->name + person->birth.dateNumToString())

#define GetPersonString(person, strKey) person->strKey = storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024)
#define GetPersonBirth(person, strKey) person->strKey = IntDateTime(atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str()), 0)
#define GetPersonInt(person, strKey) person->strKey = atoi(storage.ini->ReadIni(#strKey, person->name + person->birth.dateNumToString(), 1024).c_str())

map<string, string> mapData;
storage_ storage;

storage_::storage_(){
    ini = new Cini(GetConfig(txtPath, string));
    /*
    Ctxt txt(GetConfig(txtPath, string));
    txt.OpenFile_a();
    string data;
    int readresult = -1;
    int readsize = 100;
    int size = 0;
    while (true){
        data.resize(size + 100);
        readresult = fread(&data[size], 1, 100, txt.pFile);
        if (readresult != readsize){
            data.resize(size + readresult);
            break;
        }
        size = size + 100;
    }
    static CStopWatch stopwatch;
    //json.LoadJson(data);
    int xxx = stopwatch.GetWatchTime();
    txt.CloseFile();*/
    /*
	int n = -1;
	while (n++ != txt.vectxt.size() - 1){
		int i = 0;
		string strValue;
		while (i++ != txt.vectxt.at(n).size() - 1){
            strValue = strValue + txt.vectxt.at(n).at(i) + GetConfig(Split, string).c_str();
		}
		mapData[txt.vectxt.at(n).at(0)] = strValue;
	}*/
}

void Storage::SaveTotxt(Person* person){
    string path = CGetPath::GetCurrentExePath() + "picture/";
    if (!PathIsDirectory(path.c_str())){
        ::CreateDirectory(path.c_str(), NULL);
    }

	string strPerson;
    string personKey = person->name + person->birth.dateNumToString();
	if (person->picture1 != ""){
        bool x = CScreen::ChangeToBmp(path + personKey + "1.bmp", person->picture1);
        person->picture1 = "picture/" + personKey + "1.bmp";
	}
	if (person->picture2 != ""){
        CScreen::ChangeToBmp(path + personKey + "2.bmp", person->picture2);
        person->picture2 = "picture/" + personKey + "2.bmp";
	}
	if (person->picture3 != ""){
        CScreen::ChangeToBmp(path + personKey + "3.bmp", person->picture3);
        person->picture3 = "picture/" + personKey + "3.bmp";
	}
    
    //Cini ini("");
    
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
    //mapData[person->name + person->birth.dateNumToString()] = strPerson;
    /*
	Ctxt txt(GetConfig(txtPath, string));
    txt.OpenFile_w();
    fwrite(data.c_str(), 1, data.length(), txt.pFile);
    txt.CloseFile();
    Ctxt txtBk(GetConfig(txtPathBk, string));
    txtBk.OpenFile_w();
    fwrite(data.c_str(), 1, data.length(), txtBk.pFile);
    txtBk.CloseFile();*/
	//txt.AddLine("%s", (person->name + person->birth.dateNumToString() + configInfo.storage[ConfigInfo::Split].toValue<string>().c_str() + strPerson).c_str());
}

void Storage::GetFromtxt(Person* person){
    static CStopWatch so;
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
    int xxx = so.GetWatchTime();
    int x = 3;
}