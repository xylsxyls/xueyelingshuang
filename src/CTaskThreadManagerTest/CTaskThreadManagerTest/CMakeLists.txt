#设置CMAKE最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.1.1)
option(CTTM_WASM_SMOKE "Build the standalone Emscripten pthread smoke test" OFF)
if(CTTM_WASM_SMOKE)
    cmake_minimum_required(VERSION 3.15)
    project(CTaskWasmSmoke LANGUAGES CXX)
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR "This target verifies the Emscripten pthread consumer")
    endif()
    if(NOT CTTM_INCLUDE_ROOT OR NOT CTTM_LIBRARY_ROOT)
        message(FATAL_ERROR "Set CTTM_INCLUDE_ROOT to matched C++ headers and CTTM_LIBRARY_ROOT to pthread archives")
    endif()
    add_executable(CTaskWasmSmoke CTaskWasmSmoke.cpp CTaskWasmSmoke.h)
    set_target_properties(CTaskWasmSmoke PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES SUFFIX ".js")
    target_compile_definitions(CTaskWasmSmoke PRIVATE STATIC_LIB)
    target_include_directories(CTaskWasmSmoke PRIVATE "${CTTM_INCLUDE_ROOT}")
    target_compile_options(CTaskWasmSmoke PRIVATE -pthread -fexceptions)
    target_link_libraries(CTaskWasmSmoke PRIVATE "${CTTM_LIBRARY_ROOT}/CTaskThreadManager.a" "${CTTM_LIBRARY_ROOT}/Semaphore.a")
    target_link_options(CTaskWasmSmoke PRIVATE -pthread -fexceptions -sPROXY_TO_PTHREAD -sPTHREAD_POOL_SIZE=4 -sENVIRONMENT=node -sEXIT_RUNTIME=1 -sINITIAL_MEMORY=67108864)
    return()
endif()

SET(CMAKE_CXX_STANDARD 11)
if(NOT DEFINED DEBUGRELEASE)
    SET(DEBUGRELEASE release)
endif()
if(NOT DEFINED BIT)
    SET(BIT 64)
endif()

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset='gb2312'")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset='gb2312'")

if("${DEBUGRELEASE}" STREQUAL "debug")
#设置项目名称
SET(PROJECT_NAME CTaskThreadManagerTestd)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wl,-rpath=. -O0 -g -ggdb ")
SET(CMAKE_BUILD_TYPE "Debug")
elseif("${DEBUGRELEASE}" STREQUAL "release")
#设置项目名称
SET(PROJECT_NAME CTaskThreadManagerTest)
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wl,-rpath=. -O2")
SET(CMAKE_BUILD_TYPE "Release")
endif()

if("${BIT}" STREQUAL "32")
    MESSAGE(STATUS "using 32bits (x86)")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
elseif("${BIT}" STREQUAL "64")
    MESSAGE(STATUS "using 64bits (x86_64)")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
elseif("${BIT}" STREQUAL "arm32")
    MESSAGE(STATUS "using arm32 (armhf)")
    SET(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
    SET(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")
elseif("${BIT}" STREQUAL "arm64")
    MESSAGE(STATUS "using arm64 (aarch64)")
    SET(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
    SET(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")
else()
    MESSAGE(FATAL_ERROR "Unknown BIT value: ${BIT}. Please set BIT to '32', '64', 'arm32', or 'arm64'")
endif()

#建立项目
PROJECT(${PROJECT_NAME})

#MESSAGE("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
#${PROJECT_SOURCE_DIR}="xueyelingshuang/src/CTaskThreadManagerTest/CTaskThreadManagerTest"
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../../lib/)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../../lib/)

#添加包含目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../../../include/)

#添加库目录
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/../../../lib/)

#获取代码，在项目中，将所有代码都放在src文件夹中
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/ DIR_SRCS)
# Wasm 检查有独立 main，只通过 CTTM_WASM_SMOKE 目标构建。
foreach(TEST_SOURCE ${DIR_SRCS})
    get_filename_component(TEST_SOURCE_NAME "${TEST_SOURCE}" NAME)
    if(TEST_SOURCE_NAME STREQUAL "CTaskWasmSmoke.cpp")
        list(REMOVE_ITEM DIR_SRCS "${TEST_SOURCE}")
    endif()
endforeach()
#编译可执行程序
ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PRIVATE CTTM_TEST_NO_LEGACY_SEND_TASK CTTM_BENCH_V3 CTTM_BENCH_V5)
#如果要生成静态链接库
#ADD_LIBRARY(${PROJECT_NAME} STATIC ${DIR_SRCS})
#如果要生成动态链接库
#ADD_LIBRARY(${PROJECT_NAME} SHARED ${DIR_SRCS})

#添加库
list(APPEND LIBRARYLIST ${LIBRARIESRELY})
foreach (EVERYLIBRARY ${LIBRARYLIST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../../../lib/${EVERYLIBRARY})
endforeach ()
FIND_PACKAGE(Threads REQUIRED)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})