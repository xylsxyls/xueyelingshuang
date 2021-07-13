
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

#--------------------------------------------------------------------
IntDateTime_bit=$1
IntDateTime_dlllib=$2
IntDateTime_debugRelease=$3
IntDateTime_allSame=$4
"$CLOUD_REBUILD" IntDateTime $IntDateTime_bit $IntDateTime_dlllib $IntDateTime_debugRelease $IntDateTime_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

protocname="protoc.exe"

if [[ "$OSTYPE" =~ ^linux ]]; then
    protocname="protoc"
fi
$xueyelingshuang"/tools/protobuf/"$protocname -I=$xueyelingshuang"/src/LogTest/LogTest" --cpp_out=$xueyelingshuang"/src/LogSender/LogSender/src" $xueyelingshuang"/src/LogTest/LogTest/LogTestMessage.proto"