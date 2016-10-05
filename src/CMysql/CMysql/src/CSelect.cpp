#include <SDKDDKVer.h>
#include "CSelect.h"
#include <map>

CSelect& CSelect::operator()(CString Table){
	tf.Table = Table;
	return *this;
}

CSelect& CSelect::operator[](CString Field){
	tf.Field = Field;
	listTf.push_back(tf);
	return *this;
}

CSelect::CSelect(){

}

CSelect::CSelect(const CSelect& sel){
	this->tf = sel.tf;
	this->listTf = sel.listTf;
}

CSelect CSelect::operator=(const CSelect& sel){
	this->tf = sel.tf;
	this->listTf = sel.listTf;
	return *this;
}

CString CSelect::toCString(){
	CString strResult;
	for(auto it = listTf.begin();it != listTf.end();it++){
		strResult = strResult + it->Table + "." + it->Field + ",";
	}
	strResult.Delete(strResult.GetLength() - 1,1);
	return strResult;
}

CString CSelect::toTableString(){
	//因为要参与去重，所以把所有的表名存入map中再遍历
	CString strSelectTable;
	map<CString,int> mapTable;
	for(auto itList = listTf.begin();itList != listTf.end();itList++){
		mapTable[itList->Table] = 1;
	}
	for(auto itMap = mapTable.begin();itMap != mapTable.end();itMap++){
		strSelectTable = strSelectTable + itMap->first + ",";
	}
	strSelectTable.Delete(strSelectTable.GetLength() - 1,1);
	return strSelectTable;
}