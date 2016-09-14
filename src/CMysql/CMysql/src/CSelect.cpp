#include "CSelect.h"

CSelect CSelect::operator()(CString Table){
	tf.Table = Table;
	return *this;
}

CSelect CSelect::operator[](CString Field){
	tf.Field = Field;
	list.push_back(tf);
	return *this;
}

CSelect::CSelect(){

}

CSelect::CSelect(const CSelect& sel){
	this->tf = sel.tf;
	this->list = sel.list;
}

CSelect CSelect::operator=(const CSelect& sel){
	this->tf = sel.tf;
	this->list = sel.list;
	return *this;
}