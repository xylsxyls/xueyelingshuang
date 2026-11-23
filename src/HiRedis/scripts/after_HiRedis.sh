#!/bin/bash
set -e
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"

rm -f -- "$xueyelingshuang/include/HiRedis/HiRedisConnectionReleaser.h"
rm -f -- "$xueyelingshuang/include/HiRedis/HiRedisHelper.h"