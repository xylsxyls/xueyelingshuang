librariesrely=()

:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" DllRelyTest "$DllRelyTest_bit" "$DllRelyTest_dlllib" "$DllRelyTest_debugRelease" "$DllRelyTest_allSame"
fi
librariesrely+=($'project\tDllRelyTest\t'"$DllRelyTest_dlllib")
!EOF!

#--------------------------------------------------------------------
Semaphore_bit=$1
Semaphore_dlllib=$2
Semaphore_debugRelease=$3
Semaphore_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Semaphore "$Semaphore_bit" "$Semaphore_dlllib" "$Semaphore_debugRelease" "$Semaphore_allSame"
fi
librariesrely+=($'project\tSemaphore\t'"$Semaphore_dlllib")

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeQueue "$LockFreeQueue_bit" "$LockFreeQueue_dlllib" "$LockFreeQueue_debugRelease" "$LockFreeQueue_allSame"
fi
librariesrely+=($'project\tLockFreeQueue\t'"$LockFreeQueue_dlllib")

#--------------------------------------------------------------------
LockFreeMap_bit=$1
LockFreeMap_dlllib=$2
LockFreeMap_debugRelease=$3
LockFreeMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeMap "$LockFreeMap_bit" "$LockFreeMap_dlllib" "$LockFreeMap_debugRelease" "$LockFreeMap_allSame"
fi
librariesrely+=($'project\tLockFreeMap\t'"$LockFreeMap_dlllib")

#--------------------------------------------------------------------
LockFreeHashMap_bit=$1
LockFreeHashMap_dlllib=$2
LockFreeHashMap_debugRelease=$3
LockFreeHashMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeHashMap "$LockFreeHashMap_bit" "$LockFreeHashMap_dlllib" "$LockFreeHashMap_debugRelease" "$LockFreeHashMap_allSame"
fi
librariesrely+=($'project\tLockFreeHashMap\t'"$LockFreeHashMap_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return