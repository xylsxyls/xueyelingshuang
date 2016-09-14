#include "CCondition.h"

CCondition CCondition::operator&&(CCondition con){
	strSQL = strSQL + " and " + con.strSQL;
	return *this;
}

CCondition CCondition::operator||(CCondition con){
	strSQL = strSQL + " or " + con.strSQL;
	return *this;
}

CCondition CCondition::operator!(){
	strSQL = " not " + strSQL;
	return *this;
}