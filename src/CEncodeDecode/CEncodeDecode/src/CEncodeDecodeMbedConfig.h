#pragma once

// 必须与tools中预编译mbedTLS使用同一配置，不能采用第三方默认ABI
#ifndef MBEDTLS_CONFIG_FILE
#define MBEDTLS_CONFIG_FILE "mbedtls_sdk_config.h"
#endif

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "mbedtlsd.lib")
#else
#pragma comment(lib, "mbedtls.lib")
#endif
#endif