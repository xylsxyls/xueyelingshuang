#include <SDKDDKVer.h>
#include "stdafx.h"
#include "Manager.h"
#include "Collect.h"
#include "ErrorInfo.h"
#include "Storage.h"
#include "PicDlgQuery.h"
#include "CCharset/CCharsetAPI.h"

void Manager::SearchInfo(HWND hwnd, Search* search){
	Collect::GetSearchInfo(search, hwnd);
}

vector<Person> Manager::Find(const Search& search){
	return Storage::FindFromtxt(search);
}

void Manager::ShowDlg(const vector<Person>& vecPerson){
	if (vecPerson.empty())
	{
		return;
	}
    CPicDlgQuery picDlgQuery;
    picDlgQuery.init(vecPerson);
    picDlgQuery.DoModal();
}

void Manager::ShowError(int error){
	AfxMessageBox(CCharset::AnsiToUnicode(errorInfo.storage[error].toValue<string>()).c_str());
}

void Manager::Refresh()
{
	Storage::Refresh();
}