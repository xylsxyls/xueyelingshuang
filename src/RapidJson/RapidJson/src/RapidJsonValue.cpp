#include "RapidJsonValue.h"

#include "RapidJsonHelper.h"
#include "rapid_json/document.h"
#include <cstring>
#include <memory>

RapidJsonValue::RapidJsonValue() :
m_document(nullptr),
m_value(nullptr)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> value(new rapidjson::Value(rapidjson::kObjectType));
	document->SetObject();
	m_document = document.release();
	m_value = value.release();
}

RapidJsonValue::RapidJsonValue(const RapidJsonValue& value) :
m_document(nullptr),
m_value(nullptr)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> jsonValue(new rapidjson::Value());
	document->SetObject();
	jsonValue->CopyFrom(*value.m_value, document->GetAllocator());
	m_document = document.release();
	m_value = jsonValue.release();
}

RapidJsonValue::RapidJsonValue(RapidJsonValue&& value) :
m_document(nullptr),
m_value(nullptr)
{
	std::unique_ptr<rapidjson::Document> emptyDocument(new rapidjson::Document());
	std::unique_ptr<rapidjson::Value> emptyValue(new rapidjson::Value(rapidjson::kObjectType));
	emptyDocument->SetObject();
	m_document = value.m_document;
	m_value = value.m_value;
	value.m_document = emptyDocument.release();
	value.m_value = emptyValue.release();
}

RapidJsonValue::~RapidJsonValue()
{
	delete m_value;
	delete m_document;
}

RapidJsonValue& RapidJsonValue::operator=(const RapidJsonValue& value)
{
	if (this != &value)
	{
		m_value->CopyFrom(*value.m_value, m_document->GetAllocator());
	}
	return *this;
}

RapidJsonValue& RapidJsonValue::operator=(RapidJsonValue&& value)
{
	if (this != &value)
	{
		std::unique_ptr<rapidjson::Document> emptyDocument(new rapidjson::Document());
		std::unique_ptr<rapidjson::Value> emptyValue(new rapidjson::Value(rapidjson::kObjectType));
		emptyDocument->SetObject();
		delete m_value;
		delete m_document;
		m_document = value.m_document;
		m_value = value.m_value;
		value.m_document = emptyDocument.release();
		value.m_value = emptyValue.release();
	}
	return *this;
}

void RapidJsonValue::setObject()
{
	m_value->SetObject();
}

void RapidJsonValue::setArray()
{
	m_value->SetArray();
}

void RapidJsonValue::setString(const std::string& value)
{
	RapidJsonHelper::setString(m_value, value, m_document);
}

void RapidJsonValue::setString(const char* value)
{
	const char* realValue = value == nullptr ? "" : value;
	m_value->SetString(realValue, RapidJsonHelper::jsonSize(std::strlen(realValue)), m_document->GetAllocator());
}

void RapidJsonValue::setInt(int32_t value)
{
	m_value->SetInt(value);
}

void RapidJsonValue::setBool(bool value)
{
	m_value->SetBool(value);
}

bool RapidJsonValue::isObject() const
{
	return m_value->IsObject();
}

bool RapidJsonValue::isArray() const
{
	return m_value->IsArray();
}

size_t RapidJsonValue::size() const
{
	if (m_value->IsArray())
	{
		return static_cast<size_t>(m_value->Size());
	}
	if (m_value->IsObject())
	{
		return static_cast<size_t>(m_value->MemberCount());
	}
	return 0;
}

void RapidJsonValue::reserve(size_t capacity)
{
	if (m_value->IsArray())
	{
		m_value->Reserve(RapidJsonHelper::jsonSize(capacity), m_document->GetAllocator());
	}
}

