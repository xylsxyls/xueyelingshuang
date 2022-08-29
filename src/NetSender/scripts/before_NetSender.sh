
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
ProcessWork_bit=$1
ProcessWork_dlllib=$2
ProcessWork_debugRelease=$3
ProcessWork_allSame=$4
"$CLOUD_REBUILD" ProcessWork $ProcessWork_bit $ProcessWork_dlllib $ProcessWork_debugRelease $ProcessWork_allSame

#--------------------------------------------------------------------
ProtoMessage_bit=$1
ProtoMessage_dlllib=$2
ProtoMessage_debugRelease=$3
ProtoMessage_allSame=$4
"$CLOUD_REBUILD" ProtoMessage $ProtoMessage_bit $ProtoMessage_dlllib $ProtoMessage_debugRelease $ProtoMessage_allSame