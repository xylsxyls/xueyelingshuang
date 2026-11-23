#!/bin/bash
# 完成 SDK 头文件发布后，仅移除两个实现内部使用的头文件。
script_root=$(cd -- "$(dirname -- "$0")/../../.." && pwd) || exit 1
sdk_root="${CLOUD_REBUILD_SDK_ROOT:-$script_root}"
headers="${sdk_root//\\//}/include/MysqlCpp"
rm -f -- "$headers/MysqlCppExecution.h" "$headers/MysqlCppPoolLifetime.h"