
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
QtControls_bit=$1
QtControls_dlllib=$2
QtControls_debugRelease=$3
QtControls_allSame=$4
"$CLOUD_REBUILD" QtControls $QtControls_bit $QtControls_dlllib $QtControls_debugRelease $QtControls_allSame

#--------------------------------------------------------------------
ManagerBase_bit=$1
ManagerBase_dlllib=$2
ManagerBase_debugRelease=$3
ManagerBase_allSame=$4
"$CLOUD_REBUILD" ManagerBase $ManagerBase_bit $ManagerBase_dlllib $ManagerBase_debugRelease $ManagerBase_allSame