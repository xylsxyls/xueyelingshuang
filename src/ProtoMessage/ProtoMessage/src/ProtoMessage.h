#pragma once
#include "ProtoMessageMacro.h"
#include <map>
#include <list>
#include <vector>
#include <stdint.h>

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

public:
	void setCode(int32_t code, const std::string& errorMsg = "");

	int32_t getCode() const;

	std::string getErrorMessage() const;

	void setKeyMap(const std::map<std::string, std::string>& mapData, const std::string& key = "global");

	void setKeyList(const std::list<std::string>& listData, const std::string& key = "global");

	void setKeyTable(const std::vector<std::vector<std::string>>& tableData, const std::string& key = "global");

	std::map<std::string, std::string> getMap(const std::string& key = "global");

	std::list<std::string> getList(const std::string& key = "global");

	std::vector<std::vector<std::string>> getTable(const std::string& key = "global");

	bool getMap(std::map<std::string, std::string>& mapData, const std::string& key = "global");

	bool getList(std::list<std::string>& listData, const std::string& key = "global");

	bool getTable(std::vector<std::vector<std::string>>& tableData, const std::string& key = "global");

	bool from(const std::string& data);

	std::string toString();

protected:
	pt::ProtoMessage* m_protoMsg;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::map<std::string, std::string>> m_keyMapData;
	std::map<std::string, std::list<std::string>> m_keyListData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_keyTableData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};