
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

#--------------------------------------------------------------------
LogManager_bit=$1
LogManager_dlllib=$2
LogManager_debugRelease=$3
LogManager_allSame=$4
"$CLOUD_REBUILD" LogManager $LogManager_bit $LogManager_dlllib $LogManager_debugRelease $LogManager_allSame

#--------------------------------------------------------------------
PdfEngine_bit=$1
PdfEngine_dlllib=$2
PdfEngine_debugRelease=$3
PdfEngine_allSame=$4
"$CLOUD_REBUILD" PdfEngine $PdfEngine_bit $PdfEngine_dlllib $PdfEngine_debugRelease $PdfEngine_allSame