void RapidJsonValue::addString(const char* key, const std::string& value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.SetString(value.c_str(), RapidJsonHelper::jsonSize(value.size()), allocator);
		return;
	}
	rapidjson::Value jsonKey;
	rapidjson::Value jsonValue;
	jsonKey.SetString(key, allocator);
	jsonValue.SetString(value.c_str(), RapidJsonHelper::jsonSize(value.size()), allocator);
	m_value->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonValue::addString(const char* key, const char* value)
{
	if (key == nullptr)
	{
		return;
	}
	const char* realValue = value == nullptr ? "" : value;
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::SizeType valueSize = RapidJsonHelper::jsonSize(std::strlen(realValue));
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.SetString(realValue, valueSize, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	rapidjson::Value jsonValue;
	jsonKey.SetString(key, allocator);
	jsonValue.SetString(realValue, valueSize, allocator);
	m_value->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonValue::addInt(const char* key, int32_t value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.SetInt(value);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_value->AddMember(jsonKey, value, allocator);
}

void RapidJsonValue::addBool(const char* key, bool value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.SetBool(value);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_value->AddMember(jsonKey, value, allocator);
}

void RapidJsonValue::addValue(const char* key, const RapidJsonValue& value)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value jsonValue;
	value.copyToRapidJsonValue(&jsonValue, m_document);
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.CopyFrom(jsonValue, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_value->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonValue::addStringArray(const char* key, const std::vector<std::string>& values)
{
	if (key == nullptr)
	{
		return;
	}
	if (!m_value->IsObject())
	{
		m_value->SetObject();
	}
	rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
	if (m_value->MemberCount() == 0)
	{
		m_value->MemberReserve(RapidJsonHelper::defaultObjectMemberCapacity(), allocator);
	}
	rapidjson::Value array(rapidjson::kArrayType);
	array.Reserve(RapidJsonHelper::jsonSize(values.size()), allocator);
	for (size_t i = 0; i < values.size(); ++i)
	{
		rapidjson::Value jsonValue;
		jsonValue.SetString(values[i].c_str(), RapidJsonHelper::jsonSize(values[i].size()), allocator);
		array.PushBack(jsonValue, allocator);
	}
	rapidjson::Value::MemberIterator it = m_value->FindMember(key);
	if (it != m_value->MemberEnd())
	{
		it->value.CopyFrom(array, allocator);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_value->AddMember(jsonKey, array, allocator);
}

void RapidJsonValue::pushString(const std::string& value)
{
	RapidJsonHelper::ensureArray(m_value);
	rapidjson::Value jsonValue;
	RapidJsonHelper::setString(&jsonValue, value, m_document);
	m_value->PushBack(jsonValue, m_document->GetAllocator());
}

void RapidJsonValue::pushString(const char* value)
{
	const char* realValue = value == nullptr ? "" : value;
	RapidJsonHelper::ensureArray(m_value);
	rapidjson::Value jsonValue;
	jsonValue.SetString(realValue, RapidJsonHelper::jsonSize(std::strlen(realValue)), m_document->GetAllocator());
	m_value->PushBack(jsonValue, m_document->GetAllocator());
}

void RapidJsonValue::pushInt(int32_t value)
{
	RapidJsonHelper::ensureArray(m_value);
	m_value->PushBack(value, m_document->GetAllocator());
}

void RapidJsonValue::pushBool(bool value)
{
	RapidJsonHelper::ensureArray(m_value);
	m_value->PushBack(value, m_document->GetAllocator());
}

void RapidJsonValue::pushValue(const RapidJsonValue& value)
{
	RapidJsonHelper::ensureArray(m_value);
	rapidjson::Value jsonValue;
	value.copyToRapidJsonValue(&jsonValue, m_document);
	m_value->PushBack(jsonValue, m_document->GetAllocator());
}

std::string RapidJsonValue::toString() const
{
	return RapidJsonHelper::valueToString(m_value);
}

void RapidJsonValue::copyToRapidJsonValue(rapidjson::Value* rapidJsonValue, rapidjson::Document* rapidJsonDocument) const
{
	if (rapidJsonValue == nullptr || rapidJsonDocument == nullptr)
	{
		return;
	}
	rapidJsonValue->CopyFrom(*m_value, rapidJsonDocument->GetAllocator());
}