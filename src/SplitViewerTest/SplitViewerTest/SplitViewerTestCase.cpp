#include "SplitViewerTestCase.h"

SplitViewerTestCase::SplitViewerTestCase(const std::wstring& name, SplitViewerTestCaseType type,
    const std::wstring& scene, const std::wstring& expected, int32_t timeoutMs) :
m_name(name),
m_type(type),
m_scene(scene),
m_expected(expected),
m_timeoutMs(timeoutMs)
{

}