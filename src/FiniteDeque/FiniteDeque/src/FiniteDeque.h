#pragma once
#include <stdint.h>
#include <deque>

/** ���޶���
*/
template <class Type>
class FiniteDeque : public std::deque < Type >
{
public:
	enum FiniteType
	{
		/** ����������֮��������ʧ��
		*/
		FINITE,

		/** ����������֮�������Ӷ���������ӵ�
		*/
		FLOW
	};

public:
	/** ���캯��
	*/
	FiniteDeque();

public:
	/** �������޸���
	@param [in] num ����
	@param [in] finiteType ��������
	*/
	void setFinite(size_t num, FiniteType finiteType = FLOW);

	/** ׷��
	@param [in] element Ԫ��
	@return �����Ƿ����ƻ�����
	*/
	bool push_back(const Type& element);

	/** ����ͷ���
	@param [in] element Ԫ��
	@return �����Ƿ����ƻ�����
	*/
	bool push_front(const Type& element);

	/** ׷�ӣ��������Ԫ��
	@param [in] element Ԫ��
	@return �����Ƿ����ƻ�����
	*/
	bool emplace_back(const Type& element);

	/** ����ͷ��ӣ��������Ԫ��
	@param [in] element Ԫ��
	@return �����Ƿ����ƻ�����
	*/
	bool emplace_front(const Type& element);

protected:
	//���Ƹ���
	size_t m_finite;
	//��������
	FiniteType m_finiteType;
};

#include "FiniteDeque.inl"