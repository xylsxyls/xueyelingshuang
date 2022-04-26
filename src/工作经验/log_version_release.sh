"$CLOUD_REBUILD" NetSender 32 lib release same
"$CLOUD_REBUILD" NetWork 32 lib release same
"$CLOUD_REBUILD" CDump 32 lib release same
"$CLOUD_REBUILD" Compress 32 lib release same
"$CLOUD_REBUILD" Cini 32 lib release same
"$CLOUD_REBUILD" LogManager 32 lib release same
"$CLOUD_REBUILD" CRandom 32 lib release same
"$CLOUD_REBUILD" CHook 32 lib release same
"$CLOUD_REBUILD" CStopWatch 32 lib release same
"$CLOUD_REBUILD" DialogManager 32 lib release same
"$CLOUD_REBUILD" CMouse 32 dll release librely
"$CLOUD_REBUILD" CScreen 32 dll release librely
"$CLOUD_REBUILD" ScreenScript 32 dll release only
"$CLOUD_REBUILD" CKeyboard 32 dll release only

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../..

protocname="protoc.exe"

if [[ "$OSTYPE" =~ ^linux ]]; then
    protocname="protoc"
fi
$xueyelingshuang"/tools/protobuf/"$protocname -I=$xueyelingshuang"/src/LogTest/LogTest" --cpp_out=$xueyelingshuang"/src/LogTest/LogTest" $xueyelingshuang"/src/LogTest/LogTest/LogTestMessage.proto"

$xueyelingshuang"/tools/protobuf/"$protocname -I=$xueyelingshuang"/src/LogTest/LogTest" --cpp_out=$xueyelingshuang"/src/LogSender/LogSender/src" $xueyelingshuang"/src/LogTest/LogTest/LogTestMessage.proto"

"$CLOUD_REBUILD" LogSender 32 dll release only