#pragma once
#include "Person.h"
#include <afx.h>

class Manager{
public:
	static int Check(HWND hwnd, Person* person);
    static int Query(HWND hwnd, Person* person);
    static void ShowError(int error);
    static void ShowPerson(HWND hwnd, Person* person);
    static void SavePerson(Person* person);
};