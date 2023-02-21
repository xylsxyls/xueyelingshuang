
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CStopWatch_bit=$1
CStopWatch_dlllib=$2
CStopWatch_debugRelease=$3
CStopWatch_allSame=$4
"$CLOUD_REBUILD" CStopWatch $CStopWatch_bit $CStopWatch_dlllib $CStopWatch_debugRelease $CStopWatch_allSame

#--------------------------------------------------------------------
CHook_bit=$1
CHook_dlllib=$2
CHook_debugRelease=$3
CHook_allSame=$4
"$CLOUD_REBUILD" CHook $CHook_bit $CHook_dlllib $CHook_debugRelease $CHook_allSame