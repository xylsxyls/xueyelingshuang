
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
Semaphore_bit=$1
Semaphore_dlllib=$2
Semaphore_debugRelease=$3
Semaphore_allSame=$4
"$CLOUD_REBUILD" Semaphore $Semaphore_bit $Semaphore_dlllib $Semaphore_debugRelease $Semaphore_allSame