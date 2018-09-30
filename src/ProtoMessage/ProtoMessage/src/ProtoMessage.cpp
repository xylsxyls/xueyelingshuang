#include "ProtoMessage.h"
#include "ProtoMessage.pb.h"
#pragma comment(lib, "libprotobuf.lib")

ProtoMessage::ProtoMessage():
m_protoMsg(nullptr)
{
	m_protoMsg = new pt::ProtoMessage;
}

void ProtoMessage::setCode(int32_t code, const std::string& errorMsg /*= ""*/)
{
	auto rspCode = m_protoMsg->mutable_rspcode();
	if (rspCode == nullptr)
	{
		return;
	}
	rspCode->set_code(code);
	rspCode->set_errormessage(errorMsg);
}

int32_t ProtoMessage::getCode() const
{
	auto rspCode = m_protoMsg->mutable_rspcode();
	if (rspCode == nullptr)
	{
		return PROTO_ERROR;
	}
	return rspCode->code();
}

std::string ProtoMessage::getErrorMessage() const
{
	auto rspCode = m_protoMsg->mutable_rspcode();
	if (rspCode == nullptr)
	{
		return "";
	}
	return rspCode->errormessage();
}

void ProtoMessage::setKeyMap(const std::map<std::string, std::string>& mapData, const std::string& key /*= "global"*/)
{
	m_keyMapData[key] = mapData;
}

void ProtoMessage::setKeyList(const std::list<std::string>& listData, const std::string& key /*= "global"*/)
{
	m_keyListData[key] = listData;
}

void ProtoMessage::setKeyTable(const std::vector<std::vector<std::string>>& tableData, const std::string& key /*= "global"*/)
{
	m_keyTableData[key] = tableData;
}

std::map<std::string, std::string> ProtoMessage::getMap(const std::string& key /*= "global"*/)
{
	std::map<std::string, std::string> result;
	getMap(result, key);
	return result;
}

std::list<std::string> ProtoMessage::getList(const std::string& key /*= "global"*/)
{
	std::list<std::string> result;
	getList(result, key);
	return result;
}

std::vector<std::vector<std::string>> ProtoMessage::getTable(const std::string& key /*= "global"*/)
{
	std::vector<std::vector<std::string>> result;
	getTable(result, key);
	return result;
}

bool ProtoMessage::getMap(std::map<std::string, std::string>& mapData, const std::string& key /*= "global"*/)
{
	auto itKeyMapData = m_keyMapData.find(key);
	if (itKeyMapData == m_keyMapData.end())
	{
		return false;
	}
	mapData = itKeyMapData->second;
	return true;
}

bool ProtoMessage::getList(std::list<std::string>& listData, const std::string& key /*= "global"*/)
{
	auto itKeyListData = m_keyListData.find(key);
	if (itKeyListData == m_keyListData.end())
	{
		return false;
	}
	listData = itKeyListData->second;
	return true;
}

bool ProtoMessage::getTable(std::vector<std::vector<std::string>>& tableData, const std::string& key /*= "global"*/)
{
	auto itKeyTableData = m_keyTableData.find(key);
	if (itKeyTableData == m_keyTableData.end())
	{
		return false;
	}
	tableData = itKeyTableData->second;
	return true;
}

bool ProtoMessage::from(const std::string& data)
{
	if (!m_protoMsg->ParseFromString(data))
	{
		return false;
	}
	//stringtomaplisttable

	//stringtomaplisttable
	return true;
}

std::string ProtoMessage::toString()
{
	//maplisttabletostring

	//maplisttabletostring
	std::string result;
	if (!m_protoMsg->SerializeToString(&result))
	{
		return "";
	}
	return result;
}

int32_t main()
{
	
	getchar();
	return 0;
}