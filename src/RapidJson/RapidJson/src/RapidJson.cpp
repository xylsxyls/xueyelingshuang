#include "RapidJson.h"
#include "RapidJsonHelper.h"
#include "rapid_json/document.h"
#include <cstring>
#include <memory>

RapidJson::RapidJson() :
m_document(nullptr),
m_value(nullptr)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> value(new rapidjson::Value());
	document->SetObject();
	m_document = document.release();
	m_value = value.release();
}

RapidJson::RapidJson(const RapidJson& rapidJson) :
m_document(nullptr),
m_value(nullptr),
m_key(rapidJson.m_key)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> value(new rapidjson::Value());
	document->CopyFrom(*rapidJson.m_document, document->GetAllocator());
	value->CopyFrom(*rapidJson.m_value, document->GetAllocator());
	m_document = document.release();
	m_value = value.release();
}

RapidJson::RapidJson(const rapidjson::Value* rapidjson) :
m_document(nullptr),
m_value(nullptr)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> value(new rapidjson::Value());
	if (rapidjson == nullptr)
	{
		document->SetNull();
		value->SetNull();
		m_document = document.release();
		m_value = value.release();
		return;
	}
	value->CopyFrom(*rapidjson, document->GetAllocator());
	document->CopyFrom(*rapidjson, document->GetAllocator());
	m_document = document.release();
	m_value = value.release();
}

RapidJson::~RapidJson()
{
	delete m_value;
	delete m_document;
}

bool RapidJson::parse(const std::string& json)
{
	m_key.clear();
	m_value->SetNull();
	if (json.empty())
	{
		m_document->SetObject();
		return false;
	}
	if (json.find('\0') != std::string::npos)
	{
		m_document->SetObject();
		return false;
	}
	m_document->Parse(json.c_str(), json.size());
	if (m_document->HasParseError())
	{
		m_document->SetObject();
		return false;
	}
	m_value->CopyFrom(*m_document, m_document->GetAllocator());
	return true;
}

