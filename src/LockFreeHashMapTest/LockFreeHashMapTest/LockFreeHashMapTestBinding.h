#pragma once

/** 仅测试工程显式实例化成员指针，绑定过程不分配、不抛异常
C++11 显式实例化声明的模板实参不执行通常的访问检查；不修改产品定义、布局或访问级别
*/
template <typename PointerType, PointerType Member, PointerType* Destination>
class LockFreeHashMapTestBinding
{
public:
    /** 在测试模块初始化时登记编译器生成的成员指针
    */
    LockFreeHashMapTestBinding();
private:
    // 显式实例化产生的初始化对象，与接收成员指针的静态字段处于同一翻译单元
    static LockFreeHashMapTestBinding s_binding;
};

#include "LockFreeHashMapTestBinding.inl"