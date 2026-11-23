#!/bin/bash
set -e
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"

rm -f -- "$xueyelingshuang/include/MysqlCpp/MysqlCppExecution.h"
rm -f -- "$xueyelingshuang/include/MysqlCpp/MysqlCppHelper.h"
rm -f -- "$xueyelingshuang/include/MysqlCpp/MysqlCppPoolLifetime.h"
