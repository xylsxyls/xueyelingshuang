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
SharedMemory_bit=$1
SharedMemory_dlllib=$2
SharedMemory_debugRelease=$3
SharedMemory_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" SharedMemory "$SharedMemory_bit" "$SharedMemory_dlllib" "$SharedMemory_debugRelease" "$SharedMemory_allSame"
fi
librariesrely+=($'project\tSharedMemory\t'"$SharedMemory_dlllib")

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CTaskThreadManager "$CTaskThreadManager_bit" "$CTaskThreadManager_dlllib" "$CTaskThreadManager_debugRelease" "$CTaskThreadManager_allSame"
fi
librariesrely+=($'project\tCTaskThreadManager\t'"$CTaskThreadManager_dlllib")

#--------------------------------------------------------------------
CorrespondParam_bit=$1
CorrespondParam_dlllib=$2
CorrespondParam_debugRelease=$3
CorrespondParam_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CorrespondParam "$CorrespondParam_bit" "$CorrespondParam_dlllib" "$CorrespondParam_debugRelease" "$CorrespondParam_allSame"
fi
librariesrely+=($'project\tCorrespondParam\t'"$CorrespondParam_dlllib")

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame"
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CSystem "$CSystem_bit" "$CSystem_dlllib" "$CSystem_debugRelease" "$CSystem_allSame"
fi
librariesrely+=($'project\tCSystem\t'"$CSystem_dlllib")

#--------------------------------------------------------------------
ReadWriteMutex_bit=$1
ReadWriteMutex_dlllib=$2
ReadWriteMutex_debugRelease=$3
ReadWriteMutex_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ReadWriteMutex "$ReadWriteMutex_bit" "$ReadWriteMutex_dlllib" "$ReadWriteMutex_debugRelease" "$ReadWriteMutex_allSame"
fi
librariesrely+=($'project\tReadWriteMutex\t'"$ReadWriteMutex_dlllib")

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
AtomicMath_bit=$1
AtomicMath_dlllib=$2
AtomicMath_debugRelease=$3
AtomicMath_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" AtomicMath "$AtomicMath_bit" "$AtomicMath_dlllib" "$AtomicMath_debugRelease" "$AtomicMath_allSame"
fi
librariesrely+=($'project\tAtomicMath\t'"$AtomicMath_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
