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
	@param [in] ptr 8�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int64_t selfAddOne(int64_t* ptr);

	/** �Լ�
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int32_t selfSubOne(int32_t* ptr);

	/** �Լ�
	@param [in] ptr 8�ֽ��ڴ��ַ���������ڹ����ڴ�
	@return �����Լ�֮���ֵ
	*/
	static int64_t selfSubOne(int64_t* ptr);

	/** ��
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int32_t selfAdd(int32_t* ptr, int32_t value);

	/** ��
	@param [in] ptr 8�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int64_t selfAdd(int64_t* ptr, int64_t value);

	/** ��
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int32_t selfSub(int32_t* ptr, int32_t value);

	/** ��
	@param [in] ptr 8�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] value ����ֵ
	@return ���ؼ�֮���ֵ
	*/
	static int64_t selfSub(int64_t* ptr, int64_t value);

	/** �Ƚ�֮�󽻻�
	@param [in] ptr 4�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] oldValue �Ƿ���ڴ���ԭ����ֵһ��
	@param [in] newValue ���һ�����޸�Ϊ�µ�ֵ
	@return �ɹ�����ֵ����true
	*/
	static bool compareAndSwap(int32_t* ptr, int32_t oldValue, int32_t newValue);

	/** �Ƚ�֮�󽻻�
	@param [in] ptr 8�ֽ��ڴ��ַ���������ڹ����ڴ�
	@param [in] oldValue �Ƿ���ڴ���ԭ����ֵһ��
	@param [in] newValue ���һ�����޸�Ϊ�µ�ֵ
	@return �ɹ�����ֵ����true
	*/
	static bool compareAndSwap(int64_t* ptr, int64_t oldValue, int64_t newValue);
};