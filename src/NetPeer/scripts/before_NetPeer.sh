
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
NetInterface_bit=$1
NetInterface_dlllib=$2
NetInterface_debugRelease=$3
NetInterface_allSame=$4
"$CLOUD_REBUILD" NetInterface $NetInterface_bit $NetInterface_dlllib $NetInterface_debugRelease $NetInterface_allSame

#--------------------------------------------------------------------
NetChannel_bit=$1
NetChannel_dlllib=$2
NetChannel_debugRelease=$3
NetChannel_allSame=$4
"$CLOUD_REBUILD" NetChannel $NetChannel_bit $NetChannel_dlllib $NetChannel_debugRelease $NetChannel_allSame