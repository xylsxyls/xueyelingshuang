#pragma once
#include "AtomicMathMacro.h"
#include <stdint.h>

/** ԭ�Ӽ�����
*/
class AtomicMathAPI AtomicMath
{
public:
	/** �Լ�
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int32_t selfAddOne(int32_t* ptr);

	/** �Լ�
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int32_t selfSubOne(int32_t* ptr);

	/** ��
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int32_t selfAdd(int32_t* ptr, int32_t value);

	/** ��
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int32_t selfSub(int32_t* ptr, int32_t value);
};