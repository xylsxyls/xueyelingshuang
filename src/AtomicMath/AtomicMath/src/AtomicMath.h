#pragma once
#include "AtomicMathMacro.h"
#include <stdint.h>

/** ԭ�Ӽ�����
*/
class AtomicMathAPI AtomicMath
{
public:
	/** �Լ�
	@param [in] 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int32_t selfAddOne(int32_t* ptr);

	/** �Լ�
	@param [in] 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int32_t selfSubOne(int32_t* ptr);
};