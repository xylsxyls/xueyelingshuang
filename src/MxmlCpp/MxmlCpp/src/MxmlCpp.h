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

	/** ����һ��xml�ַ����������ٶ�ȡ���ַ������ϴδ������ַ���
	@return �����Ƿ񴴽��ɹ�
	*/
	bool create();

	/** ����ӽڵ�
	@param [in] name �ӽڵ���
	@return �����Ƿ񴴽��ɹ�
	*/
	bool addChild(const std::string& name);

	/** ��������ֵĽڵ�
	@param [in] name �ӽڵ���
	@param [in] text �ӽڵ�����
	@return �����Ƿ񴴽��ɹ�
	*/
	bool addChild(const std::string& name, const std::string& text);

	/** ��ӽڵ�����
	@param [in] name �ڵ�������
	@param [in] attr ��������
	@return �����Ƿ���ӳɹ�
	*/
	bool addAttr(const std::string& name, const std::string& attr);

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
	mxml_node_t* m_xmlVisit;
};