#ifndef __CSYSTEM_H__
#define __CSYSTEM_H__

#include "CSystem.h"

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

//template <typename TypeClass>
//std::string className()
//{
//    std::string name = typeid(TypeClass).name();
//    CStringManager::Replace(name, "class ", "");
//    return name;
//}

#endif