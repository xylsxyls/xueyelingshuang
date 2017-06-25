#pragma once
#include "Person.h"
#include <vector>
#include "Cini/CiniAPI.h"
using namespace std;

class Search;
class Storage{
public:
	static vector<Person> FindFromtxt(const Search& search);
	static void ChangePersonToSearch(Person& person);
	static void Refresh();
	static bool IsEducationPass(const string& person, const string& search);
};

class storage_{
public:
    Cini* ini = nullptr;
	storage_();
	void GetInfo();
};