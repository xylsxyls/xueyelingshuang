
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
AtomicMath_bit=$1
AtomicMath_dlllib=$2
AtomicMath_debugRelease=$3
AtomicMath_allSame=$4
"$CLOUD_REBUILD" AtomicMath $AtomicMath_bit $AtomicMath_dlllib $AtomicMath_debugRelease $AtomicMath_allSame