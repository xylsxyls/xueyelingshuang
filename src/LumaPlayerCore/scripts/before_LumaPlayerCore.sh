
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

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
FFmpegCpp_bit=$1
FFmpegCpp_dlllib=$2
FFmpegCpp_debugRelease=$3
FFmpegCpp_allSame=$4
"$CLOUD_REBUILD" FFmpegCpp $FFmpegCpp_bit $FFmpegCpp_dlllib $FFmpegCpp_debugRelease $FFmpegCpp_allSame

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
"$CLOUD_REBUILD" CTaskThreadManager $CTaskThreadManager_bit $CTaskThreadManager_dlllib $CTaskThreadManager_debugRelease $CTaskThreadManager_allSame

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
"$CLOUD_REBUILD" LockFreeQueue $LockFreeQueue_bit $LockFreeQueue_dlllib $LockFreeQueue_debugRelease $LockFreeQueue_allSame