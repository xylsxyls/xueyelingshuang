#pragma once
#include "RapidJsonMacro.h"
#include "rapid_json/document.h"
#include <vector>

class RapidJsonAPI RapidJson
{
public:
	/** 构造函数
	*/
	RapidJson();

	/** 拷贝构造函数
	@param [in] rapidJson json
	*/
	RapidJson(const RapidJson& rapidJson);

	/** 拷贝构造函数
	@param [in] value 值
	*/
	RapidJson(const rapidjson::Value& value);

public:
	/** 解析json
	@param [in] json json字符串
	@return 返回是否解析成功
	*/
	bool parse(const std::string& json);

	/** 是否有该成员
	@param [in] key 关键字
	@return 返回是否有关键字成员
	*/
	bool hasMember(const std::string& key);

	/** 转化为字符串
	@return 返回转化的字符串
	*/
	std::string toString();

	/** 转化为具体的值
	@return 返回转化的具体的值
	*/
	const rapidjson::Value& toValue();

	/** 转化为数组
	@return 返回转化的数组
	*/
	std::vector<RapidJson> toArray();

	RapidJson operator=(int32_t value);
	RapidJson operator=(const std::string& value);
	RapidJson operator=(rapidjson::Value value);
	RapidJson operator=(std::vector<RapidJson>& value);
	RapidJson& operator[](const std::string& key);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	rapidjson::Document m_document;
	rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>& m_alloc;
	rapidjson::Value m_value;
	std::string m_key;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};