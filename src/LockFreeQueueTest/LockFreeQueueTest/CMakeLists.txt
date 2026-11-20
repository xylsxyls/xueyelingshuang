cmake_minimum_required(VERSION 3.15)
project(LockFreeQueueTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Tests own instrumentation; the library headers are never modified by this build.
set(queue_source "${CMAKE_CURRENT_SOURCE_DIR}/../../LockFreeQueue/LockFreeQueue/src")
set(queue_tests "${CMAKE_CURRENT_SOURCE_DIR}")
set(LOCK_FREE_QUEUE_WHITEBOX_DIR "${CMAKE_BINARY_DIR}/whitebox" CACHE PATH
    "Generated test headers; must be below src/temp/LockFreeQueueTest")
set(queue_root "${queue_tests}/../../..")
set(LOCK_FREE_QUEUE_HOST_TOOL "" CACHE FILEPATH "Native C++ host tool when cross-compiling")
if(NOT CMAKE_CROSSCOMPILING)
    add_executable(LockFreeQueueTestTool
        LockFreeQueueTestTool.cpp LockFreeQueueTestWhitebox.cpp
        LockFreeQueueTestRunner.cpp LockFreeQueueTestResults.cpp
        "${queue_root}/src/CSystem/CSystem/src/CSystem.cpp"
        "${queue_root}/src/JsonCpp/JsonCpp/src/json_reader.cpp"
        "${queue_root}/src/JsonCpp/JsonCpp/src/json_value.cpp"
        "${queue_root}/src/JsonCpp/JsonCpp/src/json_writer.cpp")
    target_compile_definitions(LockFreeQueueTestTool PRIVATE STATIC_LIB NOMINMAX _CRT_SECURE_NO_WARNINGS)
    if(WIN32)
        target_link_libraries(LockFreeQueueTestTool PRIVATE bcrypt Psapi Ole32 OleAut32 User32 Advapi32 Uuid)
    elseif(APPLE)
        target_link_libraries(LockFreeQueueTestTool PRIVATE "-framework Security")
    else()
        find_package(OpenSSL REQUIRED)
        target_link_libraries(LockFreeQueueTestTool PRIVATE OpenSSL::Crypto ${CMAKE_DL_LIBS})
    endif()
    if(MSVC)
        target_compile_options(LockFreeQueueTestTool PRIVATE /EHsc)
    endif()
    set(queue_host_tool "$<TARGET_FILE:LockFreeQueueTestTool>")
    set(queue_host_dependency LockFreeQueueTestTool)
else()
    if(NOT EXISTS "${LOCK_FREE_QUEUE_HOST_TOOL}")
        message(FATAL_ERROR "First build native LockFreeQueueTestTool, then pass -DLOCK_FREE_QUEUE_HOST_TOOL=/path/to/tool")
    endif()
    set(queue_host_tool "${LOCK_FREE_QUEUE_HOST_TOOL}")
endif()
add_custom_target(LockFreeQueueWhitebox
    COMMAND "${queue_host_tool}" whitebox --root "${queue_root}"
        --source "${queue_source}" --output "${LOCK_FREE_QUEUE_WHITEBOX_DIR}"
    DEPENDS ${queue_host_dependency}
    VERBATIM)
add_executable(LockFreeQueueTest LockFreeQueueTest.cpp LockFreeQueueRetryContract.cpp)
add_dependencies(LockFreeQueueTest LockFreeQueueWhitebox)
target_include_directories(LockFreeQueueTest PRIVATE "${LOCK_FREE_QUEUE_WHITEBOX_DIR}" "${queue_tests}")
target_compile_definitions(LockFreeQueueTest PRIVATE LOCK_FREE_QUEUE_TESTING)

# Performance and public consumers compile the uninstrumented production headers.
add_executable(LockFreeQueueMeasurements "${queue_tests}/LockFreeQueueMeasurements.cpp")
add_executable(LockFreeQueuePerformance "${queue_tests}/LockFreeQueuePerformance.cpp")
target_include_directories(LockFreeQueueMeasurements PRIVATE "${queue_source}")
target_include_directories(LockFreeQueuePerformance PRIVATE "${queue_source}")
target_compile_definitions(LockFreeQueuePerformance PRIVATE LOCK_FREE_QUEUE_BENCH_VARIANT=1)
# Integration consumes the current companion headers in their public include layout.
# Stage them privately so a stale installed SDK cannot satisfy an include by accident.
set(queue_peer_include "${CMAKE_CURRENT_BINARY_DIR}/peer-include")
foreach(peer LockFreeMap LockFreeHashMap Semaphore)
    set(peer_source "${queue_root}/src/${peer}/${peer}/src")
    file(GLOB peer_headers CONFIGURE_DEPENDS "${peer_source}/*.h" "${peer_source}/*.inl")
    foreach(header IN LISTS peer_headers)
        get_filename_component(header_name "${header}" NAME)
        configure_file("${header}" "${queue_peer_include}/${peer}/${header_name}" COPYONLY)
    endforeach()
endforeach()
file(GLOB queue_semaphore_sources CONFIGURE_DEPENDS "${queue_root}/src/Semaphore/Semaphore/src/*.cpp")
add_executable(LockFreeQueueIntegration LockFreeQueueIntegration.cpp ${queue_semaphore_sources})
target_include_directories(LockFreeQueueIntegration PRIVATE "${queue_source}" "${queue_peer_include}")
target_compile_definitions(LockFreeQueueIntegration PRIVATE STATIC_LIB NOMINMAX)
set(queue_targets LockFreeQueueTest LockFreeQueueIntegration LockFreeQueueMeasurements LockFreeQueuePerformance)
if(EMSCRIPTEN)
    option(LOCK_FREE_QUEUE_TESTS_WASM_THREADS "Exercise actual Wasm pthread workers" ON)
endif()
if(WIN32)
    add_executable(LockFreeQueueScheduling "${queue_tests}/LockFreeQueueScheduling.cpp")
    target_include_directories(LockFreeQueueScheduling PRIVATE "${queue_source}")
    list(APPEND queue_targets LockFreeQueueScheduling)
endif()
if(NOT EMSCRIPTEN OR LOCK_FREE_QUEUE_TESTS_WASM_THREADS)
    add_executable(LockFreeQueueSharedPtrBenchmark "${queue_tests}/LockFreeQueueSharedPtrBenchmark.cpp")
    target_include_directories(LockFreeQueueSharedPtrBenchmark PRIVATE "${queue_source}")
    list(APPEND queue_targets LockFreeQueueSharedPtrBenchmark)
endif()
set(queue_core_cases all)
if(EMSCRIPTEN)
    foreach(target IN LISTS queue_targets)
        target_compile_options(${target} PRIVATE -fexceptions)
        # The precreated shared_ptr benchmark can need more than 256 MiB.
        set(queue_max_memory 268435456)
        if(target STREQUAL "LockFreeQueueSharedPtrBenchmark")
            set(queue_max_memory 2147483648)
        endif()
        target_link_options(${target} PRIVATE -fexceptions
            -sENVIRONMENT=node -sWASM_ASYNC_COMPILATION=0 -sEXIT_RUNTIME=1
            -sALLOW_MEMORY_GROWTH=1 "-sMAXIMUM_MEMORY=${queue_max_memory}")
        if(LOCK_FREE_QUEUE_TESTS_WASM_THREADS)
            target_compile_options(${target} PRIVATE -pthread)
            target_link_options(${target} PRIVATE -pthread
                -sPTHREAD_POOL_SIZE=24 -sINITIAL_MEMORY=67108864)
        endif()
    endforeach()
    if(LOCK_FREE_QUEUE_TESTS_WASM_THREADS)
        # ID 9 waits on a real Semaphore, whose Wasm contract permits blocking
        # only on workers. Start its C++ main on a pthread, leaving JS responsive.
        target_link_options(LockFreeQueueIntegration PRIVATE -sPROXY_TO_PTHREAD=1)
    else()
        set(queue_core_cases "1,3,6,8,10,11,12,13,14,15,20,32,34,39,41,44,45,47,48,49,50")
    endif()
else()
    find_package(Threads REQUIRED)
    if(CMAKE_SYSTEM_NAME MATCHES "^(Linux|Android)$")
        # Some 32-bit ABIs lower 64-bit atomics to a runtime symbol. Probe at
        # link time; the queue still checks lock freedom on the actual device.
        include(CheckCXXSourceCompiles)
        set(queue_atomic_probe [=[
#include <atomic>
#include <cstdint>
struct Word { uint64_t value; };
int main() {
    std::atomic<Word> value(Word{1});
    Word expected = {1};
    value.compare_exchange_strong(expected, Word{2});
    return value.is_lock_free() && value.load().value == 2 ? 0 : 1;
}
]=])
        include(CMakePushCheckState)
        # Mobile toolchain files can default try_compile to a static archive,
        # which cannot detect a missing runtime symbol. Limit the override to this probe.
        function(queue_check_atomic_link result_variable)
            set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
            check_cxx_source_compiles("${queue_atomic_probe}" ${result_variable})
        endfunction()
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_LIBRARIES Threads::Threads)
        queue_check_atomic_link(QUEUE_TEST_ATOMICS_LINK)
        if(NOT QUEUE_TEST_ATOMICS_LINK)
            list(APPEND CMAKE_REQUIRED_LIBRARIES atomic)
            queue_check_atomic_link(QUEUE_TEST_LIBATOMIC_LINK)
            if(NOT QUEUE_TEST_LIBATOMIC_LINK)
                message(FATAL_ERROR "Target toolchain cannot link the required 64-bit atomic operations")
            endif()
            foreach(target IN LISTS queue_targets)
                target_link_libraries(${target} PRIVATE atomic)
            endforeach()
        endif()
        cmake_pop_check_state()
    endif()
    if(TARGET LockFreeQueueTestTool)
        target_link_libraries(LockFreeQueueTestTool PRIVATE Threads::Threads)
    endif()
    foreach(target IN LISTS queue_targets)
        target_link_libraries(${target} PRIVATE Threads::Threads)
    endforeach()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    foreach(target IN LISTS queue_targets)
        set_target_properties(${target} PROPERTIES MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.lockfreequeue.${target}")
    endforeach()
endif()
if(WIN32)
    foreach(target IN LISTS queue_targets)
        target_link_libraries(${target} PRIVATE Psapi)
    endforeach()
endif()
if(MSVC)
    foreach(target IN LISTS queue_targets)
        target_compile_options(${target} PRIVATE /EHsc)
    endforeach()
endif()
enable_testing()
if(NOT CMAKE_CROSSCOMPILING)
    get_filename_component(queue_temp "${queue_root}/src/temp/LockFreeQueueTest" ABSOLUTE)
    file(RELATIVE_PATH queue_harness_batch "${queue_temp}" "${LOCK_FREE_QUEUE_WHITEBOX_DIR}/harness-self-test")
    add_test(NAME LockFreeQueueHarness COMMAND LockFreeQueueTestTool self-test
        --root "${queue_root}" --batch "${queue_harness_batch}")
    set_tests_properties(LockFreeQueueHarness PROPERTIES TIMEOUT 30)
endif()
add_test(NAME LockFreeQueueCore COMMAND LockFreeQueueTest ${queue_core_cases})
add_test(NAME LockFreeQueueHeaders COMMAND LockFreeQueueIntegration 23)
set_tests_properties(LockFreeQueueCore LockFreeQueueHeaders PROPERTIES LABELS correctness TIMEOUT 180)
if(NOT EMSCRIPTEN OR LOCK_FREE_QUEUE_TESTS_WASM_THREADS)
    add_test(NAME LockFreeQueueNotification COMMAND LockFreeQueueIntegration 9)
    set_tests_properties(LockFreeQueueNotification PROPERTIES LABELS correctness TIMEOUT 180)
endif()
add_test(NAME LockFreeQueueBacklog COMMAND LockFreeQueueMeasurements 22)
set_tests_properties(LockFreeQueueCore LockFreeQueueBacklog PROPERTIES TIMEOUT 180)
if(NOT EMSCRIPTEN OR LOCK_FREE_QUEUE_TESTS_WASM_THREADS)
    add_test(NAME LockFreeQueuePoolPerformance COMMAND LockFreeQueuePerformance 38 --no-allocation-count)
    add_test(NAME LockFreeQueueWavePerformance COMMAND LockFreeQueuePerformance 40 --no-allocation-count)
    set_tests_properties(LockFreeQueuePoolPerformance PROPERTIES TIMEOUT 180)
    set_tests_properties(LockFreeQueueWavePerformance PROPERTIES TIMEOUT 240)
endif()