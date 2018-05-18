#ifndef __CSYSTEM_H__
#define __CSYSTEM_H__

#include "CSystem.h"

template <typename TypeClass>
TypeClass** CSystem::CreateDyadicArray(qint32 row, qint32 column)
{
    TypeClass** dyadicArrayPtr = new TypeClass*[row];
    if (dyadicArrayPtr != nullptr)
    {
        for (qint32 index = 0; index < row; ++index)
        {
            dyadicArrayPtr[index] = new TypeClass[column];
            if (dyadicArrayPtr[index] == nullptr)
            {
                for (qint32 errorIndex = 0; errorIndex < index; ++errorIndex)
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
void CSystem::DestroyDyadicArray(TypeClass** classPtr, qint32 row)
{
    if (classPtr == nullptr)
    {
        return;
    }

    for (qint32 index = 0; index < row; index++)
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