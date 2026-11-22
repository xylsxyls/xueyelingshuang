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
ProcessWork_bit=$1
ProcessWork_dlllib=$2
ProcessWork_debugRelease=$3
ProcessWork_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProcessWork "$ProcessWork_bit" "$ProcessWork_dlllib" "$ProcessWork_debugRelease" "$ProcessWork_allSame"
fi
librariesrely+=($'project\tProcessWork\t'"$ProcessWork_dlllib")

#--------------------------------------------------------------------
ProtoMessageManager_bit=$1
ProtoMessageManager_dlllib=$2
ProtoMessageManager_debugRelease=$3
ProtoMessageManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProtoMessageManager "$ProtoMessageManager_bit" "$ProtoMessageManager_dlllib" "$ProtoMessageManager_debugRelease" "$ProtoMessageManager_allSame"
fi
librariesrely+=($'project\tProtoMessageManager\t'"$ProtoMessageManager_dlllib")

#--------------------------------------------------------------------
NetInterface_bit=$1
NetInterface_dlllib=$2
NetInterface_debugRelease=$3
NetInterface_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" NetInterface "$NetInterface_bit" "$NetInterface_dlllib" "$NetInterface_debugRelease" "$NetInterface_allSame"
fi
librariesrely+=($'project\tNetInterface\t'"$NetInterface_dlllib")

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
LockFreeHashMap_bit=$1
LockFreeHashMap_dlllib=$2
LockFreeHashMap_debugRelease=$3
LockFreeHashMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeHashMap "$LockFreeHashMap_bit" "$LockFreeHashMap_dlllib" "$LockFreeHashMap_debugRelease" "$LockFreeHashMap_allSame"
fi
librariesrely+=($'project\tLockFreeHashMap\t'"$LockFreeHashMap_dlllib")

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeQueue "$LockFreeQueue_bit" "$LockFreeQueue_dlllib" "$LockFreeQueue_debugRelease" "$LockFreeQueue_allSame"
fi
librariesrely+=($'project\tLockFreeQueue\t'"$LockFreeQueue_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return