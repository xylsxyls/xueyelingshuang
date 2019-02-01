#pragma once
#include "ProtoMessageMacro.h"
#include <map>
#include <list>
#include <vector>
#include <stdint.h>
#include "Variant/VariantAPI.h"

#define PROTO_GLOBAL "global"

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
	ProtoMessage& operator[](const char* key);
	ProtoMessage operator=(const Variant& value);
	void push_back(const Variant& value);
	void push_front(const Variant& value);
	void setTable(const std::vector<std::vector<Variant>>& table);
public:
	void setCode(int32_t code, const std::string& errorMsg = "");

	int32_t getCode() const;

	std::string getErrorMessage() const;

	void setKeyValue(const std::string& dataKey, const Variant& dataValue, const std::string& key = PROTO_GLOBAL);

	void addListValue(const Variant& value, const std::string& key = PROTO_GLOBAL);

	void setKeyMap(const std::map<std::string, Variant>& mapData, const std::string& key = PROTO_GLOBAL);

	void setKeyList(const std::list<Variant>& listData, const std::string& key = PROTO_GLOBAL);

	void setKeyTable(const std::vector<std::vector<Variant>>& tableData, const std::string& key = PROTO_GLOBAL);

	std::map<std::string, Variant> getMap(const std::string& key = PROTO_GLOBAL);

	std::list<Variant> getList(const std::string& key = PROTO_GLOBAL);

	std::vector<std::vector<Variant>> getTable(const std::string& key = PROTO_GLOBAL);

	bool getMap(std::map<std::string, Variant>& mapData, const std::string& key = PROTO_GLOBAL);

	bool getList(std::list<Variant>& listData, const std::string& key = PROTO_GLOBAL);

	bool getTable(std::vector<std::vector<Variant>>& tableData, const std::string& key = PROTO_GLOBAL);

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
	std::map<std::string, std::map<std::string, Variant>> m_keyMapData;
	std::map<std::string, std::list<Variant>> m_keyListData;
	std::map<std::string, std::vector<std::vector<Variant>>> m_keyTableData;

	std::string m_key;
	std::string m_dataKey;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};