#ifndef __CSYSTEM_H__
#define __CSYSTEM_H__

template <typename TypeClass>
TypeClass** CSystem::CreateDyadicArray(int32_t row, int32_t column)
{
    if (row <= 0 || column <= 0)
    {
        return nullptr;
    }

    TypeClass** dyadicArrayPtr = new (std::nothrow) TypeClass*[row];
    if (dyadicArrayPtr == nullptr)
    {
        return nullptr;
    }
    int32_t built = 0;
    try
    {
        for (; built < row; ++built)
        {
            dyadicArrayPtr[built] = new (std::nothrow) TypeClass[column];
            if (dyadicArrayPtr[built] == nullptr)
            {
                DestroyDyadicArray(dyadicArrayPtr, built);
                return nullptr;
            }
        }
    }
    catch (...)
    {
        DestroyDyadicArray(dyadicArrayPtr, built);
        throw;
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
    }
    delete[] classPtr;
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