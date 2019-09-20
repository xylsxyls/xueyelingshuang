#include "MxmlCpp.h"
#include "mxml.h"
#include <stdint.h>
#ifdef _WIN64
#pragma comment(lib, "mxmlstat_64.lib")
#else
#pragma comment(lib, "mxmlstat_32.lib")
#endif

MxmlCpp::MxmlCpp():
m_xml(nullptr),
m_xmlRead(nullptr)
{

}

MxmlCpp::~MxmlCpp()
{
	if (m_xml != nullptr)
	{
		mxmlRelease(m_xml);
	}
}

bool MxmlCpp::load(const std::string& xml)
{
	m_xml = mxmlLoadString(nullptr, xml.c_str(), MXML_NO_CALLBACK);
	m_xmlRead = m_xml;
	return m_xml != nullptr;
}

bool MxmlCpp::firstChild(const std::string& element, const std::string& attr, const std::string& value)
{
	if (m_xmlRead == nullptr)
	{
		return false;
	}
	mxml_node_t* read = nullptr;
	if (element.empty())
	{
		read = mxmlGetFirstChild(m_xmlRead);
		if (read == nullptr)
		{
			return false;
		}
		m_xmlRead = read;
		return true;
	}
	read = mxmlFindElement(m_xmlRead, m_xmlRead, element.c_str(), attr.empty() ? nullptr : attr.c_str(), value.empty() ? nullptr : value.c_str(), MXML_DESCEND);
	if (read == nullptr)
	{
		return false;
	}
	m_xmlRead = read;
	return true;
}

bool MxmlCpp::pre()
{
	if (m_xmlRead == nullptr)
	{
		return false;
	}
	mxml_node_t* read = mxmlGetPrevSibling(m_xmlRead);
	if (read == nullptr)
	{
		return false;
	}
	m_xmlRead = read;
	return true;
}

bool MxmlCpp::next()
{
	if (m_xmlRead == nullptr)
	{
		return false;
	}
	mxml_node_t* read = mxmlGetNextSibling(m_xmlRead);
	if (read == nullptr)
	{
		return false;
	}
	m_xmlRead = read;
	return true;
}

bool MxmlCpp::parent()
{
	if (m_xmlRead == nullptr)
	{
		return false;
	}
	mxml_node_t* read = mxmlGetParent(m_xmlRead);
	if (read == nullptr)
	{
		return false;
	}
	m_xmlRead = read;
	return true;
}

std::string MxmlCpp::element()
{
	if (m_xmlRead == nullptr)
	{
		return "";
	}
	const char* element = mxmlGetElement(m_xmlRead);
	if (element == nullptr)
	{
		return "";
	}
	return element;
}

std::string MxmlCpp::attr(const std::string& name)
{
	if (m_xmlRead == nullptr)
	{
		return "";
	}
	const char* attr = mxmlElementGetAttr(m_xmlRead, name.c_str());
	if (attr == nullptr)
	{
		return "";
	}
	return attr;
}

std::string MxmlCpp::text()
{
	if (m_xmlRead == nullptr)
	{
		return "";
	}
	const char* text = mxmlGetText(m_xmlRead, nullptr);
	if (text == nullptr)
	{
		return "";
	}
	return text;
}

std::string MxmlCpp::toString()
{
	std::string result;
	//这里长度必须传1，传0返回的时候回有偶发崩溃
	int32_t size = mxmlSaveString(m_xml, &(result[0]), 1, MXML_NO_CALLBACK);
	result.resize(size - 1);
	mxmlSaveString(m_xml, &(result[0]), size, MXML_NO_CALLBACK);
	return result;
}

//int main()
//{
////	std::string xmlStr = "<?xml version=\"1.0\" encoding=\"GB2312\" ?>"
////"<Notify>"
////"<CmdType>Keepalive</CmdType>"
////	"<SN>1650</SN>"
////	"<DeviceID>34020000002000000002</DeviceID>"
////	"<Status>OK</Status>"
////"</Notify>";
//
//	std::string strxml = "<?xml version=\"1.0\"encoding=\"GB2312\" ?>"
//		"<Notify>"
//		"<CmdType>Keepalive</CmdType>"
//		"<SN a=\"1\">1660</SN>"
//		"<SN b=\"1\">1661</SN>"
//		"<DeviceID>34020000002000000002</DeviceID>"
//		"<Status>OK</Status>"
//		"</Notify>";
//
//	MxmlCpp xml;
//	xml.load(strxml);
//	auto dssdsd = xml.toString();
//	xml.firstChild();
//	xml.firstChild("SN","","1");
//	//xml.next();
//	std::string text = xml.text();
//	std::string dsds = xml.attr("b");
//	xml.firstChild();
//	std::string xxx = xml.text();
//	xml.parent();
//	xml.next();
//	xml.firstChild();
//	std::string xxx2 = xml.text();
//	return 0;
//}