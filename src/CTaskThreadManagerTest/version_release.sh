set -e
"$CLOUD_REBUILD" CTaskThreadManager 32 dll release same
"$CLOUD_REBUILD" PlatformTest 32 dll release same

# 依赖发布完毕后，为本工程的白盒用例准备私有头。
xueyelingshuang="${xueyelingshuang:-${XUEYELINGSHUANG:?}}"
headers="$xueyelingshuang/src/CTaskThreadManager/CTaskThreadManager/src"
cp -f -- "$headers/CTaskAdmission.h" "$headers/CTaskAdmission.inl" \
    "$headers/CTaskBucketCache.h" "$headers/CTaskBucketCache.inl" \
    "$headers/CTaskManagerState.h" "$headers/CTaskPoolState.h" "$headers/CTaskPriorityBucket.h" \
    "$headers/CTaskReadDomain.h" "$headers/CTaskReadDomain.inl" \
    "$headers/CTaskReadyIndex.h" "$headers/CTaskReadyIndex.inl" "$headers/CTaskResultAdapter.h" \
    "$headers/CTaskRunningIndex.h" "$headers/CTaskRunningIndex.inl" \
    "$headers/CTaskStorage.h" "$headers/CTaskStorage.inl" "$headers/CTaskStorageModels.h" "$headers/CTaskStorageModels.inl" \
    "$headers/CTaskThreadPriority.h" "$headers/CTaskThreadState.h" \
    "${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}/include/CTaskThreadManager/"