bool RapidJson::hasMember(const std::string& key)
{
	if (!m_document->IsObject())
	{
		return false;
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(key.c_str(), RapidJsonHelper::jsonSize(key.size()), m_document->GetAllocator());
	return m_document->FindMember(strKey) != m_document->MemberEnd();
}

std::string RapidJson::toString()
{
	return RapidJsonHelper::valueToString(m_document);
}

bool RapidJson::isNull() const
{
	return m_value->IsNull();
}

bool RapidJson::isInt() const
{
	return m_value->IsInt();
}

int32_t RapidJson::toIntOrDefault(int32_t defaultValue) const
{
	if (!m_value->IsInt())
	{
		return defaultValue;
	}
	return m_value->GetInt();
}

std::string RapidJson::toStringOrDefault(const std::string& defaultValue) const
{
	if (!m_value->IsString())
	{
		return defaultValue;
	}
	return std::string(m_value->GetString(), m_value->GetStringLength());
}

const rapidjson::Value* RapidJson::toRapidJsonValue() const
{
	return m_value;
}

std::vector<RapidJson> RapidJson::toArray()
{
	std::vector<RapidJson> result;
	if (!m_value->IsArray())
	{
		return result;
	}
	const auto& rapidjsonArray = m_value->GetArray();
	for (rapidjson::SizeType index = 0; index < rapidjsonArray.Size(); ++index)
	{
		result.push_back(RapidJson(&rapidjsonArray[index]));
	}
	return result;
}

RapidJson& RapidJson::operator=(const rapidjson::Value* value)
{
	rapidjson::Value rapidjsonValue;
	if (value != nullptr)
	{
		rapidjsonValue.CopyFrom(*value, m_document->GetAllocator());
	}
	if (m_key.empty())
	{
		m_value->CopyFrom(rapidjsonValue, m_document->GetAllocator());
		m_document->CopyFrom(rapidjsonValue, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(rapidjsonValue, m_document->GetAllocator());
	}
	else
	{
		m_document->AddMember(strKey, rapidjsonValue, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator=(int32_t value)
{
	if (m_key.empty())
	{
		m_document->SetInt(value);
		m_value->CopyFrom(*m_document, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.SetInt(value);
	}
	else
	{
		m_document->AddMember(strKey, value, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator=(const std::string& value)
{
	rapidjson::Value strValue(rapidjson::kStringType);
	strValue.SetString(value.c_str(), RapidJsonHelper::jsonSize(value.size()), m_document->GetAllocator());
	if (m_key.empty())
	{
		m_value->CopyFrom(strValue, m_document->GetAllocator());
		m_document->CopyFrom(strValue, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(strValue, m_document->GetAllocator());
	}
	else
	{
		m_document->AddMember(strKey, strValue, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator=(const char* value)
{
	const char* realValue = value == nullptr ? "" : value;
	rapidjson::SizeType valueSize = RapidJsonHelper::jsonSize(std::strlen(realValue));
	rapidjson::Value strValue(rapidjson::kStringType);
	strValue.SetString(realValue, valueSize, m_document->GetAllocator());
	if (m_key.empty())
	{
		m_value->CopyFrom(strValue, m_document->GetAllocator());
		m_document->CopyFrom(strValue, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(strValue, m_document->GetAllocator());
	}
	else
	{
		m_document->AddMember(strKey, strValue, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator=(const std::vector<RapidJson>& value)
{
	rapidjson::Value rapidjsonValue(rapidjson::Type::kArrayType);
	for (size_t index = 0; index < value.size(); ++index)
	{
		rapidjson::Value item;
		item.CopyFrom(*value[index].m_document, m_document->GetAllocator());
		rapidjsonValue.PushBack(item, m_document->GetAllocator());
	}
	if (m_key.empty())
	{
		m_document->CopyFrom(rapidjsonValue, m_document->GetAllocator());
		m_value->CopyFrom(*m_document, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(rapidjsonValue, m_document->GetAllocator());
	}
	else
	{
		m_document->AddMember(strKey, rapidjsonValue, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator=(const RapidJson& value)
{
	if (this == &value && m_key.empty())
	{
		return *this;
	}
	rapidjson::Value rapidjsonValue(rapidjson::Type::kObjectType);
	rapidjsonValue.CopyFrom(*value.m_document, m_document->GetAllocator());
	if (m_key.empty())
	{
		m_document->CopyFrom(rapidjsonValue, m_document->GetAllocator());
		m_value->CopyFrom(*m_document, m_document->GetAllocator());
		return *this;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), m_document->GetAllocator());
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
	rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(rapidjsonValue, m_document->GetAllocator());
	}
	else
	{
		m_document->AddMember(strKey, rapidjsonValue, m_document->GetAllocator());
	}
	m_key.clear();
	return *this;
}

RapidJson& RapidJson::operator[](const std::string& key)
{
	m_key = key;
	if (m_document->IsObject())
	{
		rapidjson::Value strKey(rapidjson::kStringType);
		strKey.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), m_document->GetAllocator());
		rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
		if (it != m_document->MemberEnd())
		{
			m_value->CopyFrom(it->value, m_document->GetAllocator());
			return *this;
		}
	}
	m_value->SetNull();
	return *this;
}

//#include "RapidJsonDocument.h"
//#include "RapidJsonValue.h"
//#include <atomic>
//#include <functional>
//#include <iostream>
//#include <sstream>
//#include <thread>
//#include <utility>
//#include <vector>
//
//int main()
//{
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//
//	RapidJsonDocument document;
//	check(document.isObject(), "RapidJsonDocument default object");
//	check(!document.parse(""), "RapidJsonDocument empty parse failed");
//	check(!document.parse("{bad json"), "RapidJsonDocument invalid parse failed");
//	check(document.parse("{\"name\":\"cook\",\"count\":3,\"ok\":true,\"tags\":[\"a\",\"b\"]}"), "RapidJsonDocument parse object");
//	check(document.getStringOrDefault("name", "") == "cook", "RapidJsonDocument get string");
//	check(document.getIntOrDefault("count", 0) == 3, "RapidJsonDocument get int");
//	check(document.getBoolOrDefault("ok", false), "RapidJsonDocument get bool");
//	check(document.getStringArrayOrEmpty("tags").size() == 2, "RapidJsonDocument get string array");
//	document.addString("name", "chef");
//	document.addInt("count", 5);
//	document.addBool("ok", false);
//	document.addStringArray("tags", std::vector<std::string>{"x", "y", "z"});
//	document.addString("literal", "char value");
//	document.addString("emptyChar", static_cast<const char*>(nullptr));
//	RapidJsonDocument verify;
//	check(verify.parse(document.toString()), "RapidJsonDocument serialize parse");
//	check(verify.getStringOrDefault("name", "") == "chef", "RapidJsonDocument replace string");
//	check(verify.getIntOrDefault("count", 0) == 5, "RapidJsonDocument replace int");
//	check(!verify.getBoolOrDefault("ok", true), "RapidJsonDocument replace bool");
//	check(verify.getStringArrayOrEmpty("tags").size() == 3, "RapidJsonDocument replace array");
//	check(verify.getStringOrDefault("literal", "") == "char value", "RapidJsonDocument add char string");
//	check(verify.getStringOrDefault("emptyChar", "default") == "", "RapidJsonDocument add null char string");
//	check(!verify.parse(""), "RapidJsonDocument empty parse clears old data");
//	check(verify.getStringOrDefault("name", "default") == "default", "RapidJsonDocument empty parse no stale string");
//	check(!verify.parse("{bad json"), "RapidJsonDocument invalid parse clears old data");
//	check(verify.getIntOrDefault("count", 99) == 99, "RapidJsonDocument invalid parse no stale int");
//	check(verify.parse("[1,2,3]") && verify.isArray(), "RapidJsonDocument parse root array");
//	check(verify.getBoolOrDefault("ok", true), "RapidJsonDocument root array getter default");
//	std::string dirtyJson("{\"name\":\"ok\"}");
//	dirtyJson.push_back('\0');
//	dirtyJson += "broken";
//	check(!verify.parse(dirtyJson), "RapidJsonDocument dirty text after zero fails");
//	check(verify.getStringOrDefault("name", "default") == "default", "RapidJsonDocument dirty text clears old data");
//
//	RapidJsonDocument nullKeyDocument;
//	nullKeyDocument.addString(nullptr, "bad");
//	nullKeyDocument.addInt(nullptr, 3);
//	nullKeyDocument.addBool(nullptr, true);
//	check(nullKeyDocument.toString() == "{}", "RapidJsonDocument null key ignored");
//
//	std::string binaryText("ab", 2);
//	binaryText.push_back('\0');
//	binaryText += "cd";
//	RapidJsonDocument binaryDocument;
//	binaryDocument.addString("binary", binaryText);
//	RapidJsonDocument binaryParsed;
//	check(binaryParsed.parse(binaryDocument.toString()), "RapidJsonDocument embedded zero parse");
//	check(binaryParsed.getStringOrDefault("binary", "") == binaryText, "RapidJsonDocument embedded zero keep length");
//
//	RapidJsonValue child;
//	child.setObject();
//	child.addString("childName", "small");
//	child.addInt("childCount", 2);
//	child.addBool("childOk", true);
//	child.addStringArray("childTags", std::vector<std::string>{"m", "n"});
//	RapidJsonDocument parent;
//	parent.addValue("child", child);
//	check(parent.toString().find("childName") != std::string::npos, "RapidJsonDocument add value");
//
//	RapidJsonValue arrayValue;
//	arrayValue.setArray();
//	arrayValue.reserve(4);
//	arrayValue.pushString("one");
//	arrayValue.pushInt(2);
//	arrayValue.pushBool(true);
//	arrayValue.pushValue(child);
//	check(arrayValue.isArray() && arrayValue.size() == 4, "RapidJsonValue array push");
//	check(arrayValue.toString().find("childName") != std::string::npos, "RapidJsonValue nested serialize");
//
//	RapidJsonValue nullKeyValue;
//	nullKeyValue.addString(nullptr, "bad");
//	nullKeyValue.addInt(nullptr, 1);
//	nullKeyValue.addBool(nullptr, true);
//	check(nullKeyValue.toString() == "{}", "RapidJsonValue null key ignored");
//	RapidJsonValue scalarValue;
//	scalarValue.setString(binaryText);
//	check(scalarValue.toString().find("\\u0000") != std::string::npos, "RapidJsonValue string embedded zero serialize");
//	scalarValue.setString("char scalar");
//	check(scalarValue.toString() == "\"char scalar\"", "RapidJsonValue set char string root");
//	scalarValue.setInt(42);
//	check(scalarValue.toString() == "42", "RapidJsonValue set int root");
//	scalarValue.setBool(true);
//	check(scalarValue.toString() == "true", "RapidJsonValue set bool root");
//	scalarValue.addString("autoObject", "yes");
//	scalarValue.addString("emptyChar", static_cast<const char*>(nullptr));
//	check(scalarValue.isObject() && scalarValue.toString().find("autoObject") != std::string::npos, "RapidJsonValue add converts to object");
//	check(scalarValue.toString().find("\"emptyChar\":\"\"") != std::string::npos, "RapidJsonValue add null char string");
//	scalarValue.setString("not array");
//	scalarValue.pushString("char item");
//	scalarValue.pushInt(8);
//	check(scalarValue.isArray() && scalarValue.size() == 2, "RapidJsonValue push converts to array");
//
//	RapidJsonValue copyValue = child;
//	check(copyValue.isObject() && copyValue.size() == child.size(), "RapidJsonValue copy construct");
//	copyValue = copyValue;
//	check(copyValue.isObject() && copyValue.size() == child.size(), "RapidJsonValue self assign");
//	RapidJsonValue assignedValue;
//	assignedValue = child;
//	check(assignedValue.isObject() && assignedValue.size() == child.size(), "RapidJsonValue copy assign");
//	RapidJsonValue moveValueSource;
//	moveValueSource.addString("name", "move");
//	RapidJsonValue moveValueTarget(std::move(moveValueSource));
//	check(moveValueTarget.toString().find("\"name\":\"move\"") != std::string::npos, "RapidJsonValue move construct");
//	moveValueSource.addString("afterMove", "source");
//	check(moveValueSource.toString().find("afterMove") != std::string::npos, "RapidJsonValue moved source usable");
//	RapidJsonValue moveValueAssign;
//	moveValueAssign = std::move(moveValueTarget);
//	check(moveValueAssign.toString().find("\"name\":\"move\"") != std::string::npos, "RapidJsonValue move assign");
//	moveValueTarget.addString("afterAssign", "target");
//	check(moveValueTarget.toString().find("afterAssign") != std::string::npos, "RapidJsonValue move assigned source usable");
//	RapidJsonDocument copyDocument = parent;
//	check(copyDocument.toString() == parent.toString(), "RapidJsonDocument copy construct");
//	copyDocument = copyDocument;
//	check(copyDocument.toString() == parent.toString(), "RapidJsonDocument self assign");
//	RapidJsonDocument moveSource;
//	moveSource.addString("name", "move");
//	RapidJsonDocument moveTarget(std::move(moveSource));
//	check(moveTarget.getStringOrDefault("name", "") == "move", "RapidJsonDocument move construct");
//	moveSource.addString("afterMove", "source");
//	check(moveSource.getStringOrDefault("afterMove", "") == "source", "RapidJsonDocument moved source usable");
//	RapidJsonDocument moveAssign;
//	moveAssign = std::move(moveTarget);
//	check(moveAssign.getStringOrDefault("name", "") == "move", "RapidJsonDocument move assign");
//	moveTarget.addString("afterAssign", "target");
//	check(moveTarget.getStringOrDefault("afterAssign", "") == "target", "RapidJsonDocument move assigned source usable");
//
//	RapidJson legacy;
//	legacy["id"] = 7;
//	legacy["title"] = "legacy";
//	RapidJsonDocument legacyDocument;
//	check(legacyDocument.parse(legacy.toString()), "RapidJson legacy serialize parse");
//	check(legacyDocument.getIntOrDefault("id", 0) == 7, "RapidJson legacy get int");
//	check(legacyDocument.getStringOrDefault("title", "") == "legacy", "RapidJson legacy get string");
//	check(legacy["missing"].isNull(), "RapidJson legacy missing key null value");
//	check(legacy["id"].isInt() && legacy.toIntOrDefault(-1) == 7, "RapidJson legacy key cache update");
//	check(legacy.parse("[{\"v\":1},{\"v\":2}]"), "RapidJson legacy parse root array");
//	check(!legacy.hasMember("v"), "RapidJson legacy hasMember root array false");
//	check(legacy.toArray().size() == 2, "RapidJson legacy root array toArray");
//	check(!legacy.parse(dirtyJson), "RapidJson legacy dirty text after zero fails");
//	check(!legacy.hasMember("name"), "RapidJson legacy dirty text clears old data");
//	RapidJson legacyRoot;
//	legacyRoot = 9;
//	check(legacyRoot.toString() == "9", "RapidJson legacy root int assign");
//	legacyRoot = std::string("root");
//	check(legacyRoot.toString() == "\"root\"", "RapidJson legacy root string assign");
//	RapidJson legacyItemA;
//	legacyItemA["v"] = 1;
//	RapidJson legacyItemB;
//	legacyItemB["v"] = 2;
//	std::vector<RapidJson> legacyArray;
//	legacyArray.push_back(legacyItemA);
//	legacyArray.push_back(legacyItemB);
//	RapidJson legacyArrayRoot;
//	legacyArrayRoot = legacyArray;
//	check(legacyArrayRoot.toArray().size() == 2, "RapidJson legacy root array assign");
//	RapidJson legacyKeyClear;
//	legacyKeyClear["items"] = legacyArray;
//	legacyKeyClear["title"] = std::string("after array");
//	RapidJsonDocument legacyKeyClearDocument;
//	check(legacyKeyClearDocument.parse(legacyKeyClear.toString()), "RapidJson legacy key clear parse");
//	check(legacyKeyClearDocument.getStringOrDefault("title", "") == "after array", "RapidJson legacy key clear after array assign");
//	RapidJson legacyObjectRoot;
//	legacyObjectRoot = legacyKeyClear;
//	check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJson legacy root object assign");
//	legacyObjectRoot = legacyObjectRoot;
//	check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJson legacy self assign");
//	legacyObjectRoot = legacyObjectRoot.toRapidJsonValue();
//	check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJson legacy self document assign");
//	legacyObjectRoot = legacyObjectRoot.toRapidJsonValue();
//	check(legacyObjectRoot.toString().find("after array") != std::string::npos, "RapidJson legacy self value assign");
//
//	RapidJson legacyNullConstruct(static_cast<const rapidjson::Value*>(nullptr));
//	check(legacyNullConstruct.isNull() && legacyNullConstruct.toString() == "null", "RapidJson legacy null pointer construct");
//	RapidJson legacyNullRoot;
//	legacyNullRoot = static_cast<const rapidjson::Value*>(nullptr);
//	check(legacyNullRoot.isNull() && legacyNullRoot.toString() == "null", "RapidJson legacy null pointer root assign");
//	RapidJson legacyNullMember;
//	legacyNullMember["empty"] = static_cast<const rapidjson::Value*>(nullptr);
//	legacyNullMember["after"] = 12;
//	RapidJsonDocument legacyNullMemberDocument;
//	check(legacyNullMemberDocument.parse(legacyNullMember.toString()), "RapidJson legacy null pointer member parse");
//	check(legacyNullMemberDocument.getIntOrDefault("after", -1) == 12, "RapidJson legacy null pointer member clears key");
//	check(legacyNullMember.toString().find("\"empty\":null") != std::string::npos, "RapidJson legacy null pointer member kept");
//
//	int32_t lifecycleFailCount = 0;
//	for (int32_t loopIndex = 0; loopIndex < 1000; ++loopIndex)
//	{
//		RapidJson loopJson;
//		loopJson["index"] = loopIndex;
//		loopJson["name"] = std::string("loop");
//		RapidJson loopJsonCopy(loopJson);
//		RapidJson loopJsonAssign;
//		loopJsonAssign = loopJsonCopy;
//		RapidJsonDocument loopDocument;
//		loopDocument.parse(loopJsonAssign.toString());
//		RapidJsonDocument loopDocumentCopy(loopDocument);
//		RapidJsonDocument loopDocumentMove(std::move(loopDocumentCopy));
//		RapidJsonDocument loopDocumentAssign;
//		loopDocumentAssign = std::move(loopDocumentMove);
//		RapidJsonValue loopValue;
//		loopValue.addInt("index", loopIndex);
//		loopValue.addString("name", "loop");
//		RapidJsonValue loopValueCopy(loopValue);
//		RapidJsonValue loopValueMove(std::move(loopValueCopy));
//		RapidJsonValue loopValueAssign;
//		loopValueAssign = std::move(loopValueMove);
//		RapidJsonDocument loopValueDocument;
//		loopValueDocument.addValue("value", loopValueAssign);
//		if (loopDocumentAssign.getIntOrDefault("index", -1) != loopIndex ||
//			loopJsonAssign.toString().find("\"name\":\"loop\"") == std::string::npos ||
//			loopValueDocument.toString().find("\"index\"") == std::string::npos)
//		{
//			++lifecycleFailCount;
//		}
//	}
//	check(lifecycleFailCount == 0, "RapidJson pointer lifecycle pressure");
//
//	std::atomic<int32_t> threadFailCount(0);
//	std::vector<std::thread> threads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		threads.push_back(std::thread([threadIndex, &threadFailCount]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
//			{
//				std::ostringstream oss;
//				oss << "worker_" << threadIndex << "_" << loopIndex;
//				RapidJsonDocument threadDocument;
//				threadDocument.addString("name", oss.str());
//				threadDocument.addInt("index", loopIndex);
//				threadDocument.addBool("ok", true);
//				RapidJsonDocument parsed;
//				if (!parsed.parse(threadDocument.toString()) ||
//					parsed.getStringOrDefault("name", "") != oss.str() ||
//					parsed.getIntOrDefault("index", -1) != loopIndex ||
//					!parsed.getBoolOrDefault("ok", false))
//				{
//					threadFailCount.fetch_add(1);
//				}
//			}
//		}));
//	}
//	for (size_t i = 0; i < threads.size(); ++i)
//	{
//		threads[i].join();
//	}
//	check(threadFailCount.load() == 0, "RapidJson multithread pressure");
//
//	RapidJsonDocument sharedDocument;
//	sharedDocument.addString("name", "shared");
//	sharedDocument.addInt("count", 11);
//	sharedDocument.addBool("ok", true);
//	const std::string sharedJson = sharedDocument.toString();
//	std::atomic<int32_t> readFailCount(0);
//	std::vector<std::thread> readThreads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		readThreads.push_back(std::thread([&sharedDocument, &sharedJson, &readFailCount]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
//			{
//				if (sharedDocument.toString() != sharedJson ||
//					sharedDocument.getStringOrDefault("name", "") != "shared" ||
//					sharedDocument.getIntOrDefault("count", -1) != 11 ||
//					!sharedDocument.getBoolOrDefault("ok", false))
//				{
//					readFailCount.fetch_add(1);
//				}
//			}
//		}));
//	}
//	for (size_t i = 0; i < readThreads.size(); ++i)
//	{
//		readThreads[i].join();
//	}
//	check(readFailCount.load() == 0, "RapidJson shared read multithread pressure");
//
//	std::cout << "RapidJson test " << (failCount == 0 ? "PASS" : "FAIL") <<
//		", total=" << totalCount << ", failed=" << failCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}
//