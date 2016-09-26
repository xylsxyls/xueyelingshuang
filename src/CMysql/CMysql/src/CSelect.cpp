#include <SDKDDKVer.h>
#include "CSelect.h"

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

CString CSelect::ToCString(){
	CString strResult;
	for(auto it = listTf.begin();it != listTf.end();it++){
		strResult = strResult + it->Table + "." + it->Field + ",";
	}
	strResult.Delete(strResult.GetLength() - 1,1);
	return strResult;
}