#include "RapidJsonCpp.h"
#include "RapidJsonHelper.h"
#include "rapidjson/document.h"
#include <memory>

RapidJsonCpp::RapidJsonCpp() :
m_document(nullptr),
m_value(nullptr),
m_hasKey(false)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Document> value(new rapidjson::Document());
	document->SetObject();
	m_document = document.release();
	m_value = value.release();
}

RapidJsonCpp::RapidJsonCpp(const RapidJsonCpp& rapidJson) :
m_document(nullptr),
m_value(nullptr),
m_key(rapidJson.m_key),
m_hasKey(rapidJson.m_hasKey)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Document> value(new rapidjson::Document());
	document->CopyFrom(*rapidJson.m_document, document->GetAllocator());
	value->CopyFrom(*rapidJson.m_value, value->GetAllocator());
	m_document = document.release();
	m_value = value.release();
}

RapidJsonCpp::RapidJsonCpp(const rapidjson::Value* rapidjson) :
m_document(nullptr),
m_value(nullptr),
m_hasKey(false)
{
	std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
	std::unique_ptr<rapidjson::Document> value(new rapidjson::Document());
	if (rapidjson == nullptr)
	{
		document->SetNull();
		value->SetNull();
		m_document = document.release();
		m_value = value.release();
		return;
	}
	value->CopyFrom(*rapidjson, value->GetAllocator(), true);
	document->CopyFrom(*rapidjson, document->GetAllocator(), true);
	m_document = document.release();
	m_value = value.release();
}

RapidJsonCpp::~RapidJsonCpp()
{
	delete m_value;
	delete m_document;
}

bool RapidJsonCpp::parse(const std::string& json)
{
	m_key.clear();
	m_hasKey = false;
    const bool valid = RapidJsonHelper::parse(*m_document, json);
    setSelected(valid ? m_document : nullptr);
    return valid;
}

void RapidJsonCpp::setSelected(const rapidjson::Value* value)
{
    m_value->SetNull();
    m_value->GetAllocator().Clear();
    if (value != nullptr)
    {
        m_value->CopyFrom(*value, m_value->GetAllocator());
    }
}

bool RapidJsonCpp::hasMember(const std::string& key)
{
	if (!m_document->IsObject())
	{
		return false;
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(rapidjson::StringRef(key.c_str(), RapidJsonHelper::jsonSize(key.size())));
	return m_document->FindMember(strKey) != m_document->MemberEnd();
}

std::string RapidJsonCpp::toString()
{
	return RapidJsonHelper::valueToString(m_document);
}

bool RapidJsonCpp::isNull() const
{
	return m_value->IsNull();
}

bool RapidJsonCpp::isInt() const
{
	return m_value->IsInt();
}

int32_t RapidJsonCpp::toIntOrDefault(int32_t defaultValue) const
{
	if (!m_value->IsInt())
	{
		return defaultValue;
	}
	return m_value->GetInt();
}

std::string RapidJsonCpp::toStringOrDefault(const std::string& defaultValue) const
{
	if (!m_value->IsString())
	{
		return defaultValue;
	}
	return std::string(m_value->GetString(), m_value->GetStringLength());
}

const rapidjson::Value* RapidJsonCpp::toRapidJsonValue() const
{
	return m_value;
}

std::vector<RapidJsonCpp> RapidJsonCpp::toArray()
{
	std::vector<RapidJsonCpp> result;
	if (!m_value->IsArray())
	{
		return result;
	}
	const auto& rapidjsonArray = m_value->GetArray();
	result.reserve(rapidjsonArray.Size());
	for (rapidjson::SizeType index = 0; index < rapidjsonArray.Size(); ++index)
	{
		result.emplace_back(&rapidjsonArray[index]);
	}
	return result;
}

RapidJsonCpp& RapidJsonCpp::assignValue(rapidjson::Value& value)
{
    if (!m_hasKey)
    {
        setSelected(&value);
        value.SetNull();
        m_document->SetNull();
        m_document->GetAllocator().Clear();
        m_document->CopyFrom(*m_value, m_document->GetAllocator());
        return *this;
    }
    if (!m_document->IsObject())
    {
        m_document->SetObject();
    }
    rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
    rapidjson::Value key(rapidjson::StringRef(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size())));
    rapidjson::Value::MemberIterator it = m_document->FindMember(key);
    setSelected(&value);
    if (it != m_document->MemberEnd())
    {
        it->value.Swap(value);
    }
    else
    {
        key.SetString(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size()), allocator);
        m_document->AddMember(key, value, allocator);
    }
    m_key.clear();
    m_hasKey = false;
    return *this;
}

