
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

SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

cp -f "$xueyelingshuang/tools/gmp/include/gmp.h" "$xueyelingshuang/include/"

if [ $1 = '32' ]
then
    cp -f "$xueyelingshuang/tools/gmp/lib/gmp.lib" "$xueyelingshuang/lib/"
else
    cp -f "$xueyelingshuang/tools/gmp/gmp64.dll/gmp64.dll/libgmp.dll.lib" "$xueyelingshuang/lib/"
    cp -f "$xueyelingshuang/tools/gmp/gmp64.dll/gmp64.dll/libgmp-10.dll" "$xueyelingshuang/lib/"
fi