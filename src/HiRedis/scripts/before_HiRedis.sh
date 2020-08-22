
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
"$CLOUD_REBUILD" CStringManager $CStringManager_bit $CStringManager_dlllib $CStringManager_debugRelease $CStringManager_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

cp -r "$xueyelingshuang/tools/HiRedis/hiredisinclude/" "$xueyelingshuang/include/hiredisinclude/"

hiredislibpath="$xueyelingshuang/tools/HiRedis/$1/$3/"
for file in $(ls "$hiredislibpath")
do
    cp "$hiredislibpath$file" "$xueyelingshuang/lib/"
done