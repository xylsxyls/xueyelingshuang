#ifndef _FINITE_DEQUE_H__
#define _FINITE_DEQUE_H__
#include "FiniteDeque.h"

template <class Type>
FiniteDeque<Type>::FiniteDeque() :
m_finite(0)
{

}

template <class Type>
void FiniteDeque<Type>::setFinite(size_t num, FiniteType finiteType)
{
	m_finite = num;
	m_finiteType = finiteType;
}

template <class Type>
bool FiniteDeque<Type>::push_back(const Type& element)
{
	bool result = false;
	switch (m_finiteType)
	{
	case FINITE:
	{
		if (FiniteDeque<Type>::size() >= m_finite)
		{
			result = true;
			break;
		}
		std::deque<Type>::push_back(element);
	}
	break;
	case FLOW:
	{
		while (FiniteDeque<Type>::size() >= m_finite)
		{
			std::deque<Type>::pop_front();
			result = true;
		}
		std::deque<Type>::push_back(element);
	}
	break;
	default:
		break;
	}
	return result;
}

template <class Type>
bool FiniteDeque<Type>::push_front(const Type& element)
{
	bool result = false;
	switch (m_finiteType)
	{
	case FINITE:
	{
		if (FiniteDeque<Type>::size() >= m_finite)
		{
			result = true;
			break;
		}
		std::deque<Type>::push_front(element);
	}
	break;
	case FLOW:
	{
		while (FiniteDeque<Type>::size() >= m_finite)
		{
			std::deque<Type>::pop_back();
			result = true;
		}
		std::deque<Type>::push_front(element);
	}
	break;
	default:
		break;
	}
	return result;
}

template <class Type>
bool FiniteDeque<Type>::emplace_back(const Type& element)
{
	bool result = false;
	switch (m_finiteType)
	{
	case FINITE:
	{
		if (FiniteDeque<Type>::size() >= m_finite)
		{
			result = true;
			break;
		}
		std::deque<Type>::emplace_back(element);
	}
	break;
	case FLOW:
	{
		while (FiniteDeque<Type>::size() >= m_finite)
		{
			std::deque<Type>::pop_front();
			result = true;
		}
		std::deque<Type>::emplace_back(element);
	}
	break;
	default:
		break;
	}
	return result;
}

template <class Type>
bool FiniteDeque<Type>::emplace_front(const Type& element)
{
	bool result = false;
	switch (m_finiteType)
	{
	case FINITE:
	{
		if (FiniteDeque<Type>::size() >= m_finite)
		{
			result = true;
			break;
		}
		std::deque<Type>::emplace_front(element);
	}
	break;
	case FLOW:
	{
		while (FiniteDeque<Type>::size() >= m_finite)
		{
			std::deque<Type>::pop_back();
			result = true;
		}
		std::deque<Type>::emplace_front(element);
	}
	break;
	default:
		break;
	}
	return result;
}

#endif