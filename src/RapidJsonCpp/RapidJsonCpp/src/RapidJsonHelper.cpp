#include "RapidJsonHelper.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <limits>
#include <stdexcept>

rapidjson::SizeType RapidJsonHelper::jsonSize(size_t value)
{
	size_t maxSize = static_cast<size_t>((std::numeric_limits<rapidjson::SizeType>::max)());
	if (value > maxSize)
	{
		throw std::length_error("RapidJSON SizeType range exceeded");
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

std::string RapidJsonHelper::valueToString(const rapidjson::Value* value)
{
	if (value == nullptr)
	{
		return std::string();
	}
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	value->Accept(writer);
	return std::string(buffer.GetString(), buffer.GetSize());
}