RapidJsonCpp& RapidJsonCpp::operator=(const rapidjson::Value* value)
{
    rapidjson::Value owned;
    if (value != nullptr)
    {
        owned.CopyFrom(*value, m_document->GetAllocator(), true);
    }
    return assignValue(owned);
}

RapidJsonCpp& RapidJsonCpp::operator=(int32_t value)
{
    rapidjson::Value number(value);
    return assignValue(number);
}

RapidJsonCpp& RapidJsonCpp::operator=(const std::string& value)
{
    rapidjson::Value text(value.data(), RapidJsonHelper::jsonSize(value.size()), m_document->GetAllocator());
    return assignValue(text);
}

RapidJsonCpp& RapidJsonCpp::operator=(const char* value)
{
    rapidjson::Value text(value ? value : "", m_document->GetAllocator());
    return assignValue(text);
}

RapidJsonCpp& RapidJsonCpp::operator=(const std::vector<RapidJsonCpp>& value)
{
    rapidjson::Value array(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& allocator = m_document->GetAllocator();
    array.Reserve(RapidJsonHelper::jsonSize(value.size()), allocator);
    for (size_t i = 0; i < value.size(); ++i)
    {
        rapidjson::Value item(*value[i].m_document, allocator);
        array.PushBack(item, allocator);
    }
    return assignValue(array);
}

RapidJsonCpp& RapidJsonCpp::operator=(const RapidJsonCpp& value)
{
    if (this == &value && !m_hasKey)
    {
        return *this;
    }
    return operator=(static_cast<const rapidjson::Value*>(value.m_document));
}

RapidJsonCpp& RapidJsonCpp::operator[](const std::string& key)
{
    if (m_hasKey && m_key == key)
    {
        return *this;
    }
	m_key = key;
	m_hasKey = true;
	if (m_document->IsObject())
	{
		rapidjson::Value strKey(rapidjson::kStringType);
		strKey.SetString(rapidjson::StringRef(m_key.c_str(), RapidJsonHelper::jsonSize(m_key.size())));
		rapidjson::Value::MemberIterator it = m_document->FindMember(strKey);
		if (it != m_document->MemberEnd())
		{
			setSelected(&it->value);
			return *this;
		}
	}
	setSelected(nullptr);
	return *this;
}

//#include "RapidJsonDocument.h"
//#include "RapidJsonValue.h"
//int main()
//{
//    RapidJsonCpp message;
//    message["status"] = 200;
//    message["user"] = "Ada";
//    RapidJsonDocument doc;
//    if (!doc.parse(message.toString()) || doc.getIntOrDefault("status", -1) != 200) return 1;
//    RapidJsonValue row;
//    row.addBool("active", true);
//    row.addString("name", doc.getStringOrDefault("user", ""));
//    row.addInt("status", doc.getIntOrDefault("status", 0));
//    if (!row.isObject() || row.size() != 3 || !row.getBoolOrDefault("active", false)) return 1;
//    RapidJsonValue rows;
//    rows.setArray();
//    rows.reserve(4);
//    rows.pushValue(row);
//    rows.pushInt(42);
//    rows.pushString("done");
//    rows.pushBool(true);
//    doc.addValue("rows", rows);
//    doc.addStringArray("tags", std::vector<std::string>(1, "example"));
//    doc.addString("statusText", "ok");
//    doc.addBool("ready", true);
//    doc.addInt("count", static_cast<int32_t>(rows.size()));
//    if (!doc.isObject() || doc.getArrayValueOrEmpty("rows").size() != 4 ||
//        doc.getStringArrayOrEmpty("tags").size() != 1 || !doc.getBoolOrDefault("ready", false) ||
//        !doc.getValue("rows").isArray()) return 1;
//    RapidJsonCpp response;
//    if (!response.parse(doc.toString()) || !response.hasMember("rows")) return 1;
//    if (!response["count"].isInt() || response.toIntOrDefault(-1) != 4) return 1;
//    if (response["statusText"].toStringOrDefault("") != "ok" || !response["missing"].isNull()) return 1;
//    const std::vector<RapidJsonCpp> values = response["rows"].toArray();
//    RapidJsonCpp copy;
//    copy = values;
//    return copy.toArray().size() == 4 && doc.getStringOrDefault("user", "") == "Ada" ? 0 : 1;
//}