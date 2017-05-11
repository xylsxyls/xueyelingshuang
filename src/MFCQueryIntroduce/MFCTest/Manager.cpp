#include <SDKDDKVer.h>
#include "Manager.h"
#include "Collect.h"
#include "ErrorInfo.h"
#include "Storage.h"
#include "PicDlgQuery.h"

void Manager::SearchInfo(HWND hwnd, Search* search){
	Collect::GetSearchInfo(search, hwnd);
}

vector<Person> Manager::Find(const Search& search){
	return Storage::FindFromtxt(search);
}

void Manager::ShowDlg(vector<Person> vecPerson){
    CPicDlgQuery picDlgQuery;
    picDlgQuery.init(vecPerson);
    picDlgQuery.DoModal();
}

void Manager::ShowError(int error){
	AfxMessageBox(errorInfo.storage[error].toValue<string>().c_str());
}