#pragma once

// DLL 不导出内部测试入口；内部状态白盒测试由正式静态库承载
#if defined(_MSC_VER) && !defined(STATIC_LIB) && !defined(CTTM_TEST_DLL)
#define CTTM_TEST_DLL
#endif