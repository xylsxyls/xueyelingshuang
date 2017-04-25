#include <SDKDDKVer.h>
#include "Manager.h"
#include "Collect.h"
#include "ErrorInfo.h"
#include <afxwin.h>
#include "Storage.h"

int Manager::Check(HWND hwnd, Person* person){
    return Collect::GetPersonInfo(person, hwnd);
}

int Manager::Query(HWND hwnd, Person* person){
    return Collect::GetQueryInfo(person, hwnd);
}

void Manager::ShowError(int error){
	AfxMessageBox(errorInfo.storage[error].toValue<string>().c_str());
}

void Manager::ShowPerson(HWND hwnd, Person* person){
    Storage::GetFromtxt(person);
    Collect::ShowPerson(person, hwnd);
}

void Manager::SavePerson(Person* person){
    Storage::SaveTotxt(person);
}