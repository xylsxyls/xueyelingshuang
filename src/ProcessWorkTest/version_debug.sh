set -e
"$CLOUD_REBUILD" ProcessWork 32 dll debug same
"$CLOUD_REBUILD" PlatformTest 32 dll debug same

# 依赖发布完毕后，为本工程的白盒用例准备私有头。
xueyelingshuang="${xueyelingshuang:-${XUEYELINGSHUANG:?}}"
headers="$xueyelingshuang/src/ProcessWork/ProcessWork/src"
cp -f -- "$headers/ProcessWorkSharedQueue.h" "$headers/ProcessWorkSharedQueue.inl" \
    "$headers/ProcessWorkStorage.h" "$headers/ProcessWorkStorage.inl" "$headers/ProcessWorkTime.h" \
    "${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}/include/ProcessWork/"