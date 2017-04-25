#pragma once
#include "Person.h"

class Storage{
public:
	static void SaveTotxt(Person* person);
    static void GetFromtxt(Person* person);
};

class storage_{
public:
	storage_();
};