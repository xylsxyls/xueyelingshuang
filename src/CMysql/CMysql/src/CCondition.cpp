#include <SDKDDKVer.h>
#include "CCondition.h"

CCondition& CCondition::operator&&(CCondition con){
	//第一次
	if(strSQL == ""){
		strSQL = con.strSQL;
		return *this;
	}
	strSQL = "(" + strSQL + " and " + con.strSQL + ")";
	return *this;
}

CCondition& CCondition::operator||(CCondition con){
	//第一次
	if(strSQL == ""){
		strSQL = con.strSQL;
		return *this;
	}
	strSQL = "(" + strSQL + " or " + con.strSQL + ")";
	return *this;
}

CCondition& CCondition::operator!(){
	//第一次
	if(strSQL == ""){
		return *this;
	}
	strSQL = "( not " + strSQL + ")";;
	return *this;
}