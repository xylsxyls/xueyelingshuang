#pragma once
#include "Person.h"
#include "Cini/CiniAPI.h"

class Storage{
public:
	static void SaveTotxt(Person* person);
    static void GetFromtxt(Person* person);
	static void DeleteIntxt(const Person& person);
};

class storage_{
public:
    Cini* ini;
	storage_();
};