
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
"$CLOUD_REBUILD" CSystem $CSystem_bit $CSystem_dlllib $CSystem_debugRelease $CSystem_allSame

#--------------------------------------------------------------------
ReadWriteMutex_bit=$1
ReadWriteMutex_dlllib=$2
ReadWriteMutex_debugRelease=$3
ReadWriteMutex_allSame=$4
"$CLOUD_REBUILD" ReadWriteMutex $ReadWriteMutex_bit $ReadWriteMutex_dlllib $ReadWriteMutex_debugRelease $ReadWriteMutex_allSame

#--------------------------------------------------------------------
IntDateTime_bit=$1
IntDateTime_dlllib=$2
IntDateTime_debugRelease=$3
IntDateTime_allSame=$4
"$CLOUD_REBUILD" IntDateTime $IntDateTime_bit $IntDateTime_dlllib $IntDateTime_debugRelease $IntDateTime_allSame