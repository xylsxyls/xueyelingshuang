#pragma once
#include "RapidJsonMacro.h"
#include "rapid_json/document.h"
#include <vector>

class RapidJsonAPI RapidJson
{
public:
	/** ���캯��
	*/
	RapidJson();

	/** �������캯��
	@param [in] rapidJson json
	*/
	RapidJson(const RapidJson& rapidJson);

	/** �������캯��
	@param [in] value ֵ
	*/
	RapidJson(const rapidjson::Value& value);

public:
	/** ����json
	@param [in] json json�ַ���
	@return �����Ƿ�����ɹ�
	*/
	bool parse(const std::string& json);

	/** �Ƿ��иó�Ա
	@param [in] key �ؼ���
	@return �����Ƿ��йؼ��ֳ�Ա
	*/
	bool hasMember(const std::string& key);

	/** ת��Ϊ�ַ���
	@return ����ת�����ַ���
	*/
	std::string toString();

	/** ת��Ϊ�����ֵ
	@return ����ת���ľ����ֵ
	*/
	const rapidjson::Value& toValue();

	/** ת��Ϊ����
	@return ����ת��������
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