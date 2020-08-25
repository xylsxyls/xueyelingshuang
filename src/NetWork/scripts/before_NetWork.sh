
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CorrespondParam_bit=$1
CorrespondParam_dlllib=$2
CorrespondParam_debugRelease=$3
CorrespondParam_allSame=$4
"$CLOUD_REBUILD" CorrespondParam $CorrespondParam_bit $CorrespondParam_dlllib $CorrespondParam_debugRelease $CorrespondParam_allSame

#--------------------------------------------------------------------
LibuvTcp_bit=$1
LibuvTcp_dlllib=$2
LibuvTcp_debugRelease=$3
LibuvTcp_allSame=$4
"$CLOUD_REBUILD" LibuvTcp $LibuvTcp_bit $LibuvTcp_dlllib $LibuvTcp_debugRelease $LibuvTcp_allSame