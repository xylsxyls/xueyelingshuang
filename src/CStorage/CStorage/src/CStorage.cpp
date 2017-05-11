#include <SDKDDKVer.h>
#include "CStorage.h"

CStorage::CStorage(){
	changeString = false;
	changeInt = false;
	nCurrentKey = 0;
}

CStorage& CStorage::operator[](CString key){
	strCurrentKey = key;
	changeString = true;
	return *this;
}

CStorage& CStorage::operator[](int key){
	nCurrentKey = key;
	changeInt = true;
	return *this;
}

CStorage CStorage::operator=(const char* value){
    if (changeString == true){
        mapData[strCurrentKey] = string(value);
        changeString = false;
    }
    if (changeInt == true){
        mapDataInt[nCurrentKey] = string(value);
        changeInt = false;
    }
    return *this;
}

CStorage CStorage::operator=(any anyValue){
	if (changeString == true){
		mapData[strCurrentKey] = anyValue;
		changeString = false;
	}
	if (changeInt == true){
		mapDataInt[nCurrentKey] = anyValue;
		changeInt = false;
	}
	return *this;
}

/*
int main(){
	CStorage stor;
	stor["2222"] = CString("3333");
	//stor["2222"] = 3.3;
	CString xxxxxxxx = stor["2222"].toValue<CString>();

	map<std::string,boost::any> map;
	any ai;
	any as;
	ai = 3;
	ai = std::string("5");
	ai = 6;
	as = std::string("2");
	string x = "1";
	map[x] = 1;
	x = std::string("2");
	map[x] = std::string("22");
	string str = map["2"].type().name();
	
	try   
	{
		int xxx = any_cast<int>(map["2"]);
	}
	catch(boost::bad_any_cast & e)
	{
		string xxxx = e.what();
		int x = 3;
	}   
	//as = "2";
	//ai = "1";
	//char*s = any_cast<char*>(ai);
	return 0;
}*/