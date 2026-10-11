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

# Cross builds use the SDK entry in LumaPlayerCore/scripts/PortableCore.cmake.
# Never treat macOS/Android/iOS as Windows and copy incompatible vendor headers.
if [[ "${LUMAPLAYER_TARGET_PLATFORM:-}" == android || "${LUMAPLAYER_TARGET_PLATFORM:-}" == ios ||
      "${LUMAPLAYER_TARGET_PLATFORM:-}" == macos || "$OSTYPE" == darwin* ]]; then
    echo 'Use -DLUMAPLAYER_DEPS_ROOT=<target SDK> for Core cross builds; shared Windows include/lib are not a target SDK.' >&2
    exit 1
fi

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
    for libfile in libavformat.so libavcodec.so libswscale.so libswresample.so libavutil.so
    do
        if [[ ! -f "$ffmpeglibpath$libfile" ]]; then
            echo "Missing shared FFmpeg library: $ffmpeglibpath$libfile; rebuild the target SDK with --enable-shared --disable-static." >&2
            exit 1
        fi
        cp -Pf "$ffmpeglibpath$libfile"* "$xueyelingshuang/lib/" || exit 1
    done
else
    if [[ $3 == 'debug' ]]; then
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/MD/"
    else
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/"
    fi
    for libfile in avformat avcodec swscale swresample avutil
    do
        cp -f "$ffmpeglibpath$libfile.lib" "$xueyelingshuang/lib/" || exit 1
    done
    for dllfile in avformat-60.dll avcodec-60.dll swscale-7.dll swresample-4.dll avutil-58.dll
    do
        cp -f "$ffmpeglibpath$dllfile" "$xueyelingshuang/lib/" || exit 1
    done
fi

# SoundTouch is a private dependency; keep the archive, license and CRT variants in tools.
soundtouchpath="$xueyelingshuang/tools/LumaPlayerKaraoke/soundtouch"
soundtouchinclude="$soundtouchpath/include/soundtouchwindows"
soundtouchlib="$soundtouchpath/lib/$bitfolder"
soundtouchname="SoundTouchDLL$libbitsuffix.lib"
soundtouchruntime="SoundTouchDLL.dll"
if [[ "$OSTYPE" =~ ^linux ]]; then
    soundtouchinclude="$soundtouchpath/include/soundtouchlinux"
    soundtouchname="libSoundTouchDLL.so"
    soundtouchruntime="$soundtouchname"
elif [[ $3 == 'debug' ]]; then
    soundtouchlib="$soundtouchlib/MD"
    soundtouchname="SoundTouchDLL${libbitsuffix}d.lib"
    soundtouchruntime="SoundTouchDLLd.dll"
fi
if [[ ! -f "$soundtouchlib/$soundtouchname" || ! -f "$soundtouchlib/$soundtouchruntime" ||
      ! -f "$soundtouchinclude/SoundTouchDLL.h" || ! -f "$soundtouchinclude/SoundTouch.h" ]]; then
    echo "Missing SoundTouch headers or matching library: $soundtouchlib/$soundtouchname" >&2
    exit 1
fi
mkdir -p "$xueyelingshuang/include/soundtouchinclude"
cp -f "$soundtouchinclude/"*.h "$xueyelingshuang/include/soundtouchinclude/" || exit 1
cp -f "$soundtouchlib/$soundtouchname" "$xueyelingshuang/lib/" || exit 1
if [[ "$soundtouchname" != "$soundtouchruntime" ]]; then
    cp -f "$soundtouchlib/$soundtouchruntime" "$xueyelingshuang/lib/" || exit 1
fi