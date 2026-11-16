#pragma once

inline LockFreeHashMapPerfValidationValue::LockFreeHashMapPerfValidationValue(int32_t value, size_t& copies) :
m_value(value),
m_copies(&copies)
{

}

inline LockFreeHashMapPerfValidationValue::LockFreeHashMapPerfValidationValue(const LockFreeHashMapPerfValidationValue& other) :
m_value(other.m_value),
m_copies(other.m_copies)
{
    ++*m_copies;
}

inline LockFreeHashMapPerfValidationValue& LockFreeHashMapPerfValidationValue::operator=(const LockFreeHashMapPerfValidationValue& other)
{
    m_value = other.m_value;
    m_copies = other.m_copies;
    ++*m_copies;
    return *this;
}

inline bool LockFreeHashMapPerfValidationValue::operator<(const LockFreeHashMapPerfValidationValue& other) const
{
    return m_value < other.m_value;
}

inline bool LockFreeHashMapPerfValidationValue::operator==(const LockFreeHashMapPerfValidationValue& other) const
{
    return m_value == other.m_value;
}

template <typename V>
bool LockFreeHashMapPerfValidation::less(const V& left, const V& right)
{
    return std::less<V>()(left, right);
}

inline bool LockFreeHashMapPerfValidation::less(const LockFreeHashMapPerfLargeValue& left, const LockFreeHashMapPerfLargeValue& right)
{
    return std::lexicographical_compare(left.m_words, left.m_words + 64, right.m_words, right.m_words + 64);
}

template <typename K, typename V>
bool LockFreeHashMapPerfValidation::less(const std::pair<K, V>& left, const std::pair<K, V>& right)
{
    return less(left.first, right.first) || (!less(right.first, left.first) && less(left.second, right.second));
}

template <typename T>
bool LockFreeHashMapPerfValidation::prepare(std::vector<T>& expected, std::vector<const T*>& scratch)
{
    try
    {
        // 参数为独立预期值；仅改变预期的排列，不读取被测表或实际结果。
        auto compare = [](const T& left, const T& right) -> bool
        {
            return less(left, right);
        };
        std::sort(expected.begin(), expected.end(), compare);
        scratch.assign(expected.size(), nullptr);
        return true;
    }
    catch (...)
    {
        scratch.clear();
        return false;
    }
}

template <typename T>
bool LockFreeHashMapPerfValidation::prepared(const std::vector<T>& actual, const std::vector<T>& expected,
    std::vector<const T*>& scratch)
{
    if (actual.size() != expected.size() || scratch.size() != expected.size())
    {
        std::fill(scratch.begin(), scratch.end(), nullptr);
        return false;
    }
    bool valid = true;
    try
    {
        for (size_t i = 0; i < actual.size(); ++i)
        {
            scratch[i] = std::addressof(actual[i]);
        }
        // 只排序已分配数组中的借用指针，不复制shared_ptr，不改变共享计数或分配器状态。
        auto compare = [](const T* left, const T* right) -> bool
        {
            return less(*left, *right);
        };
        std::sort(scratch.begin(), scratch.end(), compare);
        for (size_t i = 0; valid && i < expected.size(); ++i)
        {
            valid = *scratch[i] == expected[i];
        }
    }
    catch (...)
    {
        valid = false;
    }
    // 输出随后可能立即销毁；本轮借用不跨越校验函数返回。
    std::fill(scratch.begin(), scratch.end(), nullptr);
    return valid;
}

template <typename K>
bool LockFreeHashMapPerfValidation::keys(const std::vector<K>& actual, const std::vector<K>& expected, size_t count)
{
    if (actual.size() != count || expected.size() < count)
    {
        return false;
    }
    std::vector<K> sorted(actual), reference(expected.begin(), expected.begin() + count);
    std::sort(sorted.begin(), sorted.end());
    std::sort(reference.begin(), reference.end());
    return sorted == reference;
}

template <typename V>
bool LockFreeHashMapPerfValidation::values(const std::vector<V>& actual, const std::vector<V>& candidates, size_t count)
{
    if (actual.size() != count || candidates.size() / 2 < count)
    {
        return false;
    }
    std::vector<V> sorted(actual), reference;
    reference.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        reference.push_back(candidates[i * 2]);
    }
    // 比较器只借用测试值，不调用被测 Map，不把验证工作计入吞吐。
    auto compare = [](const V& left, const V& right) -> bool
    {
        return less(left, right);
    };
    std::sort(sorted.begin(), sorted.end(), compare);
    std::sort(reference.begin(), reference.end(), compare);
    return sorted == reference;
}

template <typename K, typename V>
bool LockFreeHashMapPerfValidation::pairs(const std::vector<std::pair<K, V>>& actual, const std::vector<K>& keys,
    const std::vector<V>& candidates, size_t count)
{
    if (actual.size() != count || keys.size() < count || candidates.size() / 2 < count)
    {
        return false;
    }
    std::map<K, V> reference;
    for (size_t i = 0; i < count; ++i)
    {
        if (!reference.insert(std::make_pair(keys[i], candidates[i * 2])).second)
        {
            return false;
        }
    }
    for (size_t i = 0; i < actual.size(); ++i)
    {
        typename std::map<K, V>::iterator found = reference.find(actual[i].first);
        if (found == reference.end() || !(found->second == actual[i].second))
        {
            return false;
        }
        reference.erase(found);
    }
    return reference.empty();
}

