#include "ProtoMessage.h"
#include "ProtoMessage.pb.h"
#if defined _WIN64 && defined _DEBUG
#pragma comment(lib, "libprotobuf-lite_64d.lib")
#elif defined _WIN64
#pragma comment(lib, "libprotobuf-lite_64.lib")
#elif defined _DEBUG
#pragma comment(lib, "libprotobuf-lite_32d.lib")
#else
#pragma comment(lib, "libprotobuf-lite_32.lib")
#endif

ProtoMessage::ProtoMessage():
m_protoMsg(nullptr),
m_key(-1),
m_dataKey(-1)
{
	m_protoMsg = new pt::ProtoMessage;
}

ProtoMessage::ProtoMessage(const ProtoMessage& other)
{
	m_protoMsg = new pt::ProtoMessage;
	m_protoMsg->CopyFrom(*(other.m_protoMsg));

	m_keyMapData = other.m_keyMapData;
	m_keyListData = other.m_keyListData;
	m_keyTableData = other.m_keyTableData;

	m_key = other.m_key;
	m_dataKey = other.m_dataKey;
}

ProtoMessage::~ProtoMessage()
{
	if (m_protoMsg != nullptr)
	{
		delete m_protoMsg;
		m_protoMsg = nullptr;
	}
}

ProtoMessage& ProtoMessage::operator[](const int32_t& key)
{
	if (m_dataKey == -1)
	{
		m_dataKey = key;
	}
	else
	{
		m_key = m_dataKey;
		m_dataKey = key;
	}
	return *this;
}

ProtoMessage ProtoMessage::operator=(const Variant& value)
{
	if (m_key == -1)
	{
		m_key = PROTO_GLOBAL;
	}
	setKeyValue(m_dataKey, value, m_key);
	m_dataKey = -1;
	m_key = -1;
	return *this;
}

void ProtoMessage::push_back(const Variant& value)
{
	if (m_dataKey == -1)
	{
		m_key = PROTO_GLOBAL;
	}
	else
	{
		m_key = m_dataKey;
	}
	m_keyListData[m_key].push_back(value);
	m_dataKey = -1;
	m_key = -1;
}

void ProtoMessage::push_front(const Variant& value)
{
	if (m_dataKey == -1)
	{
		m_key = PROTO_GLOBAL;
	}
	else
	{
		m_key = m_dataKey;
	}
	m_keyListData[m_key].push_front(value);
	m_dataKey = -1;
	m_key = -1;
}

