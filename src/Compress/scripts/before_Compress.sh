
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

cp -f "$xueyelingshuang/tools/zlibrelease/zconf.h" "$xueyelingshuang/include/"
cp -f "$xueyelingshuang/tools/zlibrelease/zlib.h" "$xueyelingshuang/include/"

if [ $1 = '32' ]
then
    cp -f "$xueyelingshuang/tools/zlibrelease\zlibstat_32.lib" "$xueyelingshuang/lib/"
else
    cp -f "$xueyelingshuang/tools/zlibrelease\zlibstat_64.lib" "$xueyelingshuang/lib/"
fi