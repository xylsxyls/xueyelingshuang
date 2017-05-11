#pragma once
#include "Person.h"
#include <vector>
using namespace std;

class Search;
class Storage{
public:
	static vector<Person> FindFromtxt(const Search& search);
};

class storage_{
public:
	storage_();
};