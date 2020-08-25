
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
"$CLOUD_REBUILD" CTaskThreadManager $CTaskThreadManager_bit $CTaskThreadManager_dlllib $CTaskThreadManager_debugRelease $CTaskThreadManager_allSame

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
"$CLOUD_REBUILD" LockFreeQueue $LockFreeQueue_bit $LockFreeQueue_dlllib $LockFreeQueue_debugRelease $LockFreeQueue_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

#该写法支持目录下含有子目录和空格
httpserviceincludepath="$xueyelingshuang/tools/libuv/libuv/"
if [ ! -d "$xueyelingshuang/include/libuv/" ]
then
    mkdir -p "$xueyelingshuang/include/libuv/"
fi
for file in $(ls "$httpserviceincludepath")
do
    cp -rf "$httpserviceincludepath$file" "$xueyelingshuang/include/libuv/"
done

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/libuv/libuvd.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/libuv/libuv.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/libuv/libuvd.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/libuv/libuv.lib" "$xueyelingshuang/lib/"
fi