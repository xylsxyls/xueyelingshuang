
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
SqlString_bit=$1
SqlString_dlllib=$2
SqlString_debugRelease=$3
SqlString_allSame=$4
"$CLOUD_REBUILD" SqlString $SqlString_bit $SqlString_dlllib $SqlString_debugRelease $SqlString_allSame

#--------------------------------------------------------------------
CCharset_bit=$1
CCharset_dlllib=$2
CCharset_debugRelease=$3
CCharset_allSame=$4
"$CLOUD_REBUILD" CCharset $CCharset_bit $CCharset_dlllib $CCharset_debugRelease $CCharset_allSame