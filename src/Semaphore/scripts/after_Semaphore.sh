#!/bin/bash
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"
rm -f -- "$xueyelingshuang/include/Semaphore/SemaphoreCount.h" &&
rm -f -- "$xueyelingshuang/include/Semaphore/SemaphoreDeadline.h" &&
rm -f -- "$xueyelingshuang/include/Semaphore/SemaphoreProcessMemory.h" &&
rm -f -- "$xueyelingshuang/include/Semaphore/SemaphoreRetry.h"