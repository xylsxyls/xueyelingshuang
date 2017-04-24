#pragma once
#include "Person.h"
#include <afx.h>

class Collect{
public:
	int GetPersonInfo(Person* person,HWND hwnd);
private:
	int CheckName(Person* person);
	int CheckBirth(Person* person);
	int CheckSex(Person* person);
	int CheckMarriage(Person* person);
	int CheckEducation(Person* person);
};