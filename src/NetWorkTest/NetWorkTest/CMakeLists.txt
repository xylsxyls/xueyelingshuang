cmake_minimum_required(VERSION 3.15)
if(BIT MATCHES "^wasm" OR DLLLIB STREQUAL "wasm" OR EMSCRIPTEN OR CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten")
    message(FATAL_ERROR "Native desktop executable; build NetPeer for Wasm")
endif()

# version prepares dependencies; this CMake entry only builds the current executable targets.
if(NOT BIT MATCHES "^(32|64|arm32|arm64)$")
    message(FATAL_ERROR "Set BIT to 32, 64, arm32 or arm64")
endif()
if(DEBUGRELEASE STREQUAL "debug")
    set(native_configuration Debug)
elseif(DEBUGRELEASE STREQUAL "release")
    set(native_configuration Release)
else()
    message(FATAL_ERROR "Set DEBUGRELEASE to debug or release")
endif()
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL native_configuration)
    message(FATAL_ERROR "CMAKE_BUILD_TYPE disagrees with DEBUGRELEASE; use a separate build directory")
endif()
set(CMAKE_BUILD_TYPE "${native_configuration}" CACHE STRING "Prepared SDK configuration" FORCE)
# Xcode also uses the one configuration of the separately prepared dependency SDK.
set(CMAKE_CONFIGURATION_TYPES "${native_configuration}" CACHE STRING "Prepared SDK configuration" FORCE)

# Explicit NDK/Apple/Linux toolchains own their compiler, sysroot and target flags.
if(CMAKE_TOOLCHAIN_FILE OR CMAKE_SYSTEM_NAME STREQUAL "Android")
elseif(CMAKE_HOST_APPLE OR CMAKE_SYSTEM_NAME MATCHES "^(Darwin|iOS)$")
    if(BIT STREQUAL "64")
        set(native_apple_arch x86_64)
    elseif(BIT STREQUAL "arm64")
        set(native_apple_arch arm64)
    else()
        message(FATAL_ERROR "Apple targets require BIT=64 or arm64")
    endif()
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "${native_apple_arch}" CACHE STRING "Apple target architecture")
    endif()
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" native_host_arch)
    if(BIT STREQUAL "32" OR BIT STREQUAL "64")
        if(NOT native_host_arch MATCHES "^(x86_64|amd64|i[3-6]86|x86)$")
            message(FATAL_ERROR "An x86 cross toolchain is required on this host")
        endif()
        string(APPEND CMAKE_C_FLAGS " -m${BIT}")
        string(APPEND CMAKE_CXX_FLAGS " -m${BIT}")
    elseif(BIT STREQUAL "arm32" AND NOT native_host_arch MATCHES "^arm(v[5-8].*)?$")
        set(CMAKE_SYSTEM_NAME Linux)
        set(CMAKE_SYSTEM_PROCESSOR arm)
        if(NOT CMAKE_C_COMPILER)
            set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
        endif()
        if(NOT CMAKE_CXX_COMPILER)
            set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
        endif()
    elseif(BIT STREQUAL "arm64" AND NOT native_host_arch MATCHES "^(aarch64|arm64)$")
        set(CMAKE_SYSTEM_NAME Linux)
        set(CMAKE_SYSTEM_PROCESSOR aarch64)
        if(NOT CMAKE_C_COMPILER)
            set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
        endif()
        if(NOT CMAKE_CXX_COMPILER)
            set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
        endif()
    endif()
elseif(CMAKE_HOST_WIN32)
    message(FATAL_ERROR "Windows uses before + rebuild or Visual Studio; native CMake is for other target platforms")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT DEFINED CMAKE_TRY_COMPILE_TARGET_TYPE)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
endif()
project(NetWorkTest LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Android|Darwin|iOS)$")
    message(FATAL_ERROR "Unsupported native CMake target; Windows uses rebuild or Visual Studio")
endif()
if(DEFINED EXPECTED_TARGET_SYSTEM AND NOT CMAKE_SYSTEM_NAME STREQUAL EXPECTED_TARGET_SYSTEM)
    message(FATAL_ERROR "The toolchain target disagrees with EXPECTED_TARGET_SYSTEM")
endif()
if(BIT STREQUAL "32" OR BIT STREQUAL "arm32")
    set(native_pointer_bytes 4)
else()
    set(native_pointer_bytes 8)
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL native_pointer_bytes)
    message(FATAL_ERROR "BIT disagrees with the actual compiler pointer size")
