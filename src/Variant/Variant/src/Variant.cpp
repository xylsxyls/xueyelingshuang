#include "Variant.h"
#include "CStringManager/CStringManagerAPI.h"
#include <string.h>

Variant::Variant():
m_data(nullptr),
m_length(0),
m_type(INIT)
{

}

Variant::Variant(const Variant& value)
{
	setVariant(value);
}

Variant::Variant(const std::string& data, VariantType type, bool isBlob /*= false*/)
{
	if (data.empty())
	{
		m_data = nullptr;
		m_length = 0;
		m_type = type;
	}
	else
	{
		if (isBlob)
		{
			setBlob(data);
			m_type = type;
		}
		else
		{
			switch (type)
			{
			case Variant::INIT:
			{
				m_length = 0;
				m_data = nullptr;
				m_type = type;
				break;
			}
			case Variant::INT32:
			{
				setInt32(::atoi(data.c_str()));
				break;
			}
			case Variant::UINT64:
			{
				setUint64(CStringManager::atoui64(data.c_str()));
				break;
			}
			case Variant::DOUBLE:
			{
				setDouble(::atof(data.c_str()));
				break;
			}
			case Variant::STRING:
			{
				setString(data.c_str());
				break;
			}
			case Variant::BLOB:
			{
				setBlob(data);
				break;
			}
			default:
				break;
			}
		}
	}
#ifdef _DEBUG
	toString(m_num);
#endif
}

Variant::~Variant()
{
	clear();
}

Variant Variant::operator=(const Variant& value)
{
	clearMemory();
	setVariant(value);
	return *this;
}

Variant::Variant(const int32_t& value)
{
	setInt32(value);
}

Variant::Variant(const uint64_t& value)
{
	setUint64(value);
}

Variant::Variant(const double& value)
{
	setDouble(value);
}

Variant::Variant(const char* value)
{
	setString(value);
}

Variant::Variant(const std::string& value)
{
	setBlob(value);
}

bool Variant::operator==(const Variant& value)
{
	if (m_length != value.m_length)
	{
		return false;
	}

	if (m_data == nullptr && value.m_data == nullptr)
	{
		return true;
	}
	else if ((m_data != nullptr && value.m_data == nullptr) || (m_data == nullptr && value.m_data != nullptr))
	{
		return false;
	}

	return ::memcmp((char*)m_data, (char*)value.m_data, m_length) == 0;
}

bool Variant::operator!=(const Variant& value)
{
	if (m_length != value.m_length)
	{
		return true;
	}

	if (m_data == nullptr && value.m_data == nullptr)
	{
		return false;
	}
	else if ((m_data != nullptr && value.m_data == nullptr) || (m_data == nullptr && value.m_data != nullptr))
	{
		return true;
	}

	return ::memcmp((char*)m_data, (char*)value.m_data, m_length) != 0;
}

Variant::operator int32_t()
{
	if (m_type != INT32 || m_data == nullptr)
	{
		return 0;
	}
	return *(int32_t*)m_data;
}

Variant::operator uint64_t()
{
	if (m_type != UINT64 || m_data == nullptr)
	{
		return 0;
	}
	return *(uint64_t*)m_data;
}

Variant::operator double()
{
	if (m_type != DOUBLE || m_data == nullptr)
	{
		return 0;
	}
	return *(double*)m_data;
}

Variant::operator std::string()
{
	if ((m_type != STRING && m_type != BLOB) || m_data == nullptr)
	{
		return "";
	}
	return std::string((char*)m_data, m_length);
}

Variant::VariantType Variant::type() const
{
	return m_type;
}

void Variant::clear()
{
	clearMemory();
	m_data = nullptr;
	m_length = 0;
	m_type = INIT;

#ifdef _DEBUG
	toString(m_num);
#endif
}

std::string Variant::toString() const
{
	std::string result;
	toString(result);
	return result;
}

void Variant::toString(std::string& result) const
{
	if (m_data == nullptr)
	{
		return;
	}

	result.clear();
	switch (m_type)
	{
	case INIT:
	{
		break;
	}
	case INT32:
	{
		CStringManager::Format(result, "%d", *(int32_t*)m_data);
		break;
	}
	case UINT64:
	{
		CStringManager::Format(result, "%I64u", *(uint64_t*)m_data);
		break;
	}
	case DOUBLE:
	{
		CStringManager::Format(result, "%d", *(double*)m_data);
		break;
	}
	case STRING:
	case BLOB:
	{
		result.append((char*)m_data, m_length);
	}
	default:
		break;
	}
}

std::string Variant::toBlob() const
{
	std::string result;
	toBlob(result);
	return result;
}

void Variant::toBlob(std::string& result) const
{
	if (m_data == nullptr)
	{
		return;
	}
	result.clear();
	result.append((char*)m_data, m_length);
}

void Variant::setInt32(const int32_t& value)
{
	m_data = new int32_t(value);
	m_length = sizeof(int32_t);
	m_type = INT32;

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::setUint64(const uint64_t& value)
{
	m_data = new uint64_t(value);
	m_length = sizeof(uint64_t);
	m_type = UINT64;

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::setDouble(const double& value)
{
	m_data = new double(value);
	m_length = sizeof(double);
	m_type = DOUBLE;

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::setString(const char* value)
{
	m_type = STRING;
	if (value == nullptr)
	{
		m_length = 0;
		m_data = nullptr;
	}
	else
	{
		m_length = ::strlen(value);
		m_data = new char[m_length];
		::memcpy(m_data, value, m_length);
	}

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::setBlob(const std::string& value)
{
	m_length = value.size();
	m_type = BLOB;

	if (m_length == 0)
	{
		m_data = nullptr;
	}
	else
	{
		m_data = new char[m_length];
		::memcpy(m_data, &value[0], m_length);
	}

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::setVariant(const Variant& value)
{
	m_length = value.m_length;
	m_type = value.m_type;

	if (m_length == 0)
	{
		m_data = nullptr;
	}
	else
	{
		m_data = new char[m_length];
		::memcpy(m_data, value.m_data, m_length);
	}

#ifdef _DEBUG
	toString(m_num);
#endif
}

void Variant::clearMemory()
{
	switch (m_type)
	{
	case INIT:
	{
		break;
	}
	case INT32:
	{
		delete (int32_t*)m_data;
		break;
	}
	case UINT64:
	{
		delete (uint64_t*)m_data;
		break;
	}
	case DOUBLE:
	{
		delete (double*)m_data;
		break;
	}
	case STRING:
	{
		delete[] (char*)m_data;
		break;
	}
	case BLOB:
	{
		delete[] (char*)m_data;
		break;
	}
	default:
		break;
	}
}

Variant Variant::operator=(const int32_t& value)
{
	clearMemory();
	setInt32(value);
	return *this;
}

Variant Variant::operator=(const uint64_t& value)
{
	clearMemory();
	setUint64(value);
	return *this;
}

Variant Variant::operator=(const double& value)
{
	clearMemory();
	setDouble(value);
	return *this;
}

Variant Variant::operator=(const char* value)
{
	clearMemory();
	setString(value);
	return *this;
}

Variant Variant::operator=(const std::string& value)
{
	clearMemory();
	setBlob(value);
	return *this;
}