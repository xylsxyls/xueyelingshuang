
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
SqlString_bit=$1
SqlString_dlllib=$2
SqlString_debugRelease=$3
SqlString_allSame=$4
"$CLOUD_REBUILD" SqlString $SqlString_bit $SqlString_dlllib $SqlString_debugRelease $SqlString_allSame

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
"$CLOUD_REBUILD" CSystem $CSystem_bit $CSystem_dlllib $CSystem_debugRelease $CSystem_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

#该写法支持目录下含有子目录和空格
srcpath="$xueyelingshuang/tools/mysqlcpp/include/"
destpath="$xueyelingshuang/include/"
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
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib/release/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib/debug/"*.dll "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib/release/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib/release/"*.dll "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib64/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib64/"*.dll "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib64/"*.lib "$xueyelingshuang/lib/"
    cp -rf "$xueyelingshuang/tools/mysqlcpp/lib64/"*.dll "$xueyelingshuang/lib/"
fi