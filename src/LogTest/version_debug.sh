"$CLOUD_REBUILD" NetSender 32 dll debug same
"$CLOUD_REBUILD" CDump 32 dll debug same
"$CLOUD_REBUILD" LogManager 32 dll debug same

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

protocname="protoc.exe"

if [[ "$OSTYPE" =~ ^linux ]]; then
    protocname="protoc"
fi
$xueyelingshuang"/tools/protobuf/"$protocname -I=$xueyelingshuang"/src/LogTest/LogTest" --cpp_out=$xueyelingshuang"/src/LogTest/LogTest" $xueyelingshuang"/src/LogTest/LogTest/LogTestMessage.proto"