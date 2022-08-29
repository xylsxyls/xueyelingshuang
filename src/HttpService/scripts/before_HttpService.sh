
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

#该写法支持目录下含有子目录和空格
httpserviceincludepath="$xueyelingshuang/tools/civetweb/include/"
if [ ! -d "$xueyelingshuang/include/civetweb/" ]
then
    mkdir -p "$xueyelingshuang/include/civetweb/"
fi
for file in $(ls "$httpserviceincludepath")
do
    cp -rf "$httpserviceincludepath$file" "$xueyelingshuang/include/civetweb/"
done

civetweblib32="civetwebstatic32"
civetweblib64="civetwebstatic64"
libsuffix=".lib"

if [[ $3 == 'debug' ]]; then
    libdebugrelease="d"
else
    libdebugrelease=""
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    civetweblib32="libcivetwebstatic32"
    civetweblib64="libcivetwebstatic64"
    libsuffix=".a"
    libdebugrelease=""
fi

if [[ $1 == '32' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/civetweb/lib/"$civetweblib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '32' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/civetweb/lib/"$civetweblib32$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'debug' ]]
then
    cp -rf $xueyelingshuang"/tools/civetweb/lib/"$civetweblib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
elif [[ $1 == '64' ]] && [[ $3 == 'release' ]]
then
    cp -rf $xueyelingshuang"/tools/civetweb/lib/"$civetweblib64$libdebugrelease$libsuffix "$xueyelingshuang/lib/"
fi