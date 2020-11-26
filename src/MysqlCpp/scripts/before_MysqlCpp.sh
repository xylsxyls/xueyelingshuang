
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

libname="mysqlcpp"
windowslibname="mysqlcppconn"
linuxlibname="libmysqlcppconn"
windowsdllname="libmysql.dll"
linuxdllname="libmysqlclient.so.18"
includecreatedir=0

#该写法支持目录下含有子目录和空格
includepath=$xueyelingshuang"/tools/"$libname"/include/"
copyincludepath=$xueyelingshuang"/include/"
if [[ $includecreatedir == 1 ]]; then
    copyincludepath=$xueyelingshuang"/include/"$libname"/"
fi
if [ ! -d $copyincludepath ]
then
    mkdir -p $copyincludepath
fi
for file in $(ls $includepath)
do
    cp -rf $includepath$file $copyincludepath
done

lib32=$windowslibname"static32"
lib64=$windowslibname"static64"
dllname=$windowsdllname
libsuffix=".lib"

if [[ $3 == 'debug' ]]; then
    libdebugrelease="d"
else
    libdebugrelease=""
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    lib32=$linuxlibname"static32"
    lib64=$linuxlibname"static64"
    dllname=$linuxdllname
    libsuffix=".a"
    libdebugrelease=""
fi

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x86/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x86/"$dllname "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x86/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x86/"$dllname "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x64/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x64/"$dllname "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x64/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/x64/"$dllname "$xueyelingshuang/lib/"
fi