endif()
if(APPLE)
    if(BIT STREQUAL "arm64")
        set(native_apple_arch arm64)
    elseif(BIT STREQUAL "64")
        set(native_apple_arch x86_64)
    else()
        message(FATAL_ERROR "Apple targets require BIT=64 or arm64")
    endif()
    if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL native_apple_arch)
        message(FATAL_ERROR "BIT disagrees with CMAKE_OSX_ARCHITECTURES; prepare each ABI separately")
    endif()
endif()
if(ANDROID)
    if(BIT STREQUAL "32")
        set(native_android_abi x86)
    elseif(BIT STREQUAL "64")
        set(native_android_abi x86_64)
    elseif(BIT STREQUAL "arm32")
        set(native_android_abi armeabi-v7a)
    else()
        set(native_android_abi arm64-v8a)
    endif()
    set(native_resolved_abi "${CMAKE_ANDROID_ARCH_ABI}")
    if(NOT native_resolved_abi)
        set(native_resolved_abi "${ANDROID_ABI}")
    endif()
    if(NOT native_resolved_abi STREQUAL native_android_abi)
        message(FATAL_ERROR "BIT disagrees with the selected Android NDK ABI")
    endif()
endif()
# Equal pointer widths do not distinguish x86_64 from arm64: compile an ABI assertion.
if(BIT STREQUAL "32")
    set(native_arch_condition "defined(__i386__) || defined(_M_IX86)")
elseif(BIT STREQUAL "64")
    set(native_arch_condition "defined(__x86_64__) || defined(_M_X64)")
elseif(BIT STREQUAL "arm32")
    set(native_arch_condition "defined(__arm__) || defined(_M_ARM)")
else()
    set(native_arch_condition "defined(__aarch64__) || defined(_M_ARM64)")
endif()
include(CheckCXXSourceCompiles)
set(native_saved_try_compile "${CMAKE_TRY_COMPILE_TARGET_TYPE}")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
unset(NATIVE_REQUESTED_ABI_MATCHES CACHE)
check_cxx_source_compiles("#if !(${native_arch_condition})\n#error Wrong architecture\n#endif\nstatic_assert(sizeof(void*) == ${native_pointer_bytes}, \"Wrong pointer size\");\nint nativeAbiCheck() { return 0; }" NATIVE_REQUESTED_ABI_MATCHES)
if(native_saved_try_compile)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE "${native_saved_try_compile}")
else()
    unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
endif()
if(NOT NATIVE_REQUESTED_ABI_MATCHES)
    message(FATAL_ERROR "BIT disagrees with the actual compiler architecture")
endif()

get_filename_component(native_repository "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(native_default_sdk "${native_repository}")
if(DEFINED ENV{CLOUD_REBUILD_SDK_ROOT} AND NOT "$ENV{CLOUD_REBUILD_SDK_ROOT}" STREQUAL "")
    set(native_default_sdk "$ENV{CLOUD_REBUILD_SDK_ROOT}")
endif()
set(SDK_ROOT "${native_default_sdk}" CACHE PATH "Target SDK prepared by before/version")
get_filename_component(SDK_ROOT "${SDK_ROOT}" ABSOLUTE)

if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "Desktop gateway processes and IPC NetSender are not mobile targets; use NetWork or NetPeer")
endif()

add_executable(NetWorkTest
    "NetWorkTest.cpp"
    "NetWorkTestCases.cpp"
    "NetInterfaceTlsTestCases.cpp"
    "NetInterfaceCppProbe.cpp"
    "NetSenderProviderCProbe.c"
    "NetInterfaceProtoTestCases.cpp"
    "NetInterfaceNativeTestCases.cpp"
    "NetInterfaceTcpProbe.cpp"
    "NetInterfaceTransportProbe.cpp"
    "NetInterfaceConnectionTestCases.cpp"
    "NetInterfaceIpcTestCases.cpp"
    "NetInterfaceHandshakeTestCases.cpp"
    "NetInterfaceRequestsTestCases.cpp"
    "NetInterfaceRpcTestCases.cpp"
    "NetServerManagerTestIncludes.h"
    "NetGatewayTestCases.cpp"
    "../../NetServerManager/NetServerManager/NetGatewayServer.cpp"
    "NetLocalTestCases.cpp"
    "NetServiceTestCases.cpp"
    "../../NetServerManager/NetServerManager/NetGatewayLocalServer.cpp"
    "../../NetServerManager/NetServerManager/NetGatewayFiles.cpp"
    "NetAccountGatewayProbe.cpp"
    "NetFileGatewayProbe.cpp"
    "NetIdentityTestCases.cpp"
    "NetTunnelTestCases.cpp"
    "NetRelayTransportProbe.cpp"
    "NetLinkTestCases.cpp"
    "NetRawLocalTestCases.cpp"
    "NetLinkLocalTestCases.cpp"
    "NetGatewayClientTestProbe.cpp"
    "NetEndpointRouterTestCases.cpp"
    "NetWorkInProcessTestCases.cpp"
    "NetSenderFacadeNetSenderCppTest.cpp"
    "NetCppBoundaryProbe.cpp"
    "NetSenderFacadeNetPeerCppTest.cpp"
    "NetSenderFacadeNetPeerCTest.cpp"
    "NetSenderFacadeNetWorkCppTest.cpp"
    "NetSenderFacadeNetWorkCTest.cpp"
    "NetSenderFacadeConcurrency.cpp"
    "NetWorkConcurrencyTestCases.cpp"
    "../../NetServerManager/NetServerManager/NetServerEnrollment.cpp"
)
set_target_properties(NetWorkTest PROPERTIES OUTPUT_NAME NetWorkTest1.0 RUNTIME_OUTPUT_DIRECTORY "${SDK_ROOT}/lib")
foreach(native_config DEBUG RELEASE)
    set_target_properties(NetWorkTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${native_config} "${SDK_ROOT}/lib")
