
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
SQLite_bit=$1
SQLite_dlllib=$2
SQLite_debugRelease=$3
SQLite_allSame=$4
"$CLOUD_REBUILD" SQLite $SQLite_bit $SQLite_dlllib $SQLite_debugRelease $SQLite_allSame

#--------------------------------------------------------------------
ReadWriteMutex_bit=$1
ReadWriteMutex_dlllib=$2
ReadWriteMutex_debugRelease=$3
ReadWriteMutex_allSame=$4
"$CLOUD_REBUILD" ReadWriteMutex $ReadWriteMutex_bit $ReadWriteMutex_dlllib $ReadWriteMutex_debugRelease $ReadWriteMutex_allSame