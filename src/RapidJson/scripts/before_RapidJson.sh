
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
httpserviceincludesrcpath="$xueyelingshuang/tools/rapid_json/"
httpserviceincludedestpath="$xueyelingshuang/include/rapid_json/"
if [ ! -d "$httpserviceincludedestpath" ]
then
    mkdir -p "$httpserviceincludedestpath"
fi
for file in $(ls "$httpserviceincludesrcpath")
do
    cp -rf "$httpserviceincludesrcpath$file" "$httpserviceincludedestpath"
done