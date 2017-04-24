#include "Manager.h"
#include "Collect.h"
#include "ErrorInfo.h"
#include <afxwin.h>
#include "Storage.h"

int Manager::Check(HWND hwnd, Person* person){
	Collect collect;
	return collect.GetPersonInfo(person, hwnd);
}

void Manager::ShowError(int error){
	AfxMessageBox(errorInfo.storage[error].toValue<string>().c_str());
}

void Manager::Save(Person* person){
	Storage storage;
	storage.SaveTotxt(person);
}