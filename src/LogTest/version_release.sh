"$CLOUD_REBUILD" NetSender 64 lib release same
"$CLOUD_REBUILD" CDump 64 lib release same
"$CLOUD_REBUILD" LogManager 64 lib release same

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../..

protocname="protoc.exe"

if [[ "$OSTYPE" =~ ^linux ]]; then
    protocname="protoc"
fi
$xueyelingshuang"/tools/protobuf/"$protocname -I=$xueyelingshuang"/src/LogTest/LogTest" --cpp_out=$xueyelingshuang"/src/LogTest/LogTest" $xueyelingshuang"/src/LogTest/LogTest/LogTestMessage.proto"
