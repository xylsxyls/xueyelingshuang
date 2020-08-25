
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

#该写法支持目录下含有子目录和空格
httpserviceincludepath="$xueyelingshuang/tools/mxml/include/"
for file in $(ls "$httpserviceincludepath")
do
    cp -rf "$httpserviceincludepath$file" "$xueyelingshuang/include/"
done

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/mxml/lib/mxmlstat_32.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/mxml/lib/mxmlstat_32.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/mxml/lib/mxmlstat_64.lib" "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/mxml/lib/mxmlstat_64.lib" "$xueyelingshuang/lib/"
fi