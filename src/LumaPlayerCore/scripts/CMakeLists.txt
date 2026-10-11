#设置CMAKE最低版本
CMAKE_MINIMUM_REQUIRED(VERSION 3.1.1)
if(DEFINED LUMAPLAYER_DEPS_ROOT)
    # Target-SDK build, isolated from the repository's shared Windows include/lib staging.
    cmake_minimum_required(VERSION 3.18)
    project(LumaPlayerCore LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    get_filename_component(LUMA_REPOSITORY "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
    get_filename_component(LUMAPLAYER_DEPS_ROOT "${LUMAPLAYER_DEPS_ROOT}" ABSOLUTE)
    set(LUMA_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/include")
    set(LUMA_TOOLS "${LUMA_REPOSITORY}/tools/LumaPlayerKaraoke")

    if(WIN32)
        message(FATAL_ERROR "Use the existing version script / VS2013 solution for Windows. This entry uses POSIX target SDKs.")
    endif()
    if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|Darwin|Android|iOS)$")
        message(FATAL_ERROR "Unsupported target: ${CMAKE_SYSTEM_NAME}")
    endif()
    foreach(header ffmpeg/libavcodec/avcodec.h ffmpeg/libavutil/avconfig.h soundtouchinclude/SoundTouch.h soundtouchinclude/SoundTouchDLL.h)
        if(NOT EXISTS "${LUMAPLAYER_DEPS_ROOT}/include/${header}")
            message(FATAL_ERROR "Missing target header ${header} in ${LUMAPLAYER_DEPS_ROOT}; do not use Windows FFmpeg headers.")
        endif()
    endforeach()

    # Compile the existing secondary libraries, not alternative copies or reduced Core implementations.
    set(LUMA_LIBRARIES CStringManager CSystem Semaphore ReadWriteMutex IntDateTime CTaskThreadManager LockFreeQueue LogManager FFmpegCpp KissFFTCpp)
    foreach(library ${LUMA_LIBRARIES} LumaPlayerCore)
        file(GLOB headers "${LUMA_REPOSITORY}/src/${library}/${library}/src/*.h"
            "${LUMA_REPOSITORY}/src/${library}/${library}/src/*.inl")
        file(MAKE_DIRECTORY "${LUMA_HEADERS}/${library}")
        foreach(header ${headers})
            get_filename_component(name "${header}" NAME)
            configure_file("${header}" "${LUMA_HEADERS}/${library}/${name}" COPYONLY)
        endforeach()
    endforeach()
    file(MAKE_DIRECTORY "${LUMA_HEADERS}/miniaudio" "${LUMA_HEADERS}/onnxruntime")
    configure_file("${LUMA_TOOLS}/miniaudio/include/miniaudio.h" "${LUMA_HEADERS}/miniaudio/miniaudio.h" COPYONLY)
    file(GLOB ort_headers "${LUMA_TOOLS}/directml-1.24.4/include/*.h")
    foreach(header ${ort_headers})
        get_filename_component(name "${header}" NAME)
        configure_file("${header}" "${LUMA_HEADERS}/onnxruntime/${name}" COPYONLY)
    endforeach()
    include_directories("${LUMA_HEADERS}" "${LUMAPLAYER_DEPS_ROOT}/include" "${LUMAPLAYER_DEPS_ROOT}/include/ffmpeg")
    find_package(Threads REQUIRED)
    foreach(library ${LUMA_LIBRARIES})
        file(GLOB sources CONFIGURE_DEPENDS "${LUMA_REPOSITORY}/src/${library}/${library}/src/*.cpp")
        add_library(${library} STATIC ${sources})
        target_link_libraries(${library} PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
    endforeach()
    target_link_libraries(CTaskThreadManager PUBLIC Semaphore)
    target_sources(KissFFTCpp PRIVATE "${LUMA_REPOSITORY}/src/KissFFTCpp/KissFFTCpp/src/kiss_fft.c"
        "${LUMA_REPOSITORY}/src/KissFFTCpp/KissFFTCpp/src/kiss_fftr.c")
    target_compile_definitions(KissFFTCpp PRIVATE kiss_fft_scalar=double)
    target_link_libraries(IntDateTime PUBLIC CStringManager)
    target_link_libraries(LogManager PUBLIC CStringManager CSystem ReadWriteMutex IntDateTime)

    # LGPL dependencies stay replaceable shared libraries, including when Core itself is static.
    foreach(component avformat avcodec swscale swresample avutil soundtouch)
        set(names ${component})
        if(component STREQUAL "soundtouch")
            set(names SoundTouchDLL)
        endif()
        find_library(LUMA_${component}_LIBRARY NAMES ${names}
            PATHS "${LUMAPLAYER_DEPS_ROOT}/lib" NO_DEFAULT_PATH)
        if(NOT LUMA_${component}_LIBRARY)
            message(FATAL_ERROR "Missing ${component} target shared library; set LUMA_${component}_LIBRARY explicitly for nonstandard shared/framework names.")
        endif()
        if(LUMA_${component}_LIBRARY MATCHES "\\.a$")
            message(FATAL_ERROR "${component} must use a replaceable shared library/framework, not ${LUMA_${component}_LIBRARY}.")
        endif()
        target_link_libraries(FFmpegCpp PUBLIC "${LUMA_${component}_LIBRARY}")
    endforeach()
    set(LUMA_FFMPEG_EXTRA_LIBRARIES "" CACHE STRING "Additional target FFmpeg codec/compression/system dependencies")
    target_link_libraries(FFmpegCpp PUBLIC ${LUMA_FFMPEG_EXTRA_LIBRARIES} m)

    file(GLOB core_sources CONFIGURE_DEPENDS "${LUMA_REPOSITORY}/src/LumaPlayerCore/LumaPlayerCore/src/*.cpp"
        "${LUMA_REPOSITORY}/src/LumaPlayerCore/LumaPlayerCore/src/*.c")
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        # miniaudio's iOS backend uses AVAudioSession; only this vendor implementation needs Objective-C++.
        enable_language(OBJCXX)
        set(CMAKE_OBJCXX_STANDARD 11)
        set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
        set_source_files_properties("${LUMA_REPOSITORY}/src/LumaPlayerCore/LumaPlayerCore/src/KaraokeMiniaudio.cpp"
            PROPERTIES LANGUAGE OBJCXX)
    endif()
    add_library(LumaPlayerCoreObjects OBJECT ${core_sources})
    add_library(LumaPlayerCore STATIC $<TARGET_OBJECTS:LumaPlayerCoreObjects>)
    set(LUMA_CORE_LINK CStringManager FFmpegCpp CTaskThreadManager LockFreeQueue LogManager KissFFTCpp Threads::Threads ${CMAKE_DL_LIBS} m)
    if(ANDROID)
        list(APPEND LUMA_CORE_LINK log android)
    elseif(APPLE)
        foreach(framework CoreFoundation CoreAudio AudioToolbox)
            find_library(LUMA_${framework}_FRAMEWORK ${framework} REQUIRED)
            list(APPEND LUMA_CORE_LINK "${LUMA_${framework}_FRAMEWORK}")
        endforeach()
    endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        foreach(framework Foundation AVFoundation)
            find_library(LUMA_${framework}_FRAMEWORK ${framework} REQUIRED)
            list(APPEND LUMA_CORE_LINK "${LUMA_${framework}_FRAMEWORK}")
        endforeach()
        find_library(LUMA_ORT_LIBRARY NAMES onnxruntime PATHS "${LUMAPLAYER_DEPS_ROOT}/lib" NO_DEFAULT_PATH)
        if(NOT LUMA_ORT_LIBRARY)
            message(FATAL_ERROR "iOS requires linked ORT. Set LUMA_ORT_LIBRARY to the correct device/simulator framework binary or archive.")
        endif()
        target_compile_definitions(LumaPlayerCoreObjects PRIVATE LUMAPLAYER_ORT_LINKED)
        list(APPEND LUMA_CORE_LINK "${LUMA_ORT_LIBRARY}")
    endif()
    set(LUMA_ORT_EXTRA_LIBRARIES "" CACHE STRING "Additional target dependencies for static ONNX Runtime")
    target_link_libraries(LumaPlayerCore PUBLIC ${LUMA_CORE_LINK} ${LUMA_ORT_EXTRA_LIBRARIES})
    target_include_directories(LumaPlayerCore PUBLIC "${LUMA_HEADERS}")

    # Link every Core translation unit, so an archive alone cannot hide missing platform symbols.
    option(LUMAPLAYER_LINK_CHECK "Build a pure-C smoke executable and fully link all Core objects" ON)
    if(LUMAPLAYER_LINK_CHECK)
        add_executable(LumaPlayerCoreLinkCheck
            "${LUMA_REPOSITORY}/src/LumaPlayerTest/LumaPlayerTest/CApiSmoke.c"
            $<TARGET_OBJECTS:LumaPlayerCoreObjects>)
        target_compile_definitions(LumaPlayerCoreLinkCheck PRIVATE LUMAPLAYER_CORE_LINK_CHECK)
        target_link_libraries(LumaPlayerCoreLinkCheck PRIVATE ${LUMA_CORE_LINK} ${LUMA_ORT_EXTRA_LIBRARIES})
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set_target_properties(LumaPlayerCoreLinkCheck PROPERTIES MACOSX_BUNDLE TRUE
                XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
        endif()
    endif()
    return()
endif()
SET(CMAKE_CXX_STANDARD 11)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset='gb2312'")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset='gb2312'")

if(${DEBUGRELEASE} STREQUAL "debug")
#设置项目名称
SET(PROJECT_NAME LumaPlayerCored)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wl,-rpath=. -O0 -g -ggdb ")
SET(CMAKE_BUILD_TYPE "Debug")
elseif(${DEBUGRELEASE} STREQUAL "release")
#设置项目名称
SET(PROJECT_NAME LumaPlayerCore)
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -Wl,-rpath=. -O2 -g")
SET(CMAKE_BUILD_TYPE "Release")
endif()

if(${BIT} STREQUAL "32")
    MESSAGE(STATUS "using 32bits (x86)")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
elseif(${BIT} STREQUAL "64")
    MESSAGE(STATUS "using 64bits (x86_64)")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
elseif(${BIT} STREQUAL "arm32")
    MESSAGE(STATUS "using arm32 (armhf)")
    SET(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
    SET(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")
elseif(${BIT} STREQUAL "arm64")
    MESSAGE(STATUS "using arm64 (aarch64)")
    SET(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
    SET(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")
else()
    MESSAGE(FATAL_ERROR "Unknown BIT value: ${BIT}. Please set BIT to '32', '64', 'arm32', or 'arm64'")
endif()

#建立项目
PROJECT(${PROJECT_NAME})

#MESSAGE("PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
#${PROJECT_SOURCE_DIR}="xueyelingshuang/src/LumaPlayerCore/scripts"
#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../../lib/)

#添加包含目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../../../include/)

#添加库目录
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/../../../lib/)

#获取代码，在项目中，将所有代码都放在src文件夹中
AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/../LumaPlayerCore/src/ DIR_SRCS)
#编译可执行程序
#ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})

if(${DLLLIB} STREQUAL "dll")
#如果要生成动态链接库
ADD_LIBRARY(${PROJECT_NAME} SHARED ${DIR_SRCS})
elseif(${DLLLIB} STREQUAL "lib")
#如果要生成静态链接库
ADD_LIBRARY(${PROJECT_NAME} STATIC ${DIR_SRCS})
endif()

#添加库
list(APPEND LIBRARYLIST ${LIBRARIESRELY})
foreach (EVERYLIBRARY ${LIBRARYLIST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/../../../lib/${EVERYLIBRARY})
endforeach ()

if(UNIX)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_DL_LIBS} pthread m)
endif()