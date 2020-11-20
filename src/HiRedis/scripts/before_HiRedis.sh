
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

libname="hiredis"
windowslibname="hiredis"
linuxlibname="libhiredis"
includecreatedir=1

#该写法支持目录下含有子目录和空格
includepath=$xueyelingshuang"/tools/"$libname"/include/hirediswindows/"
if [[ "$OSTYPE" =~ ^linux ]]; then
    includepath=$xueyelingshuang"/tools/"$libname"/include/hiredislinux/"
fi
copyincludepath=$xueyelingshuang"/include/"
if [[ $includecreatedir == 1 ]]; then
    copyincludepath=$xueyelingshuang"/include/"$libname"include/"
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
libsuffix=".lib"

if [[ $3 == 'debug' ]]; then
    libdebugrelease="d"
else
    libdebugrelease=""
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    lib32=$linuxlibname"static32"
    lib64=$linuxlibname"static64"
    libsuffix=".a"
    libdebugrelease=""
fi

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
fi

windowslibname="Win32_Interop"
lib32=$windowslibname"static32"
lib64=$windowslibname"static64"

if [[ "$OSTYPE" =~ ^msys ]]; then
    if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
    then
        cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
    then
        cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
    then
        cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
    then
        cp -rf $xueyelingshuang"/tools/"$libname"/lib/"$lib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
    fi
fi