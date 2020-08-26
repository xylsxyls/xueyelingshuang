
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
SharedMemory_bit=$1
SharedMemory_dlllib=$2
SharedMemory_debugRelease=$3
SharedMemory_allSame=$4
"$CLOUD_REBUILD" SharedMemory $SharedMemory_bit $SharedMemory_dlllib $SharedMemory_debugRelease $SharedMemory_allSame

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
"$CLOUD_REBUILD" CTaskThreadManager $CTaskThreadManager_bit $CTaskThreadManager_dlllib $CTaskThreadManager_debugRelease $CTaskThreadManager_allSame

#--------------------------------------------------------------------
CorrespondParam_bit=$1
CorrespondParam_dlllib=$2
CorrespondParam_debugRelease=$3
CorrespondParam_allSame=$4
"$CLOUD_REBUILD" CorrespondParam $CorrespondParam_bit $CorrespondParam_dlllib $CorrespondParam_debugRelease $CorrespondParam_allSame

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
"$CLOUD_REBUILD" CStringManager $CStringManager_bit $CStringManager_dlllib $CStringManager_debugRelease $CStringManager_allSame

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