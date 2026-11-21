#include "RapidJsonHelper.h"

#include "rapidjson/document.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <limits>
#include <stdexcept>

bool RapidJsonHelper::parse(rapidjson::Document& document, const std::string& json)
{
    rapidjson::Document parsed;
    if (!json.empty() && json.find('\0') == std::string::npos && !hasUnpairedSurrogate(json))
    {
        parsed.Parse<rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag |
            rapidjson::kParseFullPrecisionFlag>(json.data(), json.size());
        if (!parsed.HasParseError())
        {
            document.Swap(parsed);
            return true;
        }
    }
    document.SetObject();
    document.GetAllocator().Clear();
    return false;
}

bool RapidJsonHelper::hasUnpairedSurrogate(const std::string& json)
{
    size_t offset = json.find('\\');
    while (offset != std::string::npos)
    {
        size_t consumed = 2;
        if (json.size() - offset >= 6 && json[offset + 1] == 'u' &&
            (json[offset + 2] == 'D' || json[offset + 2] == 'd'))
        {
            const char digit = json[offset + 3];
            if ((digit >= 'c' && digit <= 'f') || (digit >= 'C' && digit <= 'F'))
            {
                return true;
            }
            if (digit == '8' || digit == '9' || digit == 'A' || digit == 'a' || digit == 'B' || digit == 'b')
            {
                // 上游会核对紧随高位的六字节低位转义，此处跳过整对
                consumed = 12;
            }
        }
        if (json.size() - offset < consumed)
        {
            return true;
        }
        offset = json.find('\\', offset + consumed);
    }
    return false;
}

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
    if (!value->Accept(writer))
    {
        return std::string();
    }
    const char* data = buffer.GetString();
    const size_t length = buffer.GetSize();
    rapidjson::MemoryStream input(data, length);
    while (input.Tell() < length)
    {
        if (static_cast<unsigned char>(input.Peek()) < 0x80)
        {
            input.Take();
            continue;
        }
        unsigned codepoint;
        if (!rapidjson::UTF8<>::Decode(input, &codepoint))
        {
            return std::string();
        }
    }
    return std::string(data, length);
}