void ProtoMessage::setTable(const std::vector<std::vector<Variant>>& table)
{
	if (m_dataKey == -1)
	{
		m_key = PROTO_GLOBAL;
	}
	else
	{
		m_key = m_dataKey;
	}
	m_keyTableData[m_key] = table;
	m_dataKey = -1;
	m_key = -1;
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

void ProtoMessage::setKeyValue(const int32_t& dataKey, const Variant& dataValue, const int32_t& key)
{
	m_keyMapData[key][dataKey] = dataValue;
}

void ProtoMessage::addListValue(const Variant& value, const int32_t& key)
{
	m_keyListData[key].push_back(value);
}

void ProtoMessage::setKeyMap(const std::map<int32_t, Variant>& mapData, const int32_t& key)
{
	m_keyMapData[key] = mapData;
}

void ProtoMessage::setKeyList(const std::list<Variant>& listData, const int32_t& key)
{
	m_keyListData[key] = listData;
}

void ProtoMessage::setKeyTable(const std::vector<std::vector<Variant>>& tableData, const int32_t& key)
{
	m_keyTableData[key] = tableData;
}

std::map<int32_t, Variant> ProtoMessage::getMap(const int32_t& key)
{
	std::map<int32_t, Variant> result;
	getMap(result, key);
	return result;
}

std::list<Variant> ProtoMessage::getList(const int32_t& key)
{
	std::list<Variant> result;
	getList(result, key);
	return result;
}

std::vector<std::vector<Variant>> ProtoMessage::getTable(const int32_t& key)
{
	std::vector<std::vector<Variant>> result;
	getTable(result, key);
	return result;
}

bool ProtoMessage::getMap(std::map<int32_t, Variant>& mapData, const int32_t& key)
{
	auto itKeyMapData = m_keyMapData.find(key);
	if (itKeyMapData == m_keyMapData.end())
	{
		return false;
	}
	mapData = itKeyMapData->second;
	return true;
}

bool ProtoMessage::getList(std::list<Variant>& listData, const int32_t& key)
{
	auto itKeyListData = m_keyListData.find(key);
	if (itKeyListData == m_keyListData.end())
	{
		return false;
	}
	listData = itKeyListData->second;
	return true;
}

bool ProtoMessage::getTable(std::vector<std::vector<Variant>>& tableData, const int32_t& key)
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
	if (m_protoMsg == nullptr || !m_protoMsg->ParseFromString(data))
	{
		return false;
	}
	clearMap();
	clearList();
	clearTable();
	
	int32_t index = -1;
	while (index++ != m_protoMsg->keymapdata_size() - 1)
	{
		auto protokeyMapData = m_protoMsg->mutable_keymapdata(index);
		const int32_t& key = protokeyMapData->key();
		auto mapData = protokeyMapData->mutable_mapdata();
		int32_t dataIndex = -1;
		while (dataIndex++ != mapData->mapkeyvalue_size() - 1)
		{
			auto mapKeyValue = mapData->mutable_mapkeyvalue(dataIndex);
			const int32_t& dataKey = mapKeyValue->key();
			auto variantValue = mapKeyValue->mutable_value();
			m_keyMapData[key][dataKey] = Variant(variantValue->data(), (Variant::VariantType)(variantValue->type()), true);
		}
	}

	index = -1;
	while (index++ != m_protoMsg->keylistdata_size() - 1)
	{
		auto protokeyListData = m_protoMsg->mutable_keylistdata(index);
		const int32_t& key = protokeyListData->key();
		auto listData = protokeyListData->mutable_listdata();
		int32_t dataIndex = -1;
		while (dataIndex++ != listData->value_size() - 1)
		{
			auto variantValue = listData->mutable_value(dataIndex);
			m_keyListData[key].push_back(Variant(variantValue->data(), (Variant::VariantType)(variantValue->type()), true));
		}
	}

	index = -1;
	while (index++ != m_protoMsg->keytabledata_size() - 1)
	{
		auto protokeyTableData = m_protoMsg->mutable_keytabledata(index);
		const int32_t& key = protokeyTableData->key();
		auto tableData = protokeyTableData->mutable_tabledata();
		std::vector<std::vector<Variant>> vecData;
		int32_t lineIndex = -1;
		while (lineIndex++ != tableData->listdata_size() - 1)
		{
			std::vector<Variant> vecLine;
			auto list = tableData->mutable_listdata(lineIndex);
			int32_t dataIndex = -1;
			while (dataIndex++ != list->value_size() - 1)
			{
				auto variantValue = list->mutable_value(dataIndex);
				vecLine.push_back(Variant(variantValue->data(), (Variant::VariantType)(variantValue->type()), true));
			}
			vecData.push_back(vecLine);
		}
		m_keyTableData[key] = vecData;
	}

	return true;
}

std::string ProtoMessage::toString()
{
	std::string result;
	toString(result);
	return result;
}

bool ProtoMessage::toString(std::string& result)
{
	if (m_protoMsg == nullptr)
	{
		return false;
	}

	m_protoMsg->Clear();
	for (auto itKeyMapData = m_keyMapData.begin(); itKeyMapData != m_keyMapData.end(); ++itKeyMapData)
	{
		auto& key = itKeyMapData->first;
		auto protoKeyMapData = m_protoMsg->add_keymapdata();
		protoKeyMapData->set_key(key);
		auto protoMapData = protoKeyMapData->mutable_mapdata();
		auto& mapData = itKeyMapData->second;
		for (auto itData = mapData.begin(); itData != mapData.end(); ++itData)
		{
			auto& dataKey = itData->first;
			const Variant& dataValue = itData->second;
			auto mapKeyValue = protoMapData->add_mapkeyvalue();
			mapKeyValue->set_key(dataKey);
			auto variantValue = mapKeyValue->mutable_value();
			std::string blob;
			dataValue.toBlob(blob);
			variantValue->set_data(blob);
			variantValue->set_type((int32_t)dataValue.type());
		}
	}

	for (auto itKeyListData = m_keyListData.begin(); itKeyListData != m_keyListData.end(); ++itKeyListData)
	{
		auto& key = itKeyListData->first;
		auto protokeyListData = m_protoMsg->add_keylistdata();
		protokeyListData->set_key(key);
		auto protoListData = protokeyListData->mutable_listdata();
		auto& listData = itKeyListData->second;
		for (auto itData = listData.begin(); itData != listData.end(); ++itData)
		{
			const Variant& dataValue = *itData;
			auto variantValue = protoListData->add_value();
			std::string blob;
			dataValue.toBlob(blob);
			variantValue->set_data(blob);
			variantValue->set_type((int32_t)dataValue.type());
		}
	}

	for (auto itKeyTableData = m_keyTableData.begin(); itKeyTableData != m_keyTableData.end(); ++itKeyTableData)
	{
		auto& key = itKeyTableData->first;
		auto protoKeyTableData = m_protoMsg->add_keytabledata();
		protoKeyTableData->set_key(key);
		auto protoTableData = protoKeyTableData->mutable_tabledata();
		auto& tableData = itKeyTableData->second;
		int32_t lineIndex = -1;
		while (lineIndex++ != tableData.size() - 1)
		{
			auto list = protoTableData->add_listdata();
			auto& listData = tableData[lineIndex];
			int32_t dataIndex = -1;
			while (dataIndex++ != listData.size() - 1)
			{
				const Variant& dataValue = listData[dataIndex];
				auto variantValue = list->add_value();
				std::string blob;
				dataValue.toBlob(blob);
				variantValue->set_data(blob);
				variantValue->set_type((int32_t)dataValue.type());
			}
		}
	}
	return m_protoMsg->SerializeToString(&result);
}

