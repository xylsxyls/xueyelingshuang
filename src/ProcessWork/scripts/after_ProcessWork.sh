#!/bin/bash
set -e
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"

rm -f -- "$xueyelingshuang/include/ProcessWork/ProcessWorkSharedQueue.h"
rm -f -- "$xueyelingshuang/include/ProcessWork/ProcessWorkSharedQueue.inl"
rm -f -- "$xueyelingshuang/include/ProcessWork/ProcessWorkStorage.h"
rm -f -- "$xueyelingshuang/include/ProcessWork/ProcessWorkStorage.inl"
rm -f -- "$xueyelingshuang/include/ProcessWork/ProcessWorkTime.h"
rm -f -- "$xueyelingshuang/include/ProcessWork/ReadTask.h"
rm -f -- "$xueyelingshuang/include/ProcessWork/ReceiveTask.h"
rm -f -- "$xueyelingshuang/include/ProcessWork/SendTask.h"
