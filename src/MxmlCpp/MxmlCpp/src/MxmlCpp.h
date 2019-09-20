#pragma once
#include "MxmlCppMacro.h"
#include <string>

struct _mxml_node_s;
typedef struct _mxml_node_s mxml_node_t;

/** xml������
*/
class MxmlCpp
{
public:
	/** ���캯��
	*/
	MxmlCpp();

	/** ��������
	*/
	~MxmlCpp();

public:
	/** ����xml�ַ���
	@param [in] xml xml�ַ���
	@return �����Ƿ���سɹ������سɹ���һ���ܶ�ȡÿ���ֶε�ֵ������׼��xml�޷���ȡ�ڲ���ֵ
	*/
	bool load(const std::string& xml);

	/** �����ӽڵ�
	@param [in] element �ӽڵ����ƣ��ӽڵ��������Ϊ��������һ���ӽڵ�
	@param [in] attr �������ƣ�������뵽���и����Եĵ�һ���ڵ�
	@param [in] value ����ֵ��������뵽ƥ������ֵ�ĵ�һ���ڵ㣬����Ҫ��attr������ֵ
	@return �����Ƿ����ɹ�
	*/
	bool firstChild(const std::string& element = "", const std::string& attr = "", const std::string& value = "");

	/** ���뵽��һ���ڵ�
	@return �����Ƿ����ɹ�
	*/
	bool pre();

	/** ���뵽��һ���ڵ�
	@return �����Ƿ����ɹ�
	*/
	bool next();

	/** ���뵽���ڵ�
	@return �����Ƿ����ɹ�
	*/
	bool parent();

	/** ��ȡ�ڵ�����
	@return ���ؽڵ�����
	*/
	std::string element();

	/** ��ȡ����ֵ
	@param [in] name ������
	@return ��������ֵ
	*/
	std::string attr(const std::string& name);

	/** ��ȡ�ڵ�����
	@return ���ؽڵ�����
	*/
	std::string text();

	/** ����xml�ַ���
	@return ����xml�ַ���
	*/
	std::string toString();

private:
	mxml_node_t* m_xml;
	mxml_node_t* m_xmlRead;
};