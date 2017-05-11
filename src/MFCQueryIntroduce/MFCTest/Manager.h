#pragma once
#include <afx.h>
#include "Person.h"
#include <vector>

#include "Search.h"
using namespace std;

//class Search;
class Manager{
public:
	static void SearchInfo(HWND hwnd, Search* search);
	static vector<Person> Find(const Search& search);
	static void ShowDlg(vector<Person> vecPerson);
    static void ShowError(int error);
};