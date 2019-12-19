#include "RapidJson.h"
#include "rapid_json/writer.h"

RapidJson::RapidJson():
m_alloc(m_document.GetAllocator())
{
	m_document.SetObject();
}

RapidJson::RapidJson(const RapidJson& rapidJson):
m_alloc(m_document.GetAllocator())
{
	m_key = rapidJson.m_key;
	m_value.CopyFrom(rapidJson.m_value, m_alloc);
	m_document.CopyFrom(rapidJson.m_document, m_alloc);
}

RapidJson::RapidJson(const rapidjson::Value& rapidjson):
m_alloc(m_document.GetAllocator())
{
	m_value.CopyFrom(rapidjson, m_alloc);
	m_document.CopyFrom(rapidjson, m_alloc);
}

bool RapidJson::parse(const std::string& json)
{
	return !(m_document.Parse(json.c_str()).HasParseError());
}

bool RapidJson::hasMember(const std::string& key)
{
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(key.c_str(), m_alloc);
	return m_document.HasMember(strKey);
}

std::string RapidJson::toString()
{
	rapidjson::StringBuffer m_jsonBuffer;
	rapidjson::Writer<rapidjson::StringBuffer> m_writer(m_jsonBuffer);
	m_document.Accept(m_writer);
	return m_jsonBuffer.GetString();
}

const rapidjson::Value& RapidJson::toValue()
{
	return m_value;
}

std::vector<RapidJson> RapidJson::toArray()
{
	std::vector<RapidJson> result;
	if (!m_value.IsArray())
	{
		return result;
	}
	const auto& rapidjsonArray = m_value.GetArray();
	int32_t index = -1;
	while (index++ != rapidjsonArray.Size() - 1)
	{
		rapidjson::Value value;
		value.CopyFrom(rapidjsonArray[index], m_alloc);
		result.push_back(value);
	}
	return result;
}

RapidJson RapidJson::operator=(rapidjson::Value value)
{
	if (m_key.empty())
	{
		m_value.CopyFrom(value, m_alloc);
		return *this;
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), m_alloc);
	if (m_document.HasMember(strKey))
	{
		m_document[strKey] = value;
	}
	else
	{
		m_document.AddMember(strKey, value, m_alloc);
	}
	m_key.clear();
	return *this;
}

RapidJson RapidJson::operator=(int32_t value)
{
	if (m_key.empty())
	{
		m_value.CopyFrom(rapidjson::Value(value), m_alloc);
		return *this;
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), m_alloc);
	if (m_document.HasMember(strKey))
	{
		m_document[strKey] = value;
	}
	else
	{
		m_document.AddMember(strKey, value, m_alloc);
	}
	m_key.clear();
	return *this;
}

RapidJson RapidJson::operator=(const std::string& value)
{
	rapidjson::Value strValue(rapidjson::kStringType);
	strValue.SetString(value.c_str(), m_alloc);
	if (m_key.empty())
	{
		m_value.CopyFrom(strValue, m_alloc);
		return *this;
	}
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), m_alloc);
	if (m_document.HasMember(strKey))
	{
		m_document[strKey] = strValue;
	}
	else
	{
		m_document.AddMember(strKey, strValue, m_alloc);
	}
	m_key.clear();
	return *this;
}

RapidJson RapidJson::operator=(std::vector<RapidJson>& value)
{
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), m_alloc);
	rapidjson::Value rapidjsonValue(rapidjson::Type::kArrayType);
	int32_t index = -1;
	while (index++ != value.size() - 1)
	{
		rapidjsonValue.PushBack(value[index].m_document, m_alloc);
	}
	if (m_document.HasMember(strKey))
	{
		m_document[strKey] = rapidjsonValue;
	}
	else
	{
		m_document.AddMember(strKey, rapidjsonValue, m_alloc);
	}
	return *this;
}

RapidJson RapidJson::operator=(const RapidJson& value)
{
	rapidjson::Value strKey(rapidjson::kStringType);
	strKey.SetString(m_key.c_str(), m_alloc);
	rapidjson::Value rapidjsonValue(rapidjson::Type::kObjectType);
	rapidjsonValue.CopyFrom(value.m_document, m_alloc);
	if (m_document.HasMember(strKey))
	{
		m_document[strKey] = rapidjsonValue;
	}
	else
	{
		m_document.AddMember(strKey, rapidjsonValue, m_alloc);
	}
	return *this;
}

RapidJson& RapidJson::operator[](const std::string& key)
{
	m_key = key;
	if (hasMember(key))
	{
		rapidjson::Value strKey(rapidjson::kStringType);
		strKey.SetString(m_key.c_str(), m_alloc);
		m_value.CopyFrom(m_document[strKey], m_alloc);
	}
	return *this;
}

//int main()
//{
//	RapidJson json;
//	json["1"] = 20;
//	json["2"] = "2000000";
//	json["2"] = "2000000";
//	json["3"] = 40;
//	std::string result2 = json.toString();
//	std::vector<RapidJson> x = json["3"].toArray();
//	std::string result = json.toString();
//	std::vector<RapidJson> x2 = json["3"].toArray();
//
//	//std::vector<RapidJson> vecJson;
//	//vecJson.push_back(json);
//	//vecJson.push_back(json);
//	//json["4"] = 50;
//	//vecJson.push_back(json);
//	//json["5"] = vecJson;
//	//std::string result = json.toString();
//	//RapidJson json2;
//	//json2.parse(result);
//	//auto dss = json2.toString();
//	//dss = json2.toString();
//	//json2["6"] = "23";
//	//auto sdsd = json2["6"].toValue().GetString();
//	//dss = json2.toString();
//	//auto x = json2["5"].toArray();
//	//auto sss = RapidJson(x[2].toValue())["4"].toValue().GetInt();
//	//auto dsfsdsd = json2.toString();
//	getchar();
//	return 0;
//}