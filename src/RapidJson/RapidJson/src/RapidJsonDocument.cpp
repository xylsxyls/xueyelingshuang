#include "RapidJsonDocument.h"

#include "RapidJsonHelper.h"
#include "RapidJsonValue.h"
#include "rapid_json/document.h"
#include <cstring>
#include <memory>

RapidJsonDocument::RapidJsonDocument() :
m_document(nullptr)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	document->SetObject();
	m_document = document.release();
}

RapidJsonDocument::RapidJsonDocument(const RapidJsonDocument& document) :
m_document(nullptr)
{
	std::unique_ptr<rapidjson::Document> newDocument(new rapidjson::Document());
	newDocument->CopyFrom(*document.m_document, newDocument->GetAllocator());
	m_document = newDocument.release();
}

RapidJsonDocument::RapidJsonDocument(RapidJsonDocument&& document) :
m_document(nullptr)
{
	std::unique_ptr<rapidjson::Document> emptyDocument(new rapidjson::Document());
	emptyDocument->SetObject();
	m_document = document.m_document;
	document.m_document = emptyDocument.release();
}

RapidJsonDocument::~RapidJsonDocument()
{
	delete m_document;
}

RapidJsonDocument& RapidJsonDocument::operator=(const RapidJsonDocument& document)
{
	if (this != &document)
	{
		m_document->CopyFrom(*document.m_document, m_document->GetAllocator());
	}
	return *this;
}

RapidJsonDocument& RapidJsonDocument::operator=(RapidJsonDocument&& document)
{
	if (this != &document)
	{
		std::unique_ptr<rapidjson::Document> emptyDocument(new rapidjson::Document());
		emptyDocument->SetObject();
		delete m_document;
		m_document = document.m_document;
		document.m_document = emptyDocument.release();
	}
	return *this;
}

bool RapidJsonDocument::parse(const std::string& json)
{
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
	return true;
}

void RapidJsonDocument::setObject()
{
	m_document->SetObject();
}

void RapidJsonDocument::setArray()
{
	m_document->SetArray();
}

bool RapidJsonDocument::isObject() const
{
	return m_document->IsObject();
}

bool RapidJsonDocument::isArray() const
{
	return m_document->IsArray();
}

void RapidJsonDocument::addString(const char* key, const std::string& value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.SetString(value.c_str(), RapidJsonHelper::jsonSize(value.size()), allocator);
		return;
	}
	rapidjson::Value jsonKey;
	rapidjson::Value jsonValue;
	jsonKey.SetString(key, allocator);
	jsonValue.SetString(value.c_str(), RapidJsonHelper::jsonSize(value.size()), allocator);
	m_document->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonDocument::addString(const char* key, const char* value)
{
	if (key == nullptr)
	{
		return;
	}
	const char* realValue = value == nullptr ? "" : value;
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::SizeType valueSize = RapidJsonHelper::jsonSize(std::strlen(realValue));
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.SetString(realValue, valueSize, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	rapidjson::Value jsonValue;
	jsonKey.SetString(key, allocator);
	jsonValue.SetString(realValue, valueSize, allocator);
	m_document->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonDocument::addInt(const char* key, int32_t value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.SetInt(value);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, value, allocator);
}

void RapidJsonDocument::addBool(const char* key, bool value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.SetBool(value);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, value, allocator);
}

void RapidJsonDocument::addValue(const char* key, const RapidJsonValue& value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value jsonValue;
	value.copyToRapidJsonValue(&jsonValue, m_document);
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(jsonValue, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonDocument::addStringArray(const char* key, const std::vector<std::string>& values)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_document->IsObject())
	{
		m_document->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_document->MemberCount() == 0)
	{
		m_document->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value array(rapidjson::kArrayType);
	array.Reserve(RapidJsonHelper::jsonSize(values.size()), allocator);
	for (size_t i = 0; i < values.size(); ++i)
	{
		rapidjson::Value jsonValue;
		jsonValue.SetString(values[i].c_str(), RapidJsonHelper::jsonSize(values[i].size()), allocator);
		array.PushBack(jsonValue, allocator);
	}
	rapidjson::Value::MemberIterator it = m_document->FindMember(key);
	if (it != m_document->MemberEnd())
	{
		it->value.CopyFrom(array, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, array, allocator);
}

std::string RapidJsonDocument::getStringOrDefault(const char* key, const std::string& defaultValue) const
{
	if (key == nullptr)
	{
		return defaultValue;
	}
	if (!m_document->IsObject())
	{
		return defaultValue;
	}
	rapidjson::Value::ConstMemberIterator it = m_document->FindMember(key);
	if (it == m_document->MemberEnd() || !it->value.IsString())
	{
		return defaultValue;
	}
	return std::string(it->value.GetString(), it->value.GetStringLength());
}

int32_t RapidJsonDocument::getIntOrDefault(const char* key, int32_t defaultValue) const
{
	if (key == nullptr)
	{
		return defaultValue;
	}
	if (!m_document->IsObject())
	{
		return defaultValue;
	}
	rapidjson::Value::ConstMemberIterator it = m_document->FindMember(key);
	if (it == m_document->MemberEnd() || !it->value.IsInt())
	{
		return defaultValue;
	}
	return it->value.GetInt();
}

bool RapidJsonDocument::getBoolOrDefault(const char* key, bool defaultValue) const
{
	if (key == nullptr)
	{
		return defaultValue;
	}
	if (!m_document->IsObject())
	{
		return defaultValue;
	}
	rapidjson::Value::ConstMemberIterator it = m_document->FindMember(key);
	if (it == m_document->MemberEnd() || !it->value.IsBool())
	{
		return defaultValue;
	}
	return it->value.GetBool();
}

std::vector<std::string> RapidJsonDocument::getStringArrayOrEmpty(const char* key) const
{
	std::vector<std::string> values;
	if (key == nullptr)
	{
		return values;
	}
	if (!m_document->IsObject())
	{
		return values;
	}
	rapidjson::Value::ConstMemberIterator it = m_document->FindMember(key);
	if (it == m_document->MemberEnd() || !it->value.IsArray())
	{
		return values;
	}
	for (rapidjson::SizeType i = 0; i < it->value.Size(); ++i)
	{
		if (it->value[i].IsString())
		{
			values.push_back(std::string(it->value[i].GetString(), it->value[i].GetStringLength()));
		}
	}
	return values;
}

std::string RapidJsonDocument::toString() const
{
	return RapidJsonHelper::valueToString(m_document);
}