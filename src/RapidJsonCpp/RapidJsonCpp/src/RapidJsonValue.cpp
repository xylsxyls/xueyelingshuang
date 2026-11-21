#include "RapidJsonValue.h"

#include "RapidJsonHelper.h"
#include "rapidjson/document.h"
#include <cstring>
#include <memory>
#include <limits>
#include <stdexcept>

RapidJsonValue::RapidJsonValue() :
m_document(new rapidjson::Document(rapidjson::kObjectType))
{
}

RapidJsonValue::RapidJsonValue(const RapidJsonValue& value) :
m_document(nullptr)
{
    std::unique_ptr<rapidjson::Document> result(new rapidjson::Document());
    result->CopyFrom(*value.m_document, result->GetAllocator());
    m_document = result.release();
}

RapidJsonValue::RapidJsonValue(RapidJsonValue&& value) :
m_document(new rapidjson::Document(rapidjson::kObjectType))
{
    std::swap(m_document, value.m_document);
}

RapidJsonValue::RapidJsonValue(const rapidjson::Value* value) :
m_document(nullptr)
{
    std::unique_ptr<rapidjson::Document> result(new rapidjson::Document(rapidjson::kObjectType));
    if (value != nullptr)
    {
        result->CopyFrom(*value, result->GetAllocator(), true);
    }
    m_document = result.release();
}

RapidJsonValue::~RapidJsonValue()
{
    delete m_document;
}

RapidJsonValue& RapidJsonValue::operator=(const RapidJsonValue& value)
{
    if (this != &value)
    {
        RapidJsonValue copy(value);
        std::swap(m_document, copy.m_document);
    }
    return *this;
}

RapidJsonValue& RapidJsonValue::operator=(RapidJsonValue&& value)
{
    if (this != &value)
    {
        std::swap(m_document, value.m_document);
        value.setObject();
    }
    return *this;
}

void RapidJsonValue::setObject()
{
	m_document->SetObject();
	m_document->GetAllocator().Clear();
}

void RapidJsonValue::setArray()
{
	m_document->SetArray();
	m_document->GetAllocator().Clear();
}

void RapidJsonValue::setString(const std::string& value)
{
	RapidJsonHelper::setString(m_document, value, m_document);
}

void RapidJsonValue::setString(const char* value)
{
	const char* realValue = value == nullptr ? "" : value;
	m_document->SetString(realValue, RapidJsonHelper::jsonSize(std::strlen(realValue)), m_document->GetAllocator());
}

void RapidJsonValue::setInt(int32_t value)
{
	m_document->SetInt(value);
}

void RapidJsonValue::setBool(bool value)
{
	m_document->SetBool(value);
}

bool RapidJsonValue::isObject() const
{
	return m_document->IsObject();
}

bool RapidJsonValue::isArray() const
{
	return m_document->IsArray();
}

size_t RapidJsonValue::size() const
{
	if (m_document->IsArray())
	{
		return static_cast<size_t>(m_document->Size());
	}
	if (m_document->IsObject())
	{
		return static_cast<size_t>(m_document->MemberCount());
	}
	return 0;
}

void RapidJsonValue::reserve(size_t capacity)
{
	if (m_document->IsArray())
	{
        if (capacity > static_cast<size_t>((std::numeric_limits<ptrdiff_t>::max)()) / sizeof(rapidjson::Value))
        {
            throw std::length_error("RapidJSON array size exceeds the addressable range");
        }
		m_document->Reserve(RapidJsonHelper::jsonSize(capacity), m_document->GetAllocator());
	}
}

void RapidJsonValue::addString(const char* key, const std::string& value)
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

void RapidJsonValue::addString(const char* key, const char* value)
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

void RapidJsonValue::addInt(const char* key, int32_t value)
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

void RapidJsonValue::addBool(const char* key, bool value)
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

void RapidJsonValue::addValue(const char* key, const RapidJsonValue& value)
{
	if (key == nullptr)
	{
		return;
	}
	rapidjson::Value jsonValue;
	value.copyToRapidJsonValue(&jsonValue, m_document);
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
		it->value.Swap(jsonValue);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, jsonValue, allocator);
}

void RapidJsonValue::addStringArray(const char* key, const std::vector<std::string>& values)
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
		it->value.Swap(array);
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, allocator);
	m_document->AddMember(jsonKey, array, allocator);
}

void RapidJsonValue::pushString(const std::string& value)
{
	RapidJsonHelper::ensureArray(m_document);
	rapidjson::Value jsonValue;
	RapidJsonHelper::setString(&jsonValue, value, m_document);
	m_document->PushBack(jsonValue, m_document->GetAllocator());
}

void RapidJsonValue::pushString(const char* value)
{
	const char* realValue = value == nullptr ? "" : value;
	RapidJsonHelper::ensureArray(m_document);
	rapidjson::Value jsonValue;
	jsonValue.SetString(realValue, RapidJsonHelper::jsonSize(std::strlen(realValue)), m_document->GetAllocator());
	m_document->PushBack(jsonValue, m_document->GetAllocator());
}

void RapidJsonValue::pushInt(int32_t value)
{
	RapidJsonHelper::ensureArray(m_document);
	m_document->PushBack(value, m_document->GetAllocator());
}

void RapidJsonValue::pushBool(bool value)
{
	RapidJsonHelper::ensureArray(m_document);
	m_document->PushBack(value, m_document->GetAllocator());
}

void RapidJsonValue::pushValue(const RapidJsonValue& value)
{
	rapidjson::Value jsonValue;
	value.copyToRapidJsonValue(&jsonValue, m_document);
	RapidJsonHelper::ensureArray(m_document);
	m_document->PushBack(jsonValue, m_document->GetAllocator());
}

std::string RapidJsonValue::getStringOrDefault(const char* key, const std::string& defaultValue) const
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

int32_t RapidJsonValue::getIntOrDefault(const char* key, int32_t defaultValue) const
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

bool RapidJsonValue::getBoolOrDefault(const char* key, bool defaultValue) const
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

RapidJsonValue RapidJsonValue::getValue(const char* key) const
{
	if (key == nullptr)
	{
		return RapidJsonValue();
	}
	if (!m_document->IsObject())
	{
		return RapidJsonValue();
	}
	rapidjson::Value::ConstMemberIterator it = m_document->FindMember(key);
	if (it == m_document->MemberEnd())
	{
		return RapidJsonValue();
	}
	return RapidJsonValue(&it->value);
}

std::vector<RapidJsonValue> RapidJsonValue::getArrayValueOrEmpty(const char* key) const
{
	std::vector<RapidJsonValue> values;
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
	values.reserve(static_cast<size_t>(it->value.Size()));
	for (rapidjson::SizeType i = 0; i < it->value.Size(); ++i)
	{
		values.emplace_back(&it->value[i]);
	}
	return values;
}

std::vector<std::string> RapidJsonValue::getStringArrayOrEmpty(const char* key) const
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

std::string RapidJsonValue::toString() const
{
	return RapidJsonHelper::valueToString(m_document);
}

void RapidJsonValue::copyToRapidJsonValue(rapidjson::Value* rapidJsonValue, rapidjson::Document* rapidJsonDocument) const
{
	if (rapidJsonValue == nullptr || rapidJsonDocument == nullptr)
	{
		return;
	}
	rapidJsonValue->CopyFrom(*m_document, rapidJsonDocument->GetAllocator());
}