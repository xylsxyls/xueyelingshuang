#pragma once
#include "VariantMacro.h"
#include <stdint.h>
#include <string>

class VariantAPI Variant
{
public:
	enum VariantType
	{
		/** ��ʼ��
		*/
		INIT,

		/** int32_t
		*/
		INT32,

		/** uint64_t
		*/
		UINT64,

		/** double
		*/
		DOUBLE,

		/** char*
		*/
		STRING,

		/** �����ƺ�\0
		*/
		BLOB
	};
public:
	Variant();

	~Variant();

	Variant(const Variant& value);

	Variant operator=(const Variant& value);

	Variant(const int32_t& value);
	Variant(const uint64_t& value);
	Variant(const double& value);
	Variant(const char* value);
	Variant(const std::string& value);

	Variant(const std::string& data, VariantType type, bool isBlob = false);

	Variant operator=(const int32_t& value);
	Variant operator=(const uint64_t& value);
	Variant operator=(const double& value);
	Variant operator=(const char* value);
	Variant operator=(const std::string& value);

	//ֻ�Ƚ��ڴ�
	bool operator==(const Variant& value);
	bool operator!=(const Variant& value);

	operator int32_t();
	operator uint64_t();
	operator double();
	operator std::string();

	VariantType type() const;

	void clear();

	//���ַ�����ʽ���
	std::string toString() const;

	//���ڲ��Ķ����ƴ洢���
	std::string toBlob() const;

	//���ַ�����ʽ���
	void toString(std::string& result) const;

	//���ڲ��Ķ����ƴ洢���
	void toBlob(std::string& result) const;

protected:
	void setInt32(const int32_t& value);
	void setUint64(const uint64_t& value);
	void setDouble(const double& value);
	void setString(const char* value);
	void setBlob(const std::string& value);
	void setVariant(const Variant& value);

private:
	void clearMemory();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
#ifdef _DEBUG
	std::string m_num;
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	VariantType m_type;
	size_t m_length;
	void* m_data;
};