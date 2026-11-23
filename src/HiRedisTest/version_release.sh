set -e
"$CLOUD_REBUILD" HiRedis 32 lib release same
"$CLOUD_REBUILD" PlatformTest 32 dll release same

# 白盒用例从与本次构建匹配的源码单独准备命令解析头。
xueyelingshuang="${xueyelingshuang:-${XUEYELINGSHUANG:?}}"
cp -f -- "$xueyelingshuang/src/HiRedis/HiRedis/src/HiRedisHelper.h" "${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}/include/HiRedis/"