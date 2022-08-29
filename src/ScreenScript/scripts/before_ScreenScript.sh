
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CMouse_bit=$1
CMouse_dlllib=$2
CMouse_debugRelease=$3
CMouse_allSame=$4
"$CLOUD_REBUILD" CMouse $CMouse_bit $CMouse_dlllib $CMouse_debugRelease $CMouse_allSame

#--------------------------------------------------------------------
CScreen_bit=$1
CScreen_dlllib=$2
CScreen_debugRelease=$3
CScreen_allSame=$4
"$CLOUD_REBUILD" CScreen $CScreen_bit $CScreen_dlllib $CScreen_debugRelease $CScreen_allSame

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
"$CLOUD_REBUILD" CSystem $CSystem_bit $CSystem_dlllib $CSystem_debugRelease $CSystem_allSame