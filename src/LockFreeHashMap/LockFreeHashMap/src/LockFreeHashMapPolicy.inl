#pragma once

template <typename R, typename C>
LockFreeHashMapPolicyFunction<R, C>::~LockFreeHashMapPolicyFunction()
{

}

template <typename R, typename C, typename F>
LockFreeHashMapPolicyHolder<R, C, F>::LockFreeHashMapPolicyHolder(const F& function) :
m_function(function)
{

}

template <typename R, typename C, typename F>
R LockFreeHashMapPolicyHolder<R, C, F>::invoke(const C& context)
{
    return m_function(context);
}

template <typename R, typename C>
LockFreeHashMapPolicy<R, C>::LockFreeHashMapPolicy()
{

}

template <typename R, typename C>
LockFreeHashMapPolicy<R, C>::LockFreeHashMapPolicy(const LockFreeHashMapPolicy& other) :
m_function(other.m_function)
{

}

template <typename R, typename C>
LockFreeHashMapPolicy<R, C>& LockFreeHashMapPolicy<R, C>::operator=(const LockFreeHashMapPolicy& other)
{
    m_function = other.m_function;
    return *this;
}

template <typename R, typename C>
template <typename F>
LockFreeHashMapPolicy<R, C>::LockFreeHashMapPolicy(const F& function) :
m_function(std::make_shared<LockFreeHashMapPolicyHolder<R, C, typename std::decay<F>::type>>(function))
{

}

template <typename R, typename C>
LockFreeHashMapPolicy<R, C>::LockFreeHashMapPolicy(R (*function)(const C& context))
{
    if (function != nullptr)
    {
        m_function = std::make_shared<LockFreeHashMapPolicyHolder<R, C, R (*)(const C&)>>(function);
    }
}

template <typename R, typename C>
template <typename F>
LockFreeHashMapPolicy<R, C> LockFreeHashMapPolicy<R, C>::fromShared(const std::shared_ptr<F>& function)
{
    if (!function)
    {
        return LockFreeHashMapPolicy();
    }
    // context 只在调用期间借用；共享句柄保证目标存活，返回目标策略的请求
    return LockFreeHashMapPolicy([function](const C& context) -> R
    {
        return (*function)(context);
    });
}

template <typename R, typename C>
LockFreeHashMapPolicy<R, C>::operator bool() const
{
    return static_cast<bool>(m_function);
}

template <typename R, typename C>
R LockFreeHashMapPolicy<R, C>::operator()(const C& context) const
{
    if (!m_function)
    {
        throw std::bad_function_call();
    }
    return m_function->invoke(context);
}

template <typename R, typename C>
void LockFreeHashMapPolicy<R, C>::swap(LockFreeHashMapPolicy& other)
{
    m_function.swap(other.m_function);
}