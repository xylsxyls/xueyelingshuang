:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

libname="ffmpeg"
ffmpegpath=$xueyelingshuang"/tools/"$libname
includecreatedir=0

if [[ "$OSTYPE" =~ ^linux ]]; then
    includepath=$ffmpegpath"/include/ffmpeglinux/"
else
    includepath=$ffmpegpath"/include/ffmpegwindows/"
fi

copyincludepath=$xueyelingshuang"/include/"
if [[ $includecreatedir == 1 ]]; then
    copyincludepath=$xueyelingshuang"/include/"$libname"include/"
fi
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
    ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/"
    for libfile in libavformatstatic libavcodecstatic libswscalestatic libswresamplestatic libavutilstatic
    do
        cp -rf "$ffmpeglibpath$libfile$libbitsuffix.a" "$xueyelingshuang/lib/"
    done
else
    if [[ $3 == 'debug' ]]; then
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/MD/"
        for libfile in avformat avcodec swscale swresample avutil
        do
            cp -rf "$ffmpeglibpath$libfile.lib" "$xueyelingshuang/lib/"
        done
        for dllfile in avformat-60.dll avcodec-60.dll swscale-7.dll swresample-4.dll avutil-58.dll
        do
            cp -rf "$ffmpeglibpath$dllfile" "$xueyelingshuang/lib/"
        done
    else
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/"
        for libfile in avformatstatic avcodecstatic swscalestatic swresamplestatic avutilstatic
        do
            cp -rf "$ffmpeglibpath$libfile$libbitsuffix.lib" "$xueyelingshuang/lib/"
        done
    fi
fi

# Sonic is a private static dependency; match the wrapper's CRT and architecture.
sonicpath="$xueyelingshuang/tools/sonic"
sonicinclude="$sonicpath/include/sonicwindows"
soniclib="$sonicpath/lib/$bitfolder"
sonicname="sonicstatic$libbitsuffix.lib"
if [[ "$OSTYPE" =~ ^linux ]]; then
    sonicinclude="$sonicpath/include/soniclinux"
    sonicname="libsonicstatic$libbitsuffix.a"
elif [[ $3 == 'debug' ]]; then
    soniclib="$soniclib/MD"
    sonicname="sonicstatic${libbitsuffix}d.lib"
fi
if [[ ! -f "$soniclib/$sonicname" || ! -f "$sonicinclude/sonic.h" ]]; then
    echo "Missing Sonic headers or matching library: $soniclib/$sonicname" >&2
    exit 1
fi
mkdir -p "$xueyelingshuang/include/sonicinclude"
cp -f "$sonicinclude/sonic.h" "$xueyelingshuang/include/sonicinclude/" || exit 1
cp -f "$soniclib/$sonicname" "$xueyelingshuang/lib/" || exit 1