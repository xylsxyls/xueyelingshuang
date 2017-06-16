#include <SDKDDKVer.h>
#include "Storage.h"
#include "ConfigInfo.h"
#include "CScreen/CScreenAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "Search.h"

#define SetPersonString(person, strKey) storage.ini->WriteIni(#strKey, person->strKey, person->name + person->birth.dateNumToString())
#define SetPersonBirth(person, strKey) storage.ini->WriteIni(#strKey, person->strKey.dateNumToString(), person->name + person->birth.dateNumToString())
#define SetPersonInt(person, strKey) storage.ini->WriteIni(#strKey, CStringManager::Format("%d", person->strKey), person->name + person->birth.dateNumToString())

#define GetPersonString(section, strKey, person) person.strKey = storage.ini->ReadIni(#strKey, section, 1024)
#define GetPersonBirth(section, strKey, person) person.strKey = IntDateTime(atoi(storage.ini->ReadIni(#strKey, section).c_str()), 0)
#define GetPersonInt(section, strKey, person) person.strKey = atoi(storage.ini->ReadIni(#strKey, section).c_str())

vector<Person> vecData;
storage_ storage;

storage_::storage_(){
    ini = ::new Cini(GetConfig(txtPath, string));
	vector<string> vecSection = ini->GetSection(GetConfig(maxPeople, int));
	int i = -1;
	while (i++ != vecSection.size() - 1)
	{
		Person person;
		GetPersonString(vecSection.at(i), name,          person);
		GetPersonBirth (vecSection.at(i), birth,         person);
		GetPersonString(vecSection.at(i), sex,           person);
		GetPersonString(vecSection.at(i), marriage,      person);
		GetPersonString(vecSection.at(i), education,     person);
		GetPersonInt   (vecSection.at(i), tall,          person);
		GetPersonInt   (vecSection.at(i), weight,        person);
		GetPersonString(vecSection.at(i), house,         person);
		GetPersonString(vecSection.at(i), car,           person);
		GetPersonString(vecSection.at(i), household,     person);
		GetPersonString(vecSection.at(i), houseAttri,    person);
		GetPersonString(vecSection.at(i), houseAddr,     person);
		GetPersonString(vecSection.at(i), jobName,       person);
		GetPersonString(vecSection.at(i), jobNature,     person);
		GetPersonInt   (vecSection.at(i), salary,        person);
		GetPersonString(vecSection.at(i), mobile,        person);
		GetPersonString(vecSection.at(i), qq,            person);
		GetPersonString(vecSection.at(i), weChat,        person);
		GetPersonString(vecSection.at(i), fatherJob,     person);
		GetPersonString(vecSection.at(i), fatherPension, person);
		GetPersonString(vecSection.at(i), motherJob,     person);
		GetPersonString(vecSection.at(i), motherPension, person);
		GetPersonString(vecSection.at(i), picture1,      person);
		GetPersonString(vecSection.at(i), picture2,      person);
		GetPersonString(vecSection.at(i), picture3,      person);
		GetPersonString(vecSection.at(i), introduce,     person);
		vecData.push_back(person);
	}
}

vector<Person> Storage::FindFromtxt(const Search& search){
    vector<Person> result;
	int i = -1;
	while (i++ != vecData.size() - 1)
	{
		Person person = vecData.at(i);
		bool personResult = (person.birth <= search.bigBirth && person.birth >= search.smallBirth)
			&& (person.sex == search.sex)
			&& (person.tall <= search.bigTall && person.tall >= search.smallTall)
			&& (person.weight <= search.bigWeight && person.weight >= search.smallWeight)
			&& (person.salary >= search.salary);
		if (personResult == false)
		{
			continue;
		}
		ChangePersonToSearch(person);
		personResult = (search.marriage == GetConfig(noMatter, string) || person.marriage == search.marriage)
			&& (search.marriage == GetConfig(noMatter, string) || person.house == search.house)
			&& (search.marriage == GetConfig(noMatter, string) || person.education == search.education);
		if (personResult == true)
		{
			result.push_back(vecData.at(i));
		}
	}
	return result;
}

void Storage::ChangePersonToSearch(Person& person)
{
	if (person.marriage == GetConfig(notMarriage1, string))
	{
		person.marriage = GetConfig(notMarriage, string);
	}
	else if (person.marriage == GetConfig(hasMarriage1, string)
		|| person.marriage == GetConfig(hasMarriage2, string))
	{
		person.marriage = GetConfig(hasMarriage, string);
	}

	if (person.house == GetConfig(noHouse1, string))
	{
		person.house = GetConfig(noHouse, string);
	}
	else if (person.house == GetConfig(hasHouse1, string)
		|| person.house == GetConfig(hasHouse2, string))
	{
		person.house = GetConfig(hasHouse, string);
	}

	if (person.education == GetConfig(junior1, string)
		|| person.education == GetConfig(junior2, string))
	{
		person.education = GetConfig(junior, string);
	}
	else if (person.education == GetConfig(college1, string)
		|| person.education == GetConfig(college2, string)
		|| person.education == GetConfig(college3, string)
		|| person.education == GetConfig(college4, string)
		|| person.education == GetConfig(college5, string)
		|| person.education == GetConfig(college6, string))
	{
		person.education = GetConfig(college, string);
	}
	else if (person.education == GetConfig(graduate1, string)
		|| person.education == GetConfig(graduate2, string)
		|| person.education == GetConfig(graduate3, string)
		|| person.education == GetConfig(graduate4, string))
	{
		person.education = GetConfig(graduate, string);
	}
	else if (person.education == GetConfig(philosophy1, string)
		|| person.education == GetConfig(philosophy2, string))
	{
		person.education = GetConfig(philosophy, string);
	}
}