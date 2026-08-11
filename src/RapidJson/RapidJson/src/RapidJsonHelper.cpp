#include "RapidJsonHelper.h"

#include "rapid_json/document.h"
#include "rapid_json/stringbuffer.h"
#include "rapid_json/writer.h"
#include <limits>

rapidjson::SizeType RapidJsonHelper::jsonSize(size_t value)
{
	size_t maxSize = static_cast<size_t>((std::numeric_limits<rapidjson::SizeType>::max)());
	if (value > maxSize)
	{
		return (std::numeric_limits<rapidjson::SizeType>::max)();
	}
	return static_cast<rapidjson::SizeType>(value);
}

rapidjson::SizeType RapidJsonHelper::defaultObjectMemberCapacity()
{
	return 8;
}

void RapidJsonHelper::ensureArray(rapidjson::Value* value)
{
	if (value == nullptr)
	{
		return;
	}
	if (!value->IsArray())
	{
		value->SetArray();
	}
}

void RapidJsonHelper::setString(rapidjson::Value* jsonValue, const std::string& value, rapidjson::Document* document)
{
	if (jsonValue == nullptr || document == nullptr)
	{
		return;
	}
	jsonValue->SetString(value.c_str(), jsonSize(value.size()), document->GetAllocator());
}

void RapidJsonHelper::addMember(rapidjson::Value* object,
								const char* key,
								rapidjson::Value* value,
								rapidjson::Document* document)
{
	if (object == nullptr || key == nullptr || value == nullptr || document == nullptr)
	{
		return;
	}
	if (!object->IsObject())
	{
		object->SetObject();
	}
	rapidjson::Value::MemberIterator it = object->FindMember(key);
	if (it != object->MemberEnd())
	{
		it->value.CopyFrom(*value, document->GetAllocator());
		return;
	}
	rapidjson::Value jsonKey;
	jsonKey.SetString(key, document->GetAllocator());
	object->AddMember(jsonKey, *value, document->GetAllocator());
}

std::string RapidJsonHelper::valueToString(const rapidjson::Value* value)
{
	if (value == nullptr)
	{
		return std::string();
	}
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	value->Accept(writer);
	return buffer.GetString();
}