#include <SDKDDKVer.h>
#include "CTableField.h"
#include "CValue.h"
#include "CCondition.h"

CTableField CTableField::operator()(CString Table){
	this->Table = Table;
	return *this;
}

CTableField CTableField::operator[](CString Field){
	this->Field = Field;
	return *this;
}

CTableField::CTableField(){

}

CTableField::CTableField(const CTableField& tf){
	this->Table = tf.Table;
	this->Field = tf.Field;
}

CTableField CTableField::operator=(const CTableField& tf){
	this->Table = tf.Table;
	this->Field = tf.Field;
	return *this;
}

CCondition CTableField::operator==(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s=%s",Table,Table,Field,value.strValue);
	return con;
}

CCondition CTableField::operator!=(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s!=%s",Table,Table,Field,value.strValue);
	return con;
}

CCondition CTableField::operator<(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s<%s",Table,Table,Field,value.strValue);
	return con;
}

CCondition CTableField::operator>(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s>%s",Table,Table,Field,value.strValue);
	return con;
}

CCondition CTableField::operator<=(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s<=%s",Table,Table,Field,value.strValue);
	return con;
}

CCondition CTableField::operator>=(const CValue& value){
	CCondition con;
	con.strSQL.Format("%s where %s.%s>=%s",Table,Table,Field,value.strValue);
	return con;
}

//联合相等
CCondition CTableField::operator&&(const CTableField& tf){
	CCondition con;
	con.strSQL.Format("%s inner join %s on %s.%s=%s.%s",Table,tf.Table,Table,Field,tf.Table,tf.Field);
	return con;
}

//左联合，左边的数据量大于等于右边的数据量
CCondition CTableField::operator>=(const CTableField& tf){
	CCondition con;
	con.strSQL.Format("%s left outer join %s on %s.%s=%s.%s",Table,tf.Table,Table,Field,tf.Table,tf.Field);
	return con;
}

//右联合，左边的数据量小于等于右边的数据量
CCondition CTableField::operator<=(const CTableField& tf){
	CCondition con;
	con.strSQL.Format("%s right outer join %s on %s.%s=%s.%s",Table,tf.Table,Table,Field,tf.Table,tf.Field);
	return con;
}

CCondition CTableField::operator||(const CTableField& tf){
	CCondition con;
	con.strSQL.Format("%s full outer join %s on %s.%s=%s.%s",Table,tf.Table,Table,Field,tf.Table,tf.Field);
	return con;
}