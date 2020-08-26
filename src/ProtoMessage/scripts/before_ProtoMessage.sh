
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
Variant_bit=$1
Variant_dlllib=$2
Variant_debugRelease=$3
Variant_allSame=$4
"$CLOUD_REBUILD" Variant $Variant_bit $Variant_dlllib $Variant_debugRelease $Variant_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

#该写法支持目录下含有子目录和空格
srcpath="$xueyelingshuang/tools/protobuf/google/"
destpath="$xueyelingshuang/include/google/"
if [ ! -d "$destpath" ]
then
    mkdir -p "$destpath"
fi
for file in $(ls "$srcpath")
do
    cp -rf "$srcpath$file" "$destpath"
done

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/protobuf/libprotobuf-lite_32d.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/protobuf/libprotobuf-lite_32.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/protobuf/libprotobuf-lite_64d.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/protobuf/libprotobuf-lite_64.lib" "$xueyelingshuang/lib/"
fi

"$xueyelingshuang/tools/protobuf/protoc.exe" -I="$xueyelingshuang/src/ProtoMessage/ProtoMessage/src" --cpp_out="$xueyelingshuang/src/ProtoMessage/ProtoMessage/src" "$xueyelingshuang/src/ProtoMessage/ProtoMessage/src/ProtoMessage.proto"