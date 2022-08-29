
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
"$CLOUD_REBUILD" LockFreeQueue $LockFreeQueue_bit $LockFreeQueue_dlllib $LockFreeQueue_debugRelease $LockFreeQueue_allSame

#--------------------------------------------------------------------
Semaphore_bit=$1
Semaphore_dlllib=$2
Semaphore_debugRelease=$3
Semaphore_allSame=$4
"$CLOUD_REBUILD" Semaphore $Semaphore_bit $Semaphore_dlllib $Semaphore_debugRelease $Semaphore_allSame

#--------------------------------------------------------------------
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

libname="libuv"
windowslibname="uv"
linuxlibname="libuv"
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