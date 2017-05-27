#pragma once
#include "Person.h"
#include <vector>
#include "Cini/CiniAPI.h"
using namespace std;

class Search;
class Storage{
public:
	static vector<Person> FindFromtxt(const Search& search);
};

class storage_{
public:
    Cini* ini;
	storage_();
};