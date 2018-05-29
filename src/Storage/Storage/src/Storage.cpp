#include "Storage.h"

void Storage::insert(int32_t key, const boost::any& anyValue, const std::string& type)
{
	m_mapData[key] = anyValue;
	m_mapType[key] = type;
}

std::string Storage::type(int32_t key)
{
	auto itType = m_mapType.find(key);
	if (itType != m_mapType.end())
	{
		return itType->second;
	}
	return "";
}

void Storage::remove(int32_t key)
{
	auto itData = m_mapData.find(key);
	if (itData != m_mapData.end())
	{
		m_mapData.erase(itData);
		return;
	}
	auto itType = m_mapType.find(key);
	if (itType != m_mapType.end())
	{
		m_mapType.erase(itType);
		return;
	}
	return;
}

//int main()
//{
//	Storage s;
//	std::string s2 = "2";
//	typeid(s2).name();
//	s.add(1, "3");
//	auto sdsdss = s.type(1);
//	if (sdsdss == "class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >")
//	{
//		auto sd = s.find<std::string>(1);
//	}
//	if (sdsdss == "char const [2]")
//	{
//		auto sd = s.find<const char*>(1);
//		int x = 3;
//	}
//	
//	//const char* sdss = boost::any_cast<const char*>(sd);
//	getchar();
//	return 0;
//}