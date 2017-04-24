#pragma once
#include "Person.h"
#include <afx.h>

class Manager{
public:
	int Check(HWND hwnd,Person* person);
	void ShowError(int error);
	void Save(Person* person);
};