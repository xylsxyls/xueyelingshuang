#pragma once
#include "ProtoMessageMacro.h"
#include <map>
#include <list>
#include <vector>
#include <stdint.h>
#include "Variant/VariantAPI.h"

#define PROTO_GLOBAL 0

namespace pt
{
	class ProtoMessage;
}

class ProtoMessageAPI ProtoMessage
{
public:
	enum
	{
		PROTO_ERROR = -2
	};

public:
	ProtoMessage();

	ProtoMessage(const ProtoMessage& other);

	~ProtoMessage();

public:
	/** ÓÃ·¨
	msg["global"]["time"] = "12:05";
	msg["date"] = "180810";
	msg["global"].push_back(12345);
	msg["global2"].push_back("12345");
	*/
	ProtoMessage& operator[](const int32_t& key);
	ProtoMessage operator=(const Variant& value);
	void push_back(const Variant& value);
	void push_front(const Variant& value);
	void setTable(const std::vector<std::vector<Variant>>& table);
public:
	void setCode(int32_t code, const std::string& errorMsg = "");

	int32_t getCode() const;

	std::string getErrorMessage() const;

	void setKeyValue(const int32_t& dataKey, const Variant& dataValue, const int32_t& key = PROTO_GLOBAL);

	void addListValue(const Variant& value, const int32_t& key = PROTO_GLOBAL);

	void setKeyMap(const std::map<int32_t, Variant>& mapData, const int32_t& key = PROTO_GLOBAL);

	void setKeyList(const std::list<Variant>& listData, const int32_t& key = PROTO_GLOBAL);

	void setKeyTable(const std::vector<std::vector<Variant>>& tableData, const int32_t& key = PROTO_GLOBAL);

	std::map<int32_t, Variant> getMap(const int32_t& key = PROTO_GLOBAL);

	std::list<Variant> getList(const int32_t& key = PROTO_GLOBAL);

	std::vector<std::vector<Variant>> getTable(const int32_t& key = PROTO_GLOBAL);

	bool getMap(std::map<int32_t, Variant>& mapData, const int32_t& key = PROTO_GLOBAL);

	bool getList(std::list<Variant>& listData, const int32_t& key = PROTO_GLOBAL);

	bool getTable(std::vector<std::vector<Variant>>& tableData, const int32_t& key = PROTO_GLOBAL);

	bool from(const std::string& data);

	std::string toString();

	void clear();

	void clearMap();
	void clearList();
	void clearTable();

protected:
	pt::ProtoMessage* m_protoMsg;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::map<int32_t, Variant>> m_keyMapData;
	std::map<int32_t, std::list<Variant>> m_keyListData;
	std::map<int32_t, std::vector<std::vector<Variant>>> m_keyTableData;

	int32_t m_key;
	int32_t m_dataKey;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};