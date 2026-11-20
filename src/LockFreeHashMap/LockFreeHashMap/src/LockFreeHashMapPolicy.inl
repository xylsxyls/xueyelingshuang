#pragma once

template <typename Target>
inline bool LockFreeHashMapNullCallback(Target* function)
{
    return function == nullptr;
}

template <typename F>
inline bool LockFreeHashMapNullCallback(const F& function)
{
    (void)function;
    return false;
}

template <typename F>
inline bool LockFreeHashMapNullCallback(const std::reference_wrapper<F>& function)
{
    return LockFreeHashMapNullCallback(function.get());
}

template <typename R, typename... C>
LockFreeHashMapPolicyFunction<R, C...>::~LockFreeHashMapPolicyFunction()
{

}

template <typename R, typename F, typename... C>
LockFreeHashMapPolicyHolder<R, F, C...>::LockFreeHashMapPolicyHolder(const F& function) :
m_function(function)
{

}

template <typename R, typename F, typename... C>
R LockFreeHashMapPolicyHolder<R, F, C...>::invoke(const C&... context)
{
    if (LockFreeHashMapNullCallback(m_function))
    {
        throw std::bad_function_call();
    }
    return m_function(context...);
}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy()
{

}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy(std::nullptr_t function)
{
    (void)function;
}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy(const LockFreeHashMapPolicy& other) :
m_function(other.m_function)
{

}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>& LockFreeHashMapPolicy<R, C...>::operator=(const LockFreeHashMapPolicy& other)
{
    m_function = other.m_function;
    return *this;
}

template <typename R, typename... C>
template <typename F>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy(const F& function)
{
    if (!LockFreeHashMapNullCallback(function))
    {
        m_function = std::make_shared<LockFreeHashMapPolicyHolder<R, typename std::decay<F>::type, C...>>(function);
    }
}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy(R (*function)(const C&... context))
{
    if (function != nullptr)
    {
        m_function = std::make_shared<LockFreeHashMapPolicyHolder<R, R (*)(const C&...), C...>>(function);
    }
}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::LockFreeHashMapPolicy(R (*function)(C... context))
{
    if (function != nullptr)
    {
        m_function = std::make_shared<LockFreeHashMapPolicyHolder<R, R (*)(C...), C...>>(function);
    }
}

template <typename R, typename... C>
template <typename F>
LockFreeHashMapPolicy<R, C...> LockFreeHashMapPolicy<R, C...>::fromShared(const std::shared_ptr<F>& function)
{
    if (!function)
    {
        return LockFreeHashMapPolicy();
    }
    /** 通过捕获的共享句柄保活并调用原回调，不复制回调目标
    @param [in] context 仅在调用期间借用的策略上下文或通知参数
    @return 非void类型透传结果；void通知不返回结果，执行异常交由外层处理
    */
    return LockFreeHashMapPolicy([function](const C&... context) -> R
    {
        if (LockFreeHashMapNullCallback(*function))
        {
            throw std::bad_function_call();
        }
        return (*function)(context...);
    });
}

template <typename R, typename... C>
LockFreeHashMapPolicy<R, C...>::operator bool() const
{
    return static_cast<bool>(m_function);
}

template <typename R, typename... C>
R LockFreeHashMapPolicy<R, C...>::operator()(const C&... context) const
{
    if (!m_function)
    {
        throw std::bad_function_call();
    }
    return m_function->invoke(context...);
}

template <typename R, typename... C>
void LockFreeHashMapPolicy<R, C...>::swap(LockFreeHashMapPolicy& other)
{
    m_function.swap(other.m_function);
}