#include <SDKDDKVer.h>
#include "CAttri.h"

CAttri::CAttri(){
	Type = 0;
	nLength = 0;
	bHasDefault = 0;
	bPrimaryKey = 0;
	bUniqueKey = 0;
	bNotNull = 0;
	bBlob = 0;
	bAutoIncrement = 0;
	bZeroFill = 0;
}

CAttri::CAttri(const CAttri& attri){
	Type = attri.Type;
	strFieldName = attri.strFieldName;
	nLength = attri.nLength;
	bHasDefault = attri.bHasDefault;
	vDefault = attri.vDefault;
	bPrimaryKey = attri.bPrimaryKey;
	bUniqueKey = attri.bUniqueKey;
	bNotNull = attri.bNotNull;
	bBlob = attri.bBlob;
	bAutoIncrement = attri.bAutoIncrement;
	bZeroFill = attri.bZeroFill;
}

CAttri CAttri::operator = (const CAttri& attri){
	Type = attri.Type;
	strFieldName = attri.strFieldName;
	nLength = attri.nLength;
	bHasDefault = attri.bHasDefault;
	vDefault = attri.vDefault;
	bPrimaryKey = attri.bPrimaryKey;
	bUniqueKey = attri.bUniqueKey;
	bNotNull = attri.bNotNull;
	bBlob = attri.bBlob;
	bAutoIncrement = attri.bAutoIncrement;
	bZeroFill = attri.bZeroFill;
	return *this;
}

//CAttri& CAttri::operator->(){
//	return *this;
//}