endforeach()
target_include_directories(NetWorkTest PRIVATE "${SDK_ROOT}/include" "${SDK_ROOT}/include/protobuf" "${SDK_ROOT}/include/mbedtls")
target_compile_options(NetWorkTest PRIVATE "$<$<CONFIG:Release>:-O2>" "$<$<CONFIG:Release>:-g>")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(NetWorkTest PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
endif()
# CloudRebuild or the IDE supplies the existing dependency list; never build dependencies here.
set(native_prepared_libraries)
foreach(native_dependency IN LISTS LIBRARIESRELY)
    if(NOT native_dependency)
        continue()
    endif()
    if(IS_ABSOLUTE "${native_dependency}")
        set(native_dependency_path "${native_dependency}")
    else()
        set(native_dependency_path "${SDK_ROOT}/lib/${native_dependency}")
    endif()
    if(NOT EXISTS "${native_dependency_path}" OR IS_DIRECTORY "${native_dependency_path}")
        message(FATAL_ERROR "Missing prepared dependency: ${native_dependency_path}; run before/version for this target first")
    endif()
    list(APPEND native_prepared_libraries "${native_dependency_path}")
endforeach()
if(native_prepared_libraries)
    if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Android)$")
        target_link_libraries(NetWorkTest PUBLIC "-Wl,--start-group" ${native_prepared_libraries} "-Wl,--end-group")
    else()
        target_link_libraries(NetWorkTest PUBLIC ${native_prepared_libraries})
    endif()
endif()

find_package(Threads REQUIRED)
target_link_libraries(NetWorkTest PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(NetWorkTest PUBLIC rt atomic)
endif()

if(APPLE)
    find_library(native_security Security)
    if(NOT native_security)
        message(FATAL_ERROR "Target Security framework is required")
    endif()
    target_link_libraries(NetWorkTest PUBLIC "${native_security}")
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
        find_library(native_proc proc)
        if(NOT native_proc)
            message(FATAL_ERROR "Target macOS libproc is required")
        endif()
        target_link_libraries(NetWorkTest PUBLIC "${native_proc}")
    endif()
endif()
target_compile_definitions(NetWorkTest PRIVATE MBEDTLS_CONFIG_FILE="mbedtls/mbedtls_sdk_config.h")

if(NOT EXISTS "${SDK_ROOT}/lib/libmbedtlsstatic${BIT}.a" OR NOT EXISTS "${SDK_ROOT}/include/mbedtls/mbedtls_sdk_config.h")
    message(FATAL_ERROR "Missing target vendor SDK; run before/version with this platform's prepared SDK")
endif()
target_link_libraries(NetWorkTest PUBLIC "${SDK_ROOT}/lib/libmbedtlsstatic${BIT}.a")
target_include_directories(NetWorkTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../testdata/tls" "${CMAKE_CURRENT_SOURCE_DIR}/../../NetServerManager/NetServerManager")
enable_testing()
add_test(NAME NetWorkInstant COMMAND NetWorkTest --instant "--report=${CMAKE_CURRENT_BINARY_DIR}/NetWorkInstant.csv")

# White-box declarations are exposed only through each dependency API.
target_compile_definitions(NetWorkTest PRIVATE NET_GATEWAY_TEST_ACCESS)