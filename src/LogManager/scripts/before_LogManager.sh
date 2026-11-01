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
IntDateTime_bit=$1
IntDateTime_dlllib=$2
IntDateTime_debugRelease=$3
IntDateTime_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" IntDateTime "$IntDateTime_bit" "$IntDateTime_dlllib" "$IntDateTime_debugRelease" "$IntDateTime_allSame"
fi
librariesrely+=($'project\tIntDateTime\t'"$IntDateTime_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
