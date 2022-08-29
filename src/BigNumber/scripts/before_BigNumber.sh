
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
BigNumberBase_bit=$1
BigNumberBase_dlllib=lib
BigNumberBase_debugRelease=$3
BigNumberBase_allSame=$4
"$CLOUD_REBUILD" BigNumberBase $BigNumberBase_bit $BigNumberBase_dlllib $BigNumberBase_debugRelease $BigNumberBase_allSame