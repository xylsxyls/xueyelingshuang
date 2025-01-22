#pragma once
#include <stdint.h>
#include <deque>
#include <stddef.h>

/** 有限队列
*/
template <class Type>
class FiniteDeque : public std::deque < Type >
{
public:
	enum FiniteType
	{
		/** 当队列满了之后后续添加失败
		*/
		FINITE,

		/** 当队列满了之后后续添加顶掉最早添加的
		*/
		FLOW
	};

public:
	/** 构造函数
	*/
	FiniteDeque();

public:
	/** 设置有限个数
	@param [in] num 个数
	@param [in] finiteType 限制类型
	*/
	void setFinite(size_t num, FiniteType finiteType = FLOW);

	/** 获取限制个数
	@return 返回限制个数
	*/
	size_t finite();

	/** 追加
	@param [in] element 元素
	@return 返回是否被限制或流动
	*/
	bool push_back(const Type& element);

	/** 队列头添加
	@param [in] element 元素
	@return 返回是否被限制或流动
	*/
	bool push_front(const Type& element);

	/** 追加，消耗添加元素
	@param [in] element 元素
	@return 返回是否被限制或流动
	*/
	bool emplace_back(const Type& element);

	/** 队列头添加，消耗添加元素
	@param [in] element 元素
	@return 返回是否被限制或流动
	*/
	bool emplace_front(const Type& element);

protected:
	//限制个数
	size_t m_finite;
	//限制类型
	FiniteType m_finiteType;
};

#include "FiniteDeque.inl"