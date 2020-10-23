#ifndef __CSYSTEM_H__
#define __CSYSTEM_H__

template <typename TypeClass>
TypeClass** CSystem::CreateDyadicArray(int32_t row, int32_t column)
{
	TypeClass** dyadicArrayPtr = new TypeClass*[row];
	if (dyadicArrayPtr != nullptr)
	{
		for (int32_t index = 0; index < row; ++index)
		{
			dyadicArrayPtr[index] = new TypeClass[column];
			if (dyadicArrayPtr[index] == nullptr)
			{
				for (int32_t errorIndex = 0; errorIndex < index; ++errorIndex)
				{
					delete[] dyadicArrayPtr[errorIndex];
					dyadicArrayPtr[errorIndex] = nullptr;
				}
				dyadicArrayPtr = nullptr;
				break;
			}
		}
	}
	return dyadicArrayPtr;
}

template <typename TypeClass>
void CSystem::DestroyDyadicArray(TypeClass** classPtr, int32_t row)
{
	if (classPtr == nullptr)
	{
		return;
	}

	for (int32_t index = 0; index < row; index++)
	{
		delete[] classPtr[index];
		classPtr[index] = nullptr;
	}
	delete[] classPtr;
	classPtr = nullptr;
}

template <typename TRECT>
TRECT CSystem::rectValid(const TRECT& rect)
{
	TRECT result = rect;
	if (result.left < 0)
	{
		result.left = 0;
	}
	if (result.top < 0)
	{
		result.top = 0;
	}
	if (result.right < 0)
	{
		result.right = 0;
	}
	if (result.bottom < 0)
	{
		result.bottom = 0;
	}
	return result;
}

template <typename TQRect>
TQRect CSystem::qrectValid(const TQRect& rect)
{
	TQRect result = rect;
	if (result.left() < 0)
	{
		result.setLeft(0);
	}
	if (result.top() < 0)
	{
		result.setTop(0);
	}
	if (result.right() < 0)
	{
		result.setRight(0);
	}
	if (result.bottom() < 0)
	{
		result.setBottom(0);
	}
	return result;
}

#endif