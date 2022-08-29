
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
BigNumber_bit=$1
BigNumber_dlllib=$2
BigNumber_debugRelease=$3
BigNumber_allSame=$4
"$CLOUD_REBUILD" BigNumber $BigNumber_bit $BigNumber_dlllib $BigNumber_debugRelease $BigNumber_allSame