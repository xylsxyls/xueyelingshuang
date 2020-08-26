
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
DmSoft_bit=$1
DmSoft_dlllib=$2
DmSoft_debugRelease=$3
DmSoft_allSame=$4
"$CLOUD_REBUILD" DmSoft $DmSoft_bit $DmSoft_dlllib $DmSoft_debugRelease $DmSoft_allSame

#--------------------------------------------------------------------
CRandom_bit=$1
CRandom_dlllib=$2
CRandom_debugRelease=$3
CRandom_allSame=$4
"$CLOUD_REBUILD" CRandom $CRandom_bit $CRandom_dlllib $CRandom_debugRelease $CRandom_allSame

#--------------------------------------------------------------------
Rect_bit=$1
Rect_dlllib=$2
Rect_debugRelease=$3
Rect_allSame=$4
"$CLOUD_REBUILD" Rect $Rect_bit $Rect_dlllib $Rect_debugRelease $Rect_allSame