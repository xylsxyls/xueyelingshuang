
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

#--------------------------------------------------------------------
MysqlCpp_bit=$1
MysqlCpp_dlllib=$2
MysqlCpp_debugRelease=$3
MysqlCpp_allSame=$4
"$CLOUD_REBUILD" MysqlCpp $MysqlCpp_bit $MysqlCpp_dlllib $MysqlCpp_debugRelease $MysqlCpp_allSame

#--------------------------------------------------------------------
IntDateTime_bit=$1
IntDateTime_dlllib=$2
IntDateTime_debugRelease=$3
IntDateTime_allSame=$4
"$CLOUD_REBUILD" IntDateTime $IntDateTime_bit $IntDateTime_dlllib $IntDateTime_debugRelease $IntDateTime_allSame

#--------------------------------------------------------------------
HiRedis_bit=$1
HiRedis_dlllib=$2
HiRedis_debugRelease=$3
HiRedis_allSame=$4
"$CLOUD_REBUILD" HiRedis $HiRedis_bit $HiRedis_dlllib $HiRedis_debugRelease $HiRedis_allSame

#--------------------------------------------------------------------
CBase64_bit=$1
CBase64_dlllib=$2
CBase64_debugRelease=$3
CBase64_allSame=$4
"$CLOUD_REBUILD" CBase64 $CBase64_bit $CBase64_dlllib $CBase64_debugRelease $CBase64_allSame

#--------------------------------------------------------------------
ConfigManager_bit=$1
ConfigManager_dlllib=$2
ConfigManager_debugRelease=$3
ConfigManager_allSame=$4
"$CLOUD_REBUILD" ConfigManager $ConfigManager_bit $ConfigManager_dlllib $ConfigManager_debugRelease $ConfigManager_allSame