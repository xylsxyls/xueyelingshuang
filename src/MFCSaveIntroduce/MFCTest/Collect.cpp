#include "Collect.h"
#include <afx.h>
#include "ConfigInfo.h"
#include "ErrorInfo.h"

int Collect::GetPersonInfo(Person* person,HWND hwnd){
	//?发送消息让界面吧person填满
	::SendMessage(hwnd, configInfo.storage[ConfigInfo::FillPersonInt].toValue<int>(), NULL, (LPARAM)person);
	int result = 0;
	(result = CheckName(person)) || (result = CheckBirth(person)) || (result = CheckSex(person)) || (result = CheckMarriage(person)) || (result = CheckEducation(person));
	return result;
}

int Collect::CheckName(Person* person){
	if (person->name.length() <= 2 || person->name.length() > 10) return ErrorInfo::Name;
	else return 0;
}

int Collect::CheckBirth(Person* person){
	if (person->birth.toString().find("-00") != -1 || person->birth.toString().find("0000") != -1) return ErrorInfo::Birth;
	else return 0;
}

int Collect::CheckSex(Person* person){
	if (person->sex == "") return ErrorInfo::Sex;
	else return 0;
}

int Collect::CheckMarriage(Person* person){
	if (person->marriage == "") return ErrorInfo::Marriage;
	else return 0;
}

int Collect::CheckEducation(Person* person){
	if (person->education == "") return ErrorInfo::Education;
	else return 0;
}