void ProtoMessage::clear()
{
	if (m_protoMsg == nullptr)
	{
		return;
	}
	m_protoMsg->Clear();
	clearMap();
	clearList();
	clearTable();
	m_key = -1;
	m_dataKey = -1;
}

void ProtoMessage::clearMap()
{
	m_keyMapData.clear();
}

void ProtoMessage::clearList()
{
	m_keyListData.clear();
}

void ProtoMessage::clearTable()
{
	m_keyTableData.clear();
}

//int32_t main()
//{
//	ProtoMessage msg;
//	
//	//msg["1"] = "2";
//	//msg["other"]["2"] = 3;
//	//msg.push_back("123");
//	//msg.push_front("1234");
//	//msg["1"].push_back("123");
//	//msg["1"].push_back("1234");
//	//std::vector<std::vector<Variant>> vecTable;
//	//std::vector<Variant> vecList;
//	//vecList.push_back(12345);
//	//vecList.push_back("123456");
//	//vecTable.push_back(vecList);
//	//vecList[0] = 23456;
//	//vecList[1] = 234567;
//	//vecTable.push_back(vecList);
//	//msg.setKeyTable(vecTable);
//	//msg["table2"].setTable(vecTable);
//	//Cjson json;
//	//int begin = ::GetTickCount();
//	//int count = 3000;
//	//while (count-- != 0)
//	//{
//	//	msg[CStringManager::Format("%d",count).c_str()] = "2";
//	//	//json["global"][CStringManager::Format("%d", count).c_str()] = "2";
//	//	//std::string proto = msg.toString();
//	//	//ProtoMessage msg2;
//	//	//msg2.from(proto);
//	//	//std::string str = msg2.toString();
//	//	//if (str == proto)
//	//	//{
//	//	//	RCSend("same,%d",str.size());
//	//	//}
//	//	//ProtoMessage msg3;
//	//	//msg3.from(proto);
//	//	//std::string str2 = msg3.toString();
//	//	//if (str == str2)
//	//	//{
//	//	//	RCSend("same2");
//	//	//}
//	//	//RCSend("str.size = %d, proto.size = %d,cmp = %d", str.size(), proto.size(), memcmp(&str[0], &proto[0], 117));
//	//}
//	//std::string proto = msg.toString();
//	//std::string strjson = json.tostring("", "");
//	//std::string com = Compress::zlibCompress(proto);
//	//RCSend("time = %d,length = %d", ::GetTickCount() - begin, com.size());
//
//	//count = 100000;
//	//while (count-- != 0)
//	//{
//	//	Cjson json;
//	//	json["1"] = "2";
//	//	json["global"][0] = "123";
//	//	json["global"][1] = "1234";
//	//	json["global2"][0][0] = "12345";
//	//	json["global2"][0][1] = "123456";
//	//	json["global2"][1][0] = "23456";
//	//	json["global2"][1][1] = "234567";
//	//	json["global2"][1][2] = "234567";
//	//	std::string str2 = json.tostring("", "");
//	//	Cjson json2;
//	//	json2.LoadJson(str2);
//	//	std::string str3 = json.tostring("", "");
//	//}
//	//RCSend("time = %d", ::GetTickCount() - begin);
//
//	getchar();
//	return 0;
//}