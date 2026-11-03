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
NetPeer_bit=$1
NetPeer_dlllib=$2
NetPeer_debugRelease=$3
NetPeer_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" NetPeer "$NetPeer_bit" "$NetPeer_dlllib" "$NetPeer_debugRelease" "$NetPeer_allSame"
fi
librariesrely+=($'project\tNetPeer\t'"$NetPeer_dlllib")

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
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame"
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
