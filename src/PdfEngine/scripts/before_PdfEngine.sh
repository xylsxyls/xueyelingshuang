SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

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
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
"$CLOUD_REBUILD" CSystem $CSystem_bit $CSystem_dlllib $CSystem_debugRelease $CSystem_allSame

libname="pdfium"
pdfiumpath=$xueyelingshuang"/tools/"$libname

if [[ "$OSTYPE" =~ ^linux ]]; then
    includepath=$pdfiumpath"/include/pdfiumlinux/"
else
    includepath=$pdfiumpath"/include/pdfiumwindows/"
fi

copyincludepath=$xueyelingshuang"/include/"
if [ ! -d "$copyincludepath" ]
then
    mkdir -p "$copyincludepath"
fi

for file in $(ls "$includepath")
do
    cp -rf "$includepath$file" "$copyincludepath"
done

if [[ $1 == '32' ]]; then
    bitfolder="x86"
    libbitsuffix="32"
elif [[ $1 == '64' ]]; then
    bitfolder="x64"
    libbitsuffix="64"
else
    bitfolder=$1
    libbitsuffix=$1
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    pdfiumlibpath=$pdfiumpath"/lib/"$bitfolder"/"
    if [ -f "$pdfiumlibpath/libpdfium.so" ]; then
        cp -rf "$pdfiumlibpath/libpdfium.so" "$xueyelingshuang/lib/"
    fi
    if [ -f "$pdfiumlibpath/libpdfiumstatic$libbitsuffix.a" ]; then
        cp -rf "$pdfiumlibpath/libpdfiumstatic$libbitsuffix.a" "$xueyelingshuang/lib/"
    fi
else
    if [[ $3 == 'debug' ]]; then
        pdfiumlibpath=$pdfiumpath"/lib/"$bitfolder"/MD/"
    else
        pdfiumlibpath=$pdfiumpath"/lib/"$bitfolder"/"
    fi

    cp -rf "$pdfiumlibpath/pdfium.lib" "$xueyelingshuang/lib/"
    cp -rf "$pdfiumlibpath/pdfium.dll" "$xueyelingshuang/lib/"

    if [ -f "$pdfiumlibpath/pdfiumstatic$libbitsuffix.lib" ]; then
        cp -rf "$pdfiumlibpath/pdfiumstatic$libbitsuffix.lib" "$xueyelingshuang/lib/"
    fi
fi
