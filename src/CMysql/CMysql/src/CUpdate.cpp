#include <SDKDDKVer.h>
#include "CUpdate.h"
#include "CStringManager/CStringManagerAPI.h"

CUpdate::CUpdate()
{

}

CUpdate::CUpdate(const CUpdate& upd)
{
	mapValue = upd.mapValue;
	strCurrentField = upd.strCurrentField;
}

CUpdate CUpdate::operator=(const CUpdate& upd)
{
	mapValue = upd.mapValue;
	strCurrentField = upd.strCurrentField;
	return *this;
}

CUpdate CUpdate::operator=(const CValue& value)
{
	mapValue[strCurrentField] = value;
	return *this;
}

CUpdate& CUpdate::operator[](const std::string& Field)
{
	strCurrentField = Field;
	return *this;
}

std::string CUpdate::toString()
{
	std::string strResult;
	for (auto it = mapValue.begin(); it != mapValue.end(); it++)
{
		strResult = strResult + it->first + "=" + it->second.m_strValue + ",";
	}
	CStringManager::Delete(strResult, strResult.length() - 1, 1);
	return strResult;
}