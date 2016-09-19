#include <SDKDDKVer.h>
#include "CAttri.h"

CAttri::CAttri(){
	Type = 0;
	nlength = 0;
	bPrimaryKey = 0;
	bNotNull = 0;
	nAutoIncrement = 0;
	bZeroFill = 0;
}

CAttri::CAttri(const CAttri& attri){
	Type = attri.Type;
	Name = attri.Name;
	nlength = attri.nlength;
	vDefault = attri.vDefault;
	bPrimaryKey = attri.bPrimaryKey;
	bNotNull = attri.bNotNull;
	nAutoIncrement = attri.nAutoIncrement;
	bZeroFill = attri.bZeroFill;
	strComment = attri.strComment;
}

CAttri CAttri::operator = (const CAttri& attri){
	Type = attri.Type;
	Name = attri.Name;
	nlength = attri.nlength;
	vDefault = attri.vDefault;
	bPrimaryKey = attri.bPrimaryKey;
	bNotNull = attri.bNotNull;
	nAutoIncrement = attri.nAutoIncrement;
	bZeroFill = attri.bZeroFill;
	strComment = attri.strComment;
	return *this;
}