inline bool LockFreeHashMapPerfValidation::preparedSelftest()
{
    size_t copies = 0;
    typedef LockFreeHashMapPerfValidationValue Probe;
    std::vector<Probe> probeExpected, probeActual;
    probeExpected.emplace_back(2, copies);
    probeExpected.emplace_back(1, copies);
    probeActual = probeExpected;
    std::vector<const Probe*> probeScratch;
    if (!prepare(probeExpected, probeScratch))
    {
        return false;
    }
    const size_t before = copies;
    if (!prepared(probeActual, probeExpected, probeScratch) || copies != before)
    {
        return false;
    }
    const int32_t raw[] = { 2, 1, 1 };
    std::vector<int32_t> expected(raw, raw + 3), actual(expected);
    std::vector<const int32_t*> scratch;
    if (!prepare(expected, scratch) || !prepared(actual, expected, scratch))
    {
        return false;
    }
    actual[2] = 2;
    if (prepared(actual, expected, scratch))
    {
        return false;
    }
    actual.pop_back();
    if (prepared(actual, expected, scratch))
    {
        return false;
    }
    actual.assign(raw, raw + 3);
    actual.push_back(1);
    if (prepared(actual, expected, scratch))
    {
        return false;
    }
    actual.assign(raw, raw + 3);
    if (!prepared(actual, expected, scratch))
    {
        return false;
    }
    std::vector<std::pair<int32_t, int32_t>> pairsExpected, pairsActual;
    pairsExpected.push_back(std::make_pair(1, 11));
    pairsExpected.push_back(std::make_pair(2, 22));
    pairsActual = pairsExpected;
    std::reverse(pairsActual.begin(), pairsActual.end());
    std::vector<const std::pair<int32_t, int32_t>*> pairScratch;
    if (!prepare(pairsExpected, pairScratch) || !prepared(pairsActual, pairsExpected, pairScratch))
    {
        return false;
    }
    std::swap(pairsActual[0].second, pairsActual[1].second);
    if (prepared(pairsActual, pairsExpected, pairScratch))
    {
        return false;
    }
    std::vector<LockFreeHashMapPerfLargeValue> wideExpected(1, LockFreeHashMapPerfLargeValue(7)), wideActual(wideExpected);
    std::vector<const LockFreeHashMapPerfLargeValue*> wideScratch;
    if (!prepare(wideExpected, wideScratch) || !prepared(wideActual, wideExpected, wideScratch))
    {
        return false;
    }
    wideActual[0].m_words[63] ^= 1;
    if (prepared(wideActual, wideExpected, wideScratch))
    {
        return false;
    }
    typedef std::shared_ptr<const int32_t> Shared;
    Shared owner = std::make_shared<const int32_t>(31);
    std::vector<Shared> sharedExpected(2, owner), sharedActual(sharedExpected);
    std::vector<const Shared*> sharedScratch;
    if (!prepare(sharedExpected, sharedScratch))
    {
        return false;
    }
    const long references = owner.use_count();
    if (!prepared(sharedActual, sharedExpected, sharedScratch) || owner.use_count() != references ||
        sharedScratch[0] != nullptr || sharedScratch[1] != nullptr)
    {
        return false;
    }
    sharedActual[0].reset();
    if (prepared(sharedActual, sharedExpected, sharedScratch) || owner.use_count() != references - 1)
    {
        return false;
    }
    std::vector<int32_t> empty;
    return prepare(empty, scratch) && prepared(empty, empty, scratch);
}

inline bool LockFreeHashMapPerfValidation::selftest()
{
    if (!preparedSelftest())
    {
        return false;
    }
    const int32_t rawKeys[] = { 1, 2, 3 };
    const int32_t rawCandidates[] = { 10, 11, 20, 21, 30, 31 };
    std::vector<int32_t> expected(rawKeys, rawKeys + 3), candidates(rawCandidates, rawCandidates + 6);
    std::vector<int32_t> actualKeys(expected), actualValues;
    std::vector<std::pair<int32_t, int32_t>> actualPairs;
    for (size_t i = 0; i < 3; ++i)
    {
        actualValues.push_back(candidates[i * 2]);
        actualPairs.push_back(std::make_pair(expected[i], candidates[i * 2]));
    }
    std::reverse(actualKeys.begin(), actualKeys.end());
    std::reverse(actualValues.begin(), actualValues.end());
    std::reverse(actualPairs.begin(), actualPairs.end());
    if (!keys(actualKeys, expected, 3) || !values(actualValues, candidates, 3) || !pairs(actualPairs, expected, candidates, 3))
    {
        return false;
    }
    actualKeys[0] = actualKeys[1];
    actualValues[0] = actualValues[1];
    std::swap(actualPairs[0].second, actualPairs[1].second);
    if (keys(actualKeys, expected, 3) || values(actualValues, candidates, 3) || pairs(actualPairs, expected, candidates, 3))
    {
        return false;
    }
    actualPairs[0] = actualPairs[1];
    if (pairs(actualPairs, expected, candidates, 3))
    {
        return false;
    }
    std::vector<LockFreeHashMapPerfLargeValue> largeCandidates(2, LockFreeHashMapPerfLargeValue(7));
    std::vector<LockFreeHashMapPerfLargeValue> large(1, largeCandidates[0]);
    if (!values(large, largeCandidates, 1))
    {
        return false;
    }
    large[0].m_words[63] ^= 1;
    return !values(large, largeCandidates, 1) && !keys(std::vector<int32_t>(), expected, 3);
}