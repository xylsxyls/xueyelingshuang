librariesrely=()

:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" DllRelyTest "$DllRelyTest_bit" "$DllRelyTest_dlllib" "$DllRelyTest_debugRelease" "$DllRelyTest_allSame"
fi
librariesrely+=($'project\tDllRelyTest\t'"$DllRelyTest_dlllib")
!EOF!

if [[ $1 != wasm32 && $1 != wasm64 ]]; then
SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang=$SHELL_FOLDER/../../..
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
mkdir -p "$sdkpath/lib"

libname="ffmpeg"
ffmpegpath=$xueyelingshuang"/tools/"$libname
includecreatedir=0

# Cross builds use the SDK branch in LumaPlayerCore/scripts/CMakeLists.txt.
# Never treat macOS/Android/iOS as Windows and copy incompatible vendor headers.
if [[ "${LUMAPLAYER_TARGET_PLATFORM:-}" == android || "${LUMAPLAYER_TARGET_PLATFORM:-}" == ios ||
      "${LUMAPLAYER_TARGET_PLATFORM:-}" == macos || "$OSTYPE" == darwin* ||
      "${CMAKE_SYSTEM_NAME:-}" == Android || "${CMAKE_SYSTEM_NAME:-}" == iOS ||
      "${CMAKE_SYSTEM_NAME:-}" == Darwin || "${CMAKE_TOOLCHAIN_FILE:-}" == *android.toolchain.cmake ||
      "${CMAKE_OSX_SYSROOT:-}" == *iphone* ]]; then
    echo 'Use -DLUMAPLAYER_DEPS_ROOT=<target SDK> for Core cross builds; shared Windows include/lib are not a target SDK.' >&2
    exit 1
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    includepath=$ffmpegpath"/include/ffmpeglinux/"
else
    includepath=$ffmpegpath"/include/ffmpegwindows/"
fi

copyincludepath=$sdkpath"/include/"
if [[ $includecreatedir == 1 ]]; then
    copyincludepath=$sdkpath"/include/"$libname"include/"
fi
if [ ! -d "$copyincludepath" ]
then
    mkdir -p "$copyincludepath"
fi
cp -Rf "$includepath/." "$copyincludepath/"

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
        cp -Pf "$ffmpeglibpath$libfile"* "$sdkpath/lib/" || exit 1
    done
else
    if [[ $3 == 'debug' ]]; then
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/MD/"
    else
        ffmpeglibpath=$ffmpegpath"/lib/"$bitfolder"/"
    fi
    for libfile in avformat avcodec swscale swresample avutil
    do
        cp -f "$ffmpeglibpath$libfile.lib" "$sdkpath/lib/" || exit 1
    done
    for dllfile in avformat-60.dll avcodec-60.dll swscale-7.dll swresample-4.dll avutil-58.dll
    do
        cp -f "$ffmpeglibpath$dllfile" "$sdkpath/lib/" || exit 1
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
mkdir -p "$sdkpath/include/soundtouchinclude"
cp -f "$soundtouchinclude/"*.h "$sdkpath/include/soundtouchinclude/" || exit 1
cp -f "$soundtouchlib/$soundtouchname" "$sdkpath/lib/" || exit 1
if [[ "$soundtouchname" != "$soundtouchruntime" ]]; then
    cp -f "$soundtouchlib/$soundtouchruntime" "$sdkpath/lib/" || exit 1
fi

if [[ "$OSTYPE" =~ ^linux ]]; then
    for ffmpeg_lib in libavformat.so libavcodec.so libswscale.so libswresample.so libavutil.so libSoundTouchDLL.so; do
        librariesrely+=($'link\t'"$ffmpeg_lib")
    done
else
    for ffmpeg_lib in avformat.lib avcodec.lib swscale.lib swresample.lib avutil.lib "$soundtouchname"; do
        librariesrely+=($'link\t'"$ffmpeg_lib")
